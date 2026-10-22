/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Builds the theme object model
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeLoader.hpp"
#include "../model/Tmt.hpp"

HRESULT DeriveMuiPath(const WCHAR* pszStylePath, WCHAR* pszOut, int cchOut)
{
    WCHAR szDir[MAX_PATH];
    const WCHAR* pszFile;
    const WCHAR* pszSlash;
    WCHAR szLang[LOCALE_NAME_MAX_LENGTH];
    int i;

    pszSlash = wcsrchr(pszStylePath, L'\\');
    if (!pszSlash)
        pszSlash = wcsrchr(pszStylePath, L'/');
    if (!pszSlash)
        return S_FALSE;

    pszFile = pszSlash + 1;
    if ((SIZE_T)(pszSlash - pszStylePath) >= ARRAYSIZE(szDir))
        return S_FALSE;

    memcpy(szDir, pszStylePath, (SIZE_T)(pszSlash - pszStylePath) * sizeof(WCHAR));
    szDir[pszSlash - pszStylePath] = L'\0';

    for (i = 0; i < 2; i++)
    {
        if (i == 0)
        {
            if (!GetUserDefaultLocaleName(szLang, ARRAYSIZE(szLang)))
                continue;
        }
        else
        {
            StringCchCopyW(szLang, ARRAYSIZE(szLang), L"en-US");
        }

        if (FAILED(StringCchPrintfW(pszOut, cchOut, L"%s\\%s\\%s.mui",
                                    szDir, szLang, pszFile)))
            continue;

        if (GetFileAttributesW(pszOut) != INVALID_FILE_ATTRIBUTES)
            return S_OK;
    }

    pszOut[0] = L'\0';
    return S_FALSE;
}

CThemeLoader::CThemeLoader() : m_pFile(NULL), m_pUnpack(NULL), m_fLoadingRoot(false)
{
}

CThemeLoader::~CThemeLoader()
{
}

HRESULT CThemeLoader::AddData(int iClass, int iPart, int iState,
                              int lSymbolVal, int lType, UINT uResID,
                              const void* pvData, int cbData)
{
    CThemeProperty* pProp;
    CValue* pValue;
    TPID id;

    if (!m_pFile)
        return E_UNEXPECTED;

    if (iClass < 0 || iClass >= m_pFile->classNames.GetCount())
        return S_OK;

    if (m_fLoadingRoot)
    {
        pProp = m_pFile->rootProperties.Add();
        if (!pProp)
            return E_OUTOFMEMORY;

        pProp->iClass = iClass;
        pProp->iPartId = iPart;
        pProp->iStateId = iState;
        pProp->lSymbolVal = lSymbolVal;
        pProp->lType = lType;
        pProp->value.SetKind(GetThemePrimitiveID(lSymbolVal, lType));
        pProp->value.SetResourceId(uResID);
        return pProp->value.SetData(pvData, cbData);
    }

    {
        int iCls = m_pFile->EnsureClass(m_pFile->classNames[iClass].Get());
        if (iCls < 0)
            return E_OUTOFMEMORY;

        CThemeClass& cls = m_pFile->classes[iCls];

        if (iPart == 0)
        {
            pProp = cls.properties.Add();
        }
        else
        {
            CThemePart* pPart = cls.EnsurePart(iPart);
            if (!pPart)
                return E_OUTOFMEMORY;

            if (iState == 0)
            {
                pProp = pPart->properties.Add();
            }
            else
            {
                CThemeState* pState = pPart->EnsureState(iState);
                if (!pState)
                    return E_OUTOFMEMORY;
                pProp = pState->properties.Add();
            }
        }
    }

    if (!pProp)
        return E_OUTOFMEMORY;

    pProp->iClass = iClass;
    pProp->iPartId = iPart;
    pProp->iStateId = iState;
    pProp->lSymbolVal = lSymbolVal;
    pProp->lType = lType;

    pValue = &pProp->value;
    id = GetThemePrimitiveID(lSymbolVal, lType);
    pValue->SetKind(id);

    pValue->SetResourceId(uResID);

    switch (id)
    {
    case TPID_ENUM:
    case TPID_INT:
    case TPID_SIZE:
        if (cbData >= 4)
            pValue->SetInt(*(const int*)pvData);
        break;

    case TPID_BOOL:
        if (cbData >= 4)
            pValue->SetBool(*(const int*)pvData ? TRUE : FALSE);
        break;

    case TPID_COLOR:
        if (cbData >= 4)
            pValue->SetColor(*(const COLORREF*)pvData);
        break;

    case TPID_MARGINS:
        if (cbData >= (int)sizeof(MARGINS))
            pValue->SetMargins(*(const MARGINS*)pvData);
        break;

    case TPID_RECT:
        if (cbData >= (int)sizeof(RECT))
            pValue->SetRect(*(const RECT*)pvData);
        break;

    case TPID_POSITION:
        if (cbData >= (int)sizeof(POINT))
            pValue->SetPosition(*(const POINT*)pvData);
        break;

    default:
        break;
    }

    return pValue->SetData(pvData, cbData);
}

