/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Property record stream walker
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "VSUnpack.hpp"
#include "../model/Tmt.hpp"

static const WCHAR c_szCMAP[]  = L"CMAP";
static const WCHAR c_szVMAP[]  = L"VMAP";
static const WCHAR c_szBCMAP[] = L"BCMAP";
static const WCHAR c_szRMAP[]  = L"RMAP";

static const WCHAR c_szDocumentationElement[] = L"documentation";
static const WCHAR c_szGlobalsElement[]       = L"globals";
static const WCHAR c_szSysmetsElement[]       = L"SysMetrics";

static HRESULT ReadVSVariant(const BYTE* pb, int cb, int* pcbPos,
                             CStringW* pName, CStringW* pSize, CStringW* pColor);

int GetScreenDpi(void)
{
    HDC hdc = GetDC(NULL);
    int nDpi = 96;

    if (hdc)
    {
        nDpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }

    return nDpi ? nDpi : 96;
}

CVSUnpack::CVSUnpack()
    : m_iCurClass(-1), m_iCurPart(0), m_iCurState(0), m_pStyle(NULL)
{
    m_ctx.pStyle = NULL;
    m_ctx.pMui = NULL;
    _ClearDpiRecords();
}

CVSUnpack::~CVSUnpack()
{
}

void CVSUnpack::_ClearDpiRecords()
{
    for (int i = 0; i < 5; i++)
    {
        m_rgImageDpiRec[i] = NULL;
        m_rgImageRec[i] = NULL;
    }
}

HRESULT CVSUnpack::Initialize(const CPeModule* pStyle, const CPeModule* pMui)
{
    const void* pv;
    int cb;
    HRESULT hr;
    const WCHAR* psz;

    m_pStyle = pStyle;
    m_ctx.pStyle = pStyle;
    m_ctx.pMui = pMui;

    hr = _AcquireMap(c_szCMAP, &pv, &cb);
    if (FAILED(hr))
        return hr;

    psz = (const WCHAR*)pv;
    while (cb > 0)
    {
        int cch = (int)wcsnlen(psz, (SIZE_T)cb / sizeof(WCHAR));
        unsigned int cbStride;
        CStringW* pName;

        if (cch == 0 && cb < (int)sizeof(WCHAR))
            break;

        pName = m_rgClassNames.Add();
        if (!pName)
            return E_OUTOFMEMORY;

        hr = pName->Append(psz, cch);
        if (FAILED(hr))
            return hr;

        cbStride = 8u * ((unsigned int)(2 * cch + 9) >> 3);
        if ((int)cbStride <= 0 || (int)cbStride > cb)
            break;

        cb -= (int)cbStride;
        psz = (const WCHAR*)((const BYTE*)psz + cbStride);
    }

    UXTRACE(("uxtheme_new: CMAP has %d classes\n", m_rgClassNames.GetCount()));
    return S_OK;
}

HRESULT CVSUnpack::_AcquireMap(const WCHAR* pszMap, const void** ppvMap, int* pcbMap)
{
    DWORD cb = 0;
    HRESULT hr;

    hr = m_pStyle->GetResource(pszMap, pszMap, ppvMap, &cb);
    if (FAILED(hr))
        return hr;

    *pcbMap = (int)cb;
    return S_OK;
}

int CVSUnpack::_FindClass(const WCHAR* pszClass) const
{
    for (int i = 0; i < m_rgClassNames.GetCount(); i++)
        if (m_rgClassNames[i].EqualsI(pszClass))
            return i;
    return -1;
}

HRESULT CVSUnpack::_AddVSDataRecord(IParserCallBack* pfnCB, const VSRECORD* pRec)
{
    BYTE rgStack[256];
    BYTE* pb = rgStack;
    int cbData;
    HRESULT hr;

    cbData = pRec->cbData;
    if (cbData < 0)
        return E_INVALIDARG;

    if (cbData > (int)sizeof(rgStack))
    {
        pb = (BYTE*)ThemeHeapAlloc(0, (SIZE_T)cbData);
        if (!pb)
            return E_OUTOFMEMORY;
    }

    hr = LoadVSRecordData(&m_ctx, pRec, pb, &cbData);
    if (SUCCEEDED(hr))
    {
        hr = pfnCB->AddData(m_iCurClass, m_iCurPart, m_iCurState,
                            pRec->lSymbolVal, pRec->lType, pRec->uResID,
                            pb, cbData);
    }
    else
    {
        UXTRACE(("uxtheme_new: record sym=%d type=%d res=%u dropped (hr=%08lx)\n",
                 pRec->lSymbolVal, pRec->lType, pRec->uResID, (unsigned long)hr));
        hr = S_OK;
    }

    if (pb != rgStack)
        ThemeHeapFree(pb);

    return hr;
}

