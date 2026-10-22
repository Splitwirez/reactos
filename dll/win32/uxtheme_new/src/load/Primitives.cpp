/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Property record loaders
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "Primitives.hpp"
#include "../model/Tmt.hpp"

const PROPERTYMAP g_property_map[24] =
{
    { TPID_BITMAPIMAGE,      TMT_IMAGEFILE,       TMT_FILENAME,   16 },
    { TPID_BITMAPIMAGE1,     TMT_IMAGEFILE1,      TMT_FILENAME,   16 },
    { TPID_BITMAPIMAGE2,     TMT_IMAGEFILE2,      TMT_FILENAME,   16 },
    { TPID_BITMAPIMAGE3,     TMT_IMAGEFILE3,      TMT_FILENAME,   16 },
    { TPID_BITMAPIMAGE4,     TMT_IMAGEFILE4,      TMT_FILENAME,   16 },
    { TPID_BITMAPIMAGE5,     TMT_IMAGEFILE5,      TMT_FILENAME,   16 },
    { TPID_STOCKBITMAPIMAGE, TMT_STOCKIMAGEFILE,  TMT_FILENAME,   16 },
    { TPID_GLYPHIMAGE,       TMT_GLYPHIMAGEFILE,  TMT_FILENAME,   16 },
    { TPID_ATLASIMAGE,       TMT_ATLASIMAGE,      TMT_DISKSTREAM, -1 },
    { TPID_ATLASINPUTIMAGE,  TMT_ATLASINPUTIMAGE, TMT_STRING,     -1 },
    { TPID_ENUM,             0,                   TMT_ENUM,        4 },
    { TPID_STRING,           0,                   TMT_STRING,     -1 },
    { TPID_INT,              0,                   TMT_INT,         4 },
    { TPID_BOOL,             0,                   TMT_BOOL,        4 },
    { TPID_COLOR,            0,                   TMT_COLOR,       4 },
    { TPID_MARGINS,          0,                   TMT_MARGINS,    16 },
    { TPID_FILENAME,         0,                   TMT_FILENAME,   -1 },
    { TPID_SIZE,             0,                   TMT_SIZE,        4 },
    { TPID_POSITION,         0,                   TMT_POSITION,    8 },
    { TPID_RECT,             0,                   TMT_RECT,       16 },
    { TPID_FONT,             0,                   TMT_FONT,       92 },
    { TPID_INTLIST,          0,                   TMT_INTLIST,    -1 },
    { TPID_DISKSTREAM,       0,                   TMT_DISKSTREAM, -1 },
    { TPID_STREAM,           0,                   TMT_STREAM,     -1 },
};

TPID GetThemePrimitiveID(int lSymbolVal, int lType)
{
    int iCandidate = -1;
    int i;

    for (i = 0; i < 24; i++)
    {
        if (g_property_map[i].lType == lType)
        {
            iCandidate = i;
            if (g_property_map[i].lSymbolVal != 0 &&
                g_property_map[i].lSymbolVal == lSymbolVal)
            {
                return (TPID)g_property_map[i].lPrimVal;
            }
        }
    }

    if (iCandidate < 0)
        return TPID_INVALID;

    return (TPID)g_property_map[iCandidate].lPrimVal;
}

static const PROPERTYMAP* FindPropertyMapRow(TPID id)
{
    for (int i = 0; i < 24; i++)
        if (g_property_map[i].lPrimVal == (int)id)
            return &g_property_map[i];
    return NULL;
}

static bool IsSpaceW(WCHAR c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; }
static bool IsDigitW(WCHAR c) { return c >= L'0' && c <= L'9'; }

HRESULT ParseIntegerToken(const WCHAR** ppsz, const WCHAR* pszDelim, int* pnValue)
{
    const WCHAR* p = *ppsz;
    int nValue = 0;
    int nSign = 1;

    if (!p)
    {
        *pnValue = 0;
        return S_OK;
    }

    while (*p && IsSpaceW(*p))
        p++;

    if (!*p)
    {
        *ppsz = p;
        *pnValue = 0;
        return S_OK;
    }

    if (!IsDigitW(*p) && *p != L'-' && *p != L'+')
        return E_FAIL;

    if (*p == L'-')
    {
        nSign = -1;
        p++;
    }
    else if (*p == L'+')
    {
        p++;
    }

    while (IsDigitW(*p))
        nValue = nValue * 10 + (*p++ - L'0');

    *pnValue = nValue * nSign;

    while (*p && !wcschr(pszDelim, *p))
        p++;
    while (*p && wcschr(pszDelim, *p))
        p++;

    *ppsz = p;
    return S_OK;
}

