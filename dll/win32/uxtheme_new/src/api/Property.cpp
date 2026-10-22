/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme property getters
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"
#include "../load/VSRecord.hpp"
#include "../load/ThemeLoader.hpp"
#include "../model/Tmt.hpp"

static const CThemeProperty* Resolve(HTHEME hTheme, int iPartId, int iStateId,
                                     int iPropId)
{
    THEMEHANDLE* p = ValidateThemeHandle(hTheme);

    if (!p || !p->pFile)
        return NULL;

    return p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, iPropId);
}

#define E_PROP_ID_UNSUPPORTED HRESULT_FROM_WIN32(ERROR_NOT_FOUND)

extern "C" {
HRESULT WINAPI GetThemeInt(HTHEME hTheme, int iPartId, int iStateId,
                           int iPropId, int* piVal)
{
    const CThemeProperty* p;

    if (!piVal)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *piVal = p->value.GetInt();
    return S_OK;
}

HRESULT WINAPI GetThemeEnumValue(HTHEME hTheme, int iPartId, int iStateId,
                                 int iPropId, int* piVal)
{
    return GetThemeInt(hTheme, iPartId, iStateId, iPropId, piVal);
}

HRESULT WINAPI GetThemeBool(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, BOOL* pfVal)
{
    const CThemeProperty* p;

    if (!pfVal)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *pfVal = p->value.GetBool();
    return S_OK;
}

HRESULT WINAPI GetThemeColor(HTHEME hTheme, int iPartId, int iStateId,
                             int iPropId, COLORREF* pColor)
{
    const CThemeProperty* p;

    if (!pColor)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *pColor = p->value.GetColor();
    return S_OK;
}

HRESULT WINAPI GetThemePosition(HTHEME hTheme, int iPartId, int iStateId,
                                int iPropId, POINT* pPoint)
{
    const CThemeProperty* p;

    if (!pPoint)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *pPoint = p->value.GetPosition();
    return S_OK;
}

HRESULT WINAPI GetThemeRect(HTHEME hTheme, int iPartId, int iStateId,
                            int iPropId, RECT* pRect)
{
    const CThemeProperty* p;

    if (!pRect)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *pRect = p->value.GetRect();
    return S_OK;
}

HRESULT WINAPI GetThemeMargins(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                               int iPropId, RECT* prc, MARGINS* pMargins)
{
    const CThemeProperty* p;

    (void)hdc;
    (void)prc;

    if (!pMargins)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    *pMargins = p->value.GetMargins();
    return S_OK;
}

HRESULT WINAPI GetThemeFont(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                            int iPropId, LOGFONTW* pFont)
{
    const CThemeProperty* p;
    const LOGFONTW* plf;

    if (!pFont)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    plf = p->value.GetFont();
    if (!plf)
        return E_PROP_ID_UNSUPPORTED;

    *pFont = *plf;

    if (hdc)
    {
        int nDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        if (nDpi > 0 && nDpi != 96)
            pFont->lfHeight = MulDiv(pFont->lfHeight, nDpi, 96);
    }

    return S_OK;
}

HRESULT WINAPI GetThemeIntList(HTHEME hTheme, int iPartId, int iStateId,
                               int iPropId, INTLIST* pIntList)
{
    const CThemeProperty* p;
    int c;

    if (!pIntList)
        return E_POINTER;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    c = p->value.GetIntListCount();
    if (c > MAX_INTLIST_COUNT)
        c = MAX_INTLIST_COUNT;

    pIntList->iValueCount = c;
    if (c > 0)
        memcpy(pIntList->iValues, p->value.GetIntList(), (SIZE_T)c * sizeof(int));

    return S_OK;
}

HRESULT WINAPI GetThemeString(HTHEME hTheme, int iPartId, int iStateId,
                              int iPropId, LPWSTR pszBuff, int cchMaxBuffChars)
{
    const CThemeProperty* p;

    if (!pszBuff || cchMaxBuffChars <= 0)
        return E_INVALIDARG;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    return StringCchCopyW(pszBuff, cchMaxBuffChars, p->value.GetString());
}

HRESULT WINAPI GetThemeFilename(HTHEME hTheme, int iPartId, int iStateId,
                                int iPropId, LPWSTR pszBuff, int cchMaxBuffChars)
{
    const CThemeProperty* p;

    if (!pszBuff || cchMaxBuffChars <= 0)
        return E_INVALIDARG;

    p = Resolve(hTheme, iPartId, iStateId, iPropId);
    if (!p)
        return E_PROP_ID_UNSUPPORTED;

    if (p->value.Kind() != TPID_FILENAME && p->value.Kind() != TPID_STRING)
    {
        UXGAP("GetThemeFilename on a resource-backed image - there is no path");
        return E_PROP_ID_UNSUPPORTED;
    }

    return StringCchCopyW(pszBuff, cchMaxBuffChars, p->value.GetString());
}

BOOL WINAPI IsThemePartDefined(HTHEME hTheme, int iPartId, int iStateId)
{
    THEMEHANDLE* p = ValidateThemeHandle(hTheme);
    const CThemeClass* pClass;
    const CThemePart* pPart;

    (void)iStateId;

    if (!p || !p->pFile)
        return FALSE;

    if (p->iClass < 0 || p->iClass >= p->pFile->classes.GetCount())
        return FALSE;

    pClass = &p->pFile->classes[p->iClass];

    if (iPartId == 0)
        return TRUE;

    pPart = pClass->FindPart(iPartId);
    return pPart != NULL;
}

HRESULT WINAPI GetThemePropertyOrigin(HTHEME hTheme, int iPartId, int iStateId,
                                      int iPropId, PROPERTYORIGIN* pOrigin)
{
    THEMEHANDLE* p = ValidateThemeHandle(hTheme);
    const CThemeClass* pClass;
    const CThemePart* pPart;

    if (!pOrigin)
        return E_POINTER;

    if (!p || !p->pFile || p->iClass < 0)
        return E_HANDLE;

    pClass = &p->pFile->classes[p->iClass];
    pPart = pClass->FindPart(iPartId);

    if (pPart)
    {
        if (iStateId != 0)
        {
            const CThemeState* pState = pPart->FindState(iStateId);
            if (pState && pState->FindProperty(iPropId))
            {
                *pOrigin = PO_STATE;
                return S_OK;
            }
        }
        if (pPart->FindProperty(iPropId))
        {
            *pOrigin = PO_PART;
            return S_OK;
        }
    }

    if (pClass->FindProperty(iPropId))
    {
        *pOrigin = PO_CLASS;
        return S_OK;
    }

    if (p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, iPropId))
    {
        *pOrigin = PO_GLOBAL;
        return S_OK;
    }

    *pOrigin = PO_NOTFOUND;
    return S_OK;
}