HRESULT CVSUnpack::_FixSymbolAndAddVSDataRecord(IParserCallBack* pfnCB,
                                                const VSRECORD* pRec,
                                                int lSymbolVal)
{
    BYTE rgData[292];
    VSRECORD* pFixed;

    if (pRec->lSymbolVal == lSymbolVal)
        return _AddVSDataRecord(pfnCB, pRec);

    if (pRec->uResID)
    {
        memcpy(rgData, pRec, sizeof(VSRECORD));
    }
    else
    {
        if ((unsigned int)(pRec->cbData + 32) > sizeof(rgData))
            return E_FAIL;
        memcpy(rgData, pRec, (SIZE_T)pRec->cbData + 32);
    }

    pFixed = (VSRECORD*)rgData;
    pFixed->lSymbolVal = lSymbolVal;
    return _AddVSDataRecord(pfnCB, pFixed);
}

bool CVSUnpack::_DelayRecord(const VSRECORD* pRec)
{
    int sym = pRec->lSymbolVal;

    if (sym >= TMT_MINDPI1 && sym <= TMT_MINDPI5)
        return true;

    if ((unsigned int)(sym - TMT_IMAGEFILE1) <= 4)
        return true;

    return false;
}

HRESULT CVSUnpack::_SaveRecord(const VSRECORD* pRec)
{
    int sym = pRec->lSymbolVal;

    if (sym >= TMT_MINDPI1 && sym <= TMT_MINDPI5)
        m_rgImageDpiRec[sym - TMT_MINDPI1] = pRec;
    else if ((unsigned int)(sym - TMT_IMAGEFILE1) <= 4)
        m_rgImageRec[sym - TMT_IMAGEFILE1] = pRec;

    return S_OK;
}

HRESULT CVSUnpack::_FlushDelayedRecords(IParserCallBack* pfnCB)
{
    int rgDpi[5];
    int lClosestDpiIdx = -1;
    int lDist = 0x7FFFFFFF;
    int nScreenDpi = GetScreenDpi();
    HRESULT hr = S_OK;
    int i;
    int lSymbolVal;

    for (i = 0; i < 5 && SUCCEEDED(hr); i++)
    {
        int cbData = 4;

        if (!m_rgImageDpiRec[i])
        {
            rgDpi[i] = 0;
            continue;
        }

        hr = LoadVSRecordData(&m_ctx, m_rgImageDpiRec[i], &rgDpi[i], &cbData);
        if (SUCCEEDED(hr))
        {
            int d = rgDpi[i] - nScreenDpi;
            if (d < 0)
                d = -d;
            if (d <= lDist)
            {
                lClosestDpiIdx = i;
                lDist = d;
            }
        }
        else
        {
            rgDpi[i] = 0;
            hr = S_OK;
        }
    }

    lSymbolVal = TMT_MINDPI1;
    for (i = 0; i < 5 && SUCCEEDED(hr); i++)
    {
        if (!m_rgImageDpiRec[i])
            continue;
        if (i != lClosestDpiIdx && rgDpi[i] != 96)
            continue;

        hr = _FixSymbolAndAddVSDataRecord(pfnCB, m_rgImageDpiRec[i], lSymbolVal++);
    }

    lSymbolVal = TMT_IMAGEFILE1;
    for (i = 0; i < 5 && SUCCEEDED(hr); i++)
    {
        if (!m_rgImageRec[i])
            continue;
        if (i != lClosestDpiIdx && rgDpi[i] != 96 && rgDpi[i] != 0)
            continue;

        hr = _FixSymbolAndAddVSDataRecord(pfnCB, m_rgImageRec[i], lSymbolVal++);
    }

    _ClearDpiRecords();

    return hr;
}

HRESULT CVSUnpack::LoadRootMap(IParserCallBack* pfnCB)
{
    const void* pv;
    int cb;
    int cbPos = 0;
    int iDocClass;
    const VSRECORD* pRec;
    HRESULT hr;

    hr = _AcquireMap(c_szRMAP, &pv, &cb);
    if (FAILED(hr))
        return hr;

    iDocClass = _FindClass(c_szDocumentationElement);
    (void)iDocClass;

    m_iCurPart = 0;
    m_iCurState = 0;

    for (pRec = (const VSRECORD*)pv;
         pRec && SUCCEEDED(hr);
         pRec = GetNextVSRecord(pRec, cb, &cbPos))
    {
        m_iCurClass = pRec->iClass;
        hr = _AddVSDataRecord(pfnCB, pRec);
    }

    return hr;
}