HRESULT CThemeLoader::AddVariant(const WCHAR* pszName, const WCHAR* pszSize,
                                 const WCHAR* pszColor)
{
    CVariantEntry* p;

    if (!m_pFile)
        return E_UNEXPECTED;

    p = m_pFile->variants.Add();
    if (!p)
        return E_OUTOFMEMORY;

    p->strName.Set(pszName);
    p->strSize.Set(pszSize);
    p->strColor.Set(pszColor);
    return S_OK;
}

HRESULT CThemeLoader::AddBaseClass(int idClass, int idBaseClass)
{
    const int iCmapOffset = 4;
    int iCls;
    int iBase;

    if (!m_pFile)
        return E_UNEXPECTED;

    if (idBaseClass < 0)
        return S_OK;

    if (idClass + iCmapOffset >= m_pFile->classNames.GetCount() ||
        idBaseClass + iCmapOffset >= m_pFile->classNames.GetCount())
    {
        return S_OK;
    }

    iCls = m_pFile->FindClass(m_pFile->classNames[idClass + iCmapOffset].Get());
    iBase = m_pFile->FindClass(m_pFile->classNames[idBaseClass + iCmapOffset].Get());

    if (iCls >= 0 && iBase >= 0 && iCls != iBase)
        m_pFile->classes[iCls].iBaseClass = iBase;

    return S_OK;
}

HRESULT CThemeLoader::Load(const WCHAR* pszPath, const WCHAR* pszColor,
                           const WCHAR* pszSize)
{
    WCHAR szMui[MAX_PATH];
    CVSUnpack unpack;
    HRESULT hr;
    int i;

    hr = m_pFile->style.Open(pszPath);
    if (FAILED(hr))
        return hr;

    m_pFile->nVersion = m_pFile->style.GetVersion();
    m_pFile->strPath.Set(pszPath);

    if (DeriveMuiPath(pszPath, szMui, ARRAYSIZE(szMui)) == S_OK)
    {
        HINSTANCE h = LoadLibraryExW(szMui, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (h)
            m_pFile->mui.AttachRaw(h);
    }

    hr = unpack.Initialize(&m_pFile->style,
                           m_pFile->mui.IsOpen() ? &m_pFile->mui : NULL);
    if (FAILED(hr))
        return hr;

    for (i = 0; i < unpack.GetClassCount(); i++)
    {
        CStringW* p = m_pFile->classNames.Add();
        if (!p)
            return E_OUTOFMEMORY;
        p->Set(unpack.GetClassName(i));
    }

    m_fLoadingRoot = true;
    hr = unpack.LoadRootMap(this);
    m_fLoadingRoot = false;
    if (FAILED(hr))
    {
        UXTRACE(("uxtheme_new: RMAP absent or unreadable (hr=%08lx)\n",
                 (unsigned long)hr));
    }

    hr = unpack.EnumVariants(this);
    if (FAILED(hr))
        UXTRACE(("uxtheme_new: VMAP unreadable (hr=%08lx)\n", (unsigned long)hr));

    hr = unpack.LoadClassDataMap(pszColor, pszSize, this);
    if (FAILED(hr))
        return hr;

    m_pFile->strVariantName.Set(unpack.GetVariantName());
    m_pFile->strSizeName.Set(unpack.GetVariantSize());
    m_pFile->strColorName.Set(unpack.GetVariantColor());

    hr = unpack.LoadBaseClassDataMap(this);
    if (FAILED(hr))
        return hr;

    UXTRACE(("uxtheme_new: loaded '%ls' variant '%ls' (%ls/%ls): "
             "%d cmap names, %d classes with data\n",
             pszPath, m_pFile->strVariantName.Get(),
             m_pFile->strColorName.Get(), m_pFile->strSizeName.Get(),
             m_pFile->classNames.GetCount(), m_pFile->classes.GetCount()));

    return S_OK;
}

HRESULT CThemeLoader::LoadTheme(const WCHAR* pszPath, const WCHAR* pszColor,
                                const WCHAR* pszSize, CThemeFile** ppFile)
{
    CThemeLoader loader;
    CThemeFile* pFile;
    HRESULT hr;

    if (!ppFile)
        return E_POINTER;

    *ppFile = NULL;

    pFile = (CThemeFile*)ThemeHeapAlloc(0, sizeof(CThemeFile));
    if (!pFile)
        return E_OUTOFMEMORY;

    UX_NEW(pFile) CThemeFile();

    loader.m_pFile = pFile;
    hr = loader.Load(pszPath, pszColor, pszSize);

    if (FAILED(hr))
    {
        pFile->~CThemeFile();
        ThemeHeapFree(pFile);
        return hr;
    }

    *ppFile = pFile;
    return S_OK;
}