HRESULT ParseIntegerTokenList(const WCHAR** ppsz, int* prgVal, int* pcVal)
{
    int cMax = *pcVal;
    int cOut = 0;
    HRESULT hr = S_OK;

    if (cMax == 0 && prgVal)
        return E_INVALIDARG;

    while (*ppsz && **ppsz)
    {
        int nValue;

        if (prgVal && cOut >= cMax)
            break;

        hr = ParseIntegerToken(ppsz, L", ", &nValue);
        if (FAILED(hr))
            break;

        if (prgVal)
            prgVal[cOut] = nValue;
        cOut++;
    }

    *pcVal = cOut;
    return hr;
}

HRESULT ParseBool(const WCHAR* pszValue, int* pf)
{
    if (_wcsicmp(pszValue, L"true") == 0 || _wcsicmp(pszValue, L"1") == 0)
    {
        *pf = 1;
        return S_OK;
    }
    if (_wcsicmp(pszValue, L"false") == 0 || _wcsicmp(pszValue, L"0") == 0)
    {
        *pf = 0;
        return S_OK;
    }
    return E_INVALIDARG;
}

static HRESULT ParseStringToken(const WCHAR** ppsz, const WCHAR* pszDelim,
                                WCHAR* pszOut, int cchOut)
{
    const WCHAR* p = *ppsz;
    int cch = 0;

    if (!p || !*p)
        return E_FAIL;

    while (*p && IsSpaceW(*p))
        p++;

    while (*p && (pszDelim ? (wcschr(pszDelim, *p) == NULL) : (*p != L',')))
    {
        if (cch < cchOut - 1)
            pszOut[cch++] = *p;
        p++;
    }

    pszOut[cch] = L'\0';

    while (*p && (pszDelim ? (wcschr(pszDelim, *p) != NULL) : (*p == L',')))
        p++;

    *ppsz = p;
    return (cch > 0) ? S_OK : E_FAIL;
}

static HRESULT ParseFont(const WCHAR* pszValue, LOGFONTW* plf)
{
    WCHAR szToken[64];
    int lVal;
    HRESULT hr;

    memset(plf, 0, sizeof(*plf));
    plf->lfWeight = FW_NORMAL;
    plf->lfCharSet = DEFAULT_CHARSET;

    hr = ParseStringToken(&pszValue, NULL, szToken, ARRAYSIZE(szToken));
    if (FAILED(hr))
        return hr;

    StringCchCopyNW(plf->lfFaceName, LF_FACESIZE, szToken, LF_FACESIZE - 1);

    if (SUCCEEDED(ParseIntegerToken(&pszValue, L",", &lVal)))
        plf->lfHeight = -MulDiv(lVal, 96, 72);

    while (SUCCEEDED(ParseStringToken(&pszValue, L" ,", szToken, ARRAYSIZE(szToken))))
    {
        if (_wcsicmp(szToken, L"bold") == 0)
            plf->lfWeight = FW_BOLD;
        else if (_wcsicmp(szToken, L"italic") == 0)
            plf->lfItalic = TRUE;
        else if (_wcsicmp(szToken, L"underline") == 0)
            plf->lfUnderline = TRUE;
        else if (_wcsicmp(szToken, L"strikeout") == 0)
            plf->lfStrikeOut = TRUE;
        else if (_wcsicmp(szToken, L"quality:cleartype") == 0)
            plf->lfQuality = CLEARTYPE_QUALITY;
        else if (_wcsicmp(szToken, L"quality:cleartype-natural") == 0)
            plf->lfQuality = CLEARTYPE_NATURAL_QUALITY;
        else if (_wcsicmp(szToken, L"quality:antialiased") == 0)
            plf->lfQuality = ANTIALIASED_QUALITY;
        else if (_wcsicmp(szToken, L"quality:nonantialiased") == 0)
            plf->lfQuality = NONANTIALIASED_QUALITY;
        else
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return hr;
}

static HRESULT LoadStringResW(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                              WCHAR* pszData, int cchData)
{
    const WCHAR* psz;
    int cch;
    HRESULT hr;

    if (cchData <= 0)
        return E_INVALIDARG;

    *pszData = L'\0';

    hr = pCtx->pStyle->GetStringResource(pRec->uResID, &psz, &cch);
    if (FAILED(hr) && pCtx->pMui)
        hr = pCtx->pMui->GetStringResource(pRec->uResID, &psz, &cch);
    if (FAILED(hr))
        return hr;

    if (cch > cchData - 1)
        cch = cchData - 1;

    memcpy(pszData, psz, (SIZE_T)cch * sizeof(WCHAR));
    pszData[cch] = L'\0';
    return S_OK;
}

static HRESULT LoadImageFileRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                                void* pvData, int* pcbData)
{
    const void* pv;
    DWORD cb;
    HRESULT hr;
    VSIMAGEREF* pRef = (VSIMAGEREF*)pvData;

    if (!pcbData || *pcbData < (int)sizeof(VSIMAGEREF))
        return E_INVALIDARG;

    hr = pCtx->pStyle->GetResource(L"IMAGE", (UINT)LOWORD(pRec->uResID), &pv, &cb);
    if (FAILED(hr))
    {
        *pcbData = 0;
        return hr;
    }

    if (cb >= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) &&
        *(const WORD*)pv == 0x4D42 )
    {
        WORD wBitCount = *((const WORD*)pv + 14);
        if (wBitCount == 32 || wBitCount == 24)
        {
            pRef->pvBits = (const BYTE*)pv + sizeof(BITMAPFILEHEADER);
            pRef->cbBits = 0;
            pRef->uReserved = 0;
            *pcbData = (int)sizeof(VSIMAGEREF);
            return S_OK;
        }
    }

    pRef->pvBits = pv;
    pRef->cbBits = cb;
    pRef->uReserved = 0;
    *pcbData = (int)sizeof(VSIMAGEREF);
    return S_OK;
}