HRESULT CVSUnpack::EnumVariants(IParserCallBack* pfnCB)
{
    const void* pv;
    int cb;
    int cbPos = 0;
    HRESULT hr;

    hr = _AcquireMap(c_szVMAP, &pv, &cb);
    if (FAILED(hr))
        return hr;

    while (cbPos < cb)
    {
        CStringW strVariant, strSize, strColor;

        hr = ReadVSVariant((const BYTE*)pv, cb, &cbPos,
                           &strVariant, &strSize, &strColor);
        if (FAILED(hr))
            break;

        hr = pfnCB->AddVariant(strVariant.Get(), strSize.Get(), strColor.Get());
        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

HRESULT CVSUnpack::LoadBaseClassDataMap(IParserCallBack* pfnCB)
{
    const void* pv;
    int cb;
    const int* pn;
    int cClasses;
    HRESULT hr;

    hr = _AcquireMap(c_szBCMAP, &pv, &cb);
    if (FAILED(hr))
    {
        return S_OK;
    }

    if (cb < (int)sizeof(int))
        return S_OK;

    cClasses = *(const int*)pv;
    pn = (const int*)pv + 1;

    if (cClasses < 0 || (SIZE_T)cClasses * sizeof(int) + sizeof(int) > (SIZE_T)cb)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    for (int i = 0; i < cClasses && SUCCEEDED(hr); i++)
        hr = pfnCB->AddBaseClass(i, pn[i]);

    return hr;
}

static HRESULT ReadVSVariant(const BYTE* pb, int cb, int* pcbPos,
                             CStringW* pName, CStringW* pSize, CStringW* pColor)
{
    CStringW* rg[3];
    int i;

    rg[0] = pName;
    rg[1] = pSize;
    rg[2] = pColor;

    if (cb < 4 || *pcbPos >= cb)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    for (i = 0; i < 3; i++)
    {
        unsigned int cch;
        unsigned int cbStride;

        if (*pcbPos + 4 > cb)
            return E_FAIL;

        cch = *(const unsigned int*)(pb + *pcbPos);
        if (!cch)
            return E_INVALIDARG;

        *pcbPos += 4;

        cbStride = 8u * ((2u * cch + 7u) >> 3);
        if (*pcbPos + (int)cbStride > cb)
            return E_FAIL;

        if (rg[i])
        {
            rg[i]->Set(L"");
            rg[i]->Append((const WCHAR*)(pb + *pcbPos), (int)cch - 1);
        }

        *pcbPos += (int)cbStride;

        if (*pcbPos >= cb)
            return S_OK;
    }

    return S_OK;
}

HRESULT CVSUnpack::LoadClassDataMap(const WCHAR* pszColor, const WCHAR* pszSize,
                                    IParserCallBack* pfnCB)
{
    const void* pvVMap;
    int cbVMap;
    int cbPos = 0;
    bool fHit = false;
    HRESULT hr;

    const void* pvClassData = NULL;
    int cbClassData = 0;

    hr = _AcquireMap(c_szVMAP, &pvVMap, &cbVMap);
    if (FAILED(hr))
        return hr;

    while (!fHit && cbPos < cbVMap)
    {
        CStringW strVariant, strSize, strColor;

        hr = ReadVSVariant((const BYTE*)pvVMap, cbVMap, &cbPos,
                           &strVariant, &strSize, &strColor);
        if (FAILED(hr))
            break;

        if ((!pszColor || strColor.EqualsI(pszColor)) &&
            (!pszSize || strSize.EqualsI(pszSize)))
        {
            fHit = true;
            m_strVariant = strVariant;
            m_strSize = strSize;
            m_strColor = strColor;

            {
                DWORD cb = 0;
                hr = m_pStyle->GetResource(L"VARIANT", strVariant.Get(),
                                           &pvClassData, &cb);
                cbClassData = (int)cb;
            }
        }
    }

    if (!fHit)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    if (FAILED(hr))
        return hr;

    {
        int iClass = -1;
        int iPart = 0;
        int iState = 0;
        const VSRECORD* pRec = (const VSRECORD*)pvClassData;

        cbPos = 0;
        m_iCurClass = -1;
        m_iCurPart = 0;
        m_iCurState = 0;

        while (pRec && SUCCEEDED(hr))
        {
            bool fNewClass = (iClass != pRec->iClass);
            bool fNewPart = (iPart != pRec->iPart);

            if (fNewClass || fNewPart || iState != pRec->iState)
            {
                hr = _FlushDelayedRecords(pfnCB);
                if (FAILED(hr))
                    break;

                if (pRec->iClass < 0 || pRec->iClass >= m_rgClassNames.GetCount())
                {
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    break;
                }

                iPart = pRec->iPart;
                iState = pRec->iState;
                iClass = pRec->iClass;
            }

            m_iCurClass = iClass;
            m_iCurPart = iPart;
            m_iCurState = iState;

            if (_DelayRecord(pRec))
                hr = _SaveRecord(pRec);
            else
                hr = _AddVSDataRecord(pfnCB, pRec);

            pRec = GetNextVSRecord(pRec, cbClassData, &cbPos);
        }

        if (SUCCEEDED(hr))
            hr = _FlushDelayedRecords(pfnCB);
    }

    return hr;
}