HRESULT WINAPI GetThemeDocumentationProperty(LPCWSTR pszThemeName,
                                             LPCWSTR pszPropertyName,
                                             LPWSTR pszValueBuff,
                                             int cchMaxValChars)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;
    int sym = -1;
    int i;

    static const struct { const WCHAR* psz; int sym; } rgNames[] = {
        { L"Name",        TMT_NAME },
        { L"DisplayName", TMT_DISPLAYNAME },
        { L"ToolTip",     TMT_TOOLTIP },
        { L"Company",     TMT_COMPANY },
        { L"Author",      TMT_AUTHOR },
        { L"Copyright",   TMT_COPYRIGHT },
        { L"Url",         TMT_URL },
        { L"Version",     TMT_VERSION },
        { L"Description", TMT_DESCRIPTION },
    };

    if (!pszValueBuff || cchMaxValChars <= 0)
        return E_INVALIDARG;

    for (i = 0; i < ARRAYSIZE(rgNames); i++)
        if (_wcsicmp(pszPropertyName, rgNames[i].psz) == 0)
            sym = rgNames[i].sym;

    if (sym < 0)
        return E_PROP_ID_UNSUPPORTED;

    hr = CThemeLoader::LoadTheme(pszThemeName, NULL, NULL, &pFile);
    if (FAILED(hr))
        return hr;

    hr = E_PROP_ID_UNSUPPORTED;
    for (i = 0; i < pFile->rootProperties.GetCount(); i++)
    {
        if (pFile->rootProperties[i].lSymbolVal == sym)
        {
            hr = StringCchCopyW(pszValueBuff, cchMaxValChars,
                                pFile->rootProperties[i].value.GetString());
            break;
        }
    }

    pFile->Release();
    return hr;
}

COLORREF WINAPI GetThemeSysColor(HTHEME hTheme, int iColorId)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, TMT_SCROLLBAR + iColorId);
            if (p)
                return p->value.GetColor();
        }
    }

    return GetSysColor(iColorId);
}

HBRUSH WINAPI GetThemeSysColorBrush(HTHEME hTheme, int iColorId)
{
    return CreateSolidBrush(GetThemeSysColor(hTheme, iColorId));
}

int WINAPI GetThemeSysSize(HTHEME hTheme, int iSizeId)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, iSizeId);
            if (p)
                return p->value.GetInt();
        }
    }

    return GetSystemMetrics(iSizeId);
}

BOOL WINAPI GetThemeSysBool(HTHEME hTheme, int iBoolId)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, iBoolId);
            if (p)
                return p->value.GetBool();
        }
    }

    return FALSE;
}

HRESULT WINAPI GetThemeSysInt(HTHEME hTheme, int iIntId, int* piValue)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (!piValue)
        return E_POINTER;

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, iIntId);
            if (p)
            {
                *piValue = p->value.GetInt();
                return S_OK;
            }
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

HRESULT WINAPI GetThemeSysFont(HTHEME hTheme, int iFontId, LOGFONTW* plf)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (!plf)
        return E_POINTER;

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, iFontId);
            if (p)
            {
                const LOGFONTW* plfSrc = p->value.GetFont();
                if (plfSrc)
                {
                    *plf = *plfSrc;
                    return S_OK;
                }
            }
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

HRESULT WINAPI GetThemeSysString(HTHEME hTheme, int iStringId,
                                 LPWSTR pszStringBuff, int cchMaxStringChars)
{
    const CThemeProperty* p;
    THEMEHANDLE* ph = ValidateThemeHandle(hTheme);

    if (!pszStringBuff || cchMaxStringChars <= 0)
        return E_INVALIDARG;

    if (ph && ph->pFile)
    {
        int iSys = ph->pFile->FindClass(L"sysmetrics");
        if (iSys >= 0)
        {
            p = ph->pFile->ResolveProperty(iSys, 0, 0, iStringId);
            if (p)
                return StringCchCopyW(pszStringBuff, cchMaxStringChars,
                                      p->value.GetString());
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

HRESULT WINAPI GetThemeMetric(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                              int iPropId, int* piVal)
{
    (void)hdc;
    return GetThemeInt(hTheme, iPartId, iStateId, iPropId, piVal);
}

}