static HRESULT LoadStreamRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                             void* pvData, int* pcbData)
{
    const void* pv;
    DWORD cb;
    HRESULT hr;

    hr = pCtx->pStyle->GetResource(L"STREAM", (UINT)LOWORD(pRec->uResID), &pv, &cb);
    if (FAILED(hr))
        return hr;

    if ((DWORD)*pcbData > cb)
        *pcbData = (int)cb;

    memcpy(pvData, pv, (SIZE_T)*pcbData);
    return S_OK;
}

static HRESULT LoadDiskStreamRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                                 void* pvData, int* pcbData)
{
    HRSRC hRes;
    DWORD cb;
    ULARGE_INTEGER ulData;

    if (!pcbData || *pcbData < 8)
        return E_INVALIDARG;

    hRes = FindResourceW(pCtx->pStyle->GetInstance(),
                         MAKEINTRESOURCEW(LOWORD(pRec->uResID)), L"STREAM");
    if (!hRes)
        return MakeErrorLast();

    cb = SizeofResource(pCtx->pStyle->GetInstance(), hRes);
    if (!cb)
        return MakeErrorLast();

    ulData.LowPart = (DWORD)((const BYTE*)hRes - (const BYTE*)pCtx->pStyle->GetInstance());
    ulData.HighPart = cb;

    *pcbData = 8;
    memcpy(pvData, &ulData, 8);
    return S_OK;
}

static HRESULT LoadIntRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                          void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    const WCHAR* psz = szBuf;
    HRESULT hr;

    if (!pcbData || *pcbData < 4)
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    return ParseIntegerToken(&psz, L", ", (int*)pvData);
}

static HRESULT LoadBoolRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                           void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    HRESULT hr;

    if (!pcbData || *pcbData < 4)
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    return ParseBool(szBuf, (int*)pvData);
}

static HRESULT LoadRGBRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                          void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    const WCHAR* psz = szBuf;
    int rgVal[3];
    int cVal = 3;
    HRESULT hr;

    if (!pcbData || *pcbData < 4)
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    hr = ParseIntegerTokenList(&psz, rgVal, &cVal);
    if (FAILED(hr))
        return hr;

    *(COLORREF*)pvData = RGB((BYTE)rgVal[0], (BYTE)rgVal[1], (BYTE)rgVal[2]);
    return S_OK;
}

static HRESULT LoadMarginRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                             void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    const WCHAR* psz = szBuf;
    int rgVal[4];
    int cVal = 4;
    HRESULT hr;

    if (!pcbData || *pcbData < 16)
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    hr = ParseIntegerTokenList(&psz, rgVal, &cVal);
    if (FAILED(hr))
        return hr;

    memcpy(pvData, rgVal, sizeof(rgVal));
    return S_OK;
}

static HRESULT LoadPositionRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                               void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    const WCHAR* psz = szBuf;
    int rgVal[2];
    int cVal = 2;
    HRESULT hr;

    if (!pcbData || *pcbData < 8)
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    hr = ParseIntegerTokenList(&psz, rgVal, &cVal);
    if (FAILED(hr))
        return hr;

    memcpy(pvData, rgVal, sizeof(rgVal));
    return S_OK;
}

static HRESULT LoadFontRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                           void* pvData, int* pcbData)
{
    WCHAR szBuf[128];
    HRESULT hr;

    if (!pcbData || *pcbData < (int)sizeof(LOGFONTW))
        return E_INVALIDARG;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    return ParseFont(szBuf, (LOGFONTW*)pvData);
}

static HRESULT LoadStringResPrim(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                                 void* pvData, int* pcbData)
{
    return LoadStringResW(pCtx, pRec, (WCHAR*)pvData, *pcbData / (int)sizeof(WCHAR));
}

static HRESULT LoadIntlistRes(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                              void* pvData, int* pcbData)
{
    WCHAR szBuf[256];
    const WCHAR* psz = szBuf;
    HRESULT hr;
    int cMax;
    int cVal;

    hr = LoadStringResW(pCtx, pRec, szBuf, ARRAYSIZE(szBuf));
    if (FAILED(hr))
        return hr;

    cMax = *pcbData / (int)sizeof(int);
    cVal = cMax;

    hr = ParseIntegerTokenList(&psz, (int*)pvData, &cVal);
    if (FAILED(hr))
        return hr;

    *pcbData = cVal * (int)sizeof(int);
    return S_OK;
}

HRESULT LoadVSRecordData(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                         void* pvBuf, int* pcbBuf)
{
    TPID id;
    const PROPERTYMAP* pRow;

    if (!pRec || !pcbBuf)
        return E_INVALIDARG;

    if (!pvBuf)
    {
        *pcbBuf = pRec->cbData;
        return S_OK;
    }

    if (*pcbBuf < 0 || pRec->cbData < 0 || *pcbBuf < pRec->cbData)
        return E_INVALIDARG;

    id = GetThemePrimitiveID(pRec->lSymbolVal, pRec->lType);
    if (id == TPID_INVALID)
        return E_FAIL;

    if (!pRec->uResID)
    {
        memcpy(pvBuf, VSRecordInlineData(pRec), (SIZE_T)pRec->cbData);
        return S_OK;
    }

    pRow = FindPropertyMapRow(id);
    if (pRow && pRow->cbType > 0 && pRow->cbType != pRec->cbData)
        return E_INVALIDARG;

    switch (id)
    {
    case TPID_BITMAPIMAGE:
    case TPID_BITMAPIMAGE1:
    case TPID_BITMAPIMAGE2:
    case TPID_BITMAPIMAGE3:
    case TPID_BITMAPIMAGE4:
    case TPID_BITMAPIMAGE5:
    case TPID_STOCKBITMAPIMAGE:
    case TPID_GLYPHIMAGE:
        return LoadImageFileRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_ATLASIMAGE:
    case TPID_DISKSTREAM:
        return LoadDiskStreamRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_STREAM:
        return LoadStreamRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_ATLASINPUTIMAGE:
    case TPID_STRING:
    case TPID_FILENAME:
        return LoadStringResPrim(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_ENUM:
    case TPID_INT:
    case TPID_SIZE:
        return LoadIntRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_BOOL:
        return LoadBoolRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_COLOR:
        return LoadRGBRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_MARGINS:
    case TPID_RECT:
        return LoadMarginRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_POSITION:
        return LoadPositionRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_FONT:
        return LoadFontRes(pCtx, pRec, pvBuf, pcbBuf);

    case TPID_INTLIST:
        return LoadIntlistRes(pCtx, pRec, pvBuf, pcbBuf);

    default:
        return E_FAIL;
    }
}
