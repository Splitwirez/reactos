/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Part and state image resolution
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "Atlas.hpp"
#include "../load/Primitives.hpp"
#include "../load/VSRecord.hpp"
#include "../model/Tmt.hpp"

static int GetInt(CThemeFile* pFile, int iClass, int iPart, int iState,
                  int sym, int nDefault)
{
    const CThemeProperty* p = pFile->ResolveProperty(iClass, iPart, iState, sym);
    return p ? p->value.GetInt() : nDefault;
}

static BOOL GetBool(CThemeFile* pFile, int iClass, int iPart, int iState,
                    int sym, BOOL fDefault)
{
    const CThemeProperty* p = pFile->ResolveProperty(iClass, iPart, iState, sym);
    return p ? p->value.GetBool() : fDefault;
}

static COLORREF GetColor(CThemeFile* pFile, int iClass, int iPart, int iState,
                         int sym, COLORREF crDefault)
{
    const CThemeProperty* p = pFile->ResolveProperty(iClass, iPart, iState, sym);
    return p ? p->value.GetColor() : crDefault;
}

static CUxImage* LoadPartImage(CThemeFile* pFile, int iClass, int iPartId)
{
    static const int rgSyms[] = {
        TMT_IMAGEFILE1, TMT_IMAGEFILE2, TMT_IMAGEFILE3,
        TMT_IMAGEFILE4, TMT_IMAGEFILE5, TMT_IMAGEFILE,
    };
    int i;

    for (i = 0; i < (int)ARRAYSIZE(rgSyms); i++)
    {
        const CThemeProperty* p =
            pFile->ResolveProperty(iClass, iPartId, 0, rgSyms[i]);
        const VSIMAGEREF* pRef;

        if (!p || p->value.GetDataSize() < (int)sizeof(VSIMAGEREF))
            continue;

        pRef = (const VSIMAGEREF*)p->value.GetData();
        if (!pRef->pvBits)
            continue;

        return pFile->images.GetOrDecode(p->value.GetResourceId(),
                                         pRef->pvBits, pRef->cbBits);
    }

    return NULL;
}

static CUxImage* LoadClassAtlas(CThemeFile* pFile, int iClass, int iPartId)
{
    const CThemeProperty* p =
        pFile->ResolveProperty(iClass, iPartId, 0, TMT_DISKSTREAM);
    const void* pv;
    DWORD cb;

    if (!p || !p->value.GetResourceId())
        return NULL;

    if (FAILED(pFile->style.GetResource(L"STREAM",
                                        (UINT)LOWORD(p->value.GetResourceId()),
                                        &pv, &cb)))
        return NULL;

    return pFile->images.GetOrDecode(p->value.GetResourceId(), pv, cb);
}

HRESULT UxResolveDrawInfo(CThemeFile* pFile, int iClass, int iPartId, int iStateId,
                          UXDRAWINFO* pInfo, CUxImage** ppImage)
{
    const CThemeProperty* pProp;
    CUxImage* pImage = NULL;
    RECT rcAtlas;
    BOOL fHaveAtlasRect = FALSE;

    if (!pFile || !pInfo || !ppImage)
        return E_INVALIDARG;

    memset(pInfo, 0, sizeof(*pInfo));
    *ppImage = NULL;

    pInfo->eBgType = GetInt(pFile, iClass, iPartId, iStateId, TMT_BGTYPE, BT_BORDERFILL);
    pInfo->eSizingType = GetInt(pFile, iClass, iPartId, iStateId, TMT_SIZINGTYPE, ST_STRETCH);
    pInfo->eImageLayout = GetInt(pFile, iClass, iPartId, iStateId, TMT_IMAGELAYOUT, IL_HORIZONTAL);
    pInfo->iImageCount = GetInt(pFile, iClass, iPartId, iStateId, TMT_IMAGECOUNT, 1);

    pInfo->fTransparent = GetBool(pFile, iClass, iPartId, iStateId, TMT_TRANSPARENT, FALSE);
    pInfo->crTransparent = GetColor(pFile, iClass, iPartId, iStateId,
                                    TMT_TRANSPARENTCOLOR, RGB(255, 0, 255));

    pProp = pFile->ResolveProperty(iClass, iPartId, iStateId, TMT_SIZINGMARGINS);
    if (pProp)
    {
        pInfo->marSizing = pProp->value.GetMargins();
        pInfo->fSizingMargins = TRUE;
    }

    pProp = pFile->ResolveProperty(iClass, iPartId, iStateId, TMT_CONTENTMARGINS);
    if (pProp)
        pInfo->marContent = pProp->value.GetMargins();

    if (pInfo->eBgType == BT_BORDERFILL)
    {
        pInfo->eBorderType = GetInt(pFile, iClass, iPartId, iStateId, TMT_BORDERTYPE, BT_RECT);
        pInfo->eFillType   = GetInt(pFile, iClass, iPartId, iStateId, TMT_FILLTYPE, FT_SOLID);
        pInfo->iBorderSize = GetInt(pFile, iClass, iPartId, iStateId, TMT_BORDERSIZE, 1);
        pInfo->crBorder    = GetColor(pFile, iClass, iPartId, iStateId,
                                      TMT_BORDERCOLOR, RGB(0, 0, 0));
        pInfo->crFill      = GetColor(pFile, iClass, iPartId, iStateId,
                                      TMT_FILLCOLOR, RGB(255, 255, 255));
        return S_OK;
    }

    if (pInfo->eBgType == BT_NONE)
        return S_OK;

    pProp = pFile->ResolveProperty(iClass, iPartId, 0, TMT_ATLASRECT);
    if (pProp)
    {
        rcAtlas = pProp->value.GetRect();
        fHaveAtlasRect = !IsRectEmpty(&rcAtlas);
    }

    pImage = LoadPartImage(pFile, iClass, iPartId);
    if (!pImage && fHaveAtlasRect)
        pImage = LoadClassAtlas(pFile, iClass, iPartId);

    if (!pImage)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (fHaveAtlasRect)
    {
        pInfo->rcSrc = rcAtlas;

        if (pInfo->rcSrc.right > pImage->Width())
            pInfo->rcSrc.right = pImage->Width();
        if (pInfo->rcSrc.bottom > pImage->Height())
            pInfo->rcSrc.bottom = pImage->Height();
        if (pInfo->rcSrc.left < 0)
            pInfo->rcSrc.left = 0;
        if (pInfo->rcSrc.top < 0)
            pInfo->rcSrc.top = 0;
    }
    else
    {
        pInfo->rcSrc.left = 0;
        pInfo->rcSrc.top = 0;
        pInfo->rcSrc.right = pImage->Width();
        pInfo->rcSrc.bottom = pImage->Height();
    }

    if (pInfo->iImageCount > 1)
    {
        int cxStrip = pInfo->rcSrc.right - pInfo->rcSrc.left;
        int cyStrip = pInfo->rcSrc.bottom - pInfo->rcSrc.top;

        if (pInfo->eImageLayout == IL_HORIZONTAL)
        {
            int cxSingle = cxStrip / pInfo->iImageCount;
            int iOffset = (iStateId <= 0 || iStateId > pInfo->iImageCount)
                          ? 0 : cxSingle * (iStateId - 1);
            pInfo->rcSrc.left += iOffset;
            pInfo->rcSrc.right = pInfo->rcSrc.left + cxSingle;
        }
        else
        {
            int cySingle = cyStrip / pInfo->iImageCount;
            int iOffset = (iStateId <= 0 || iStateId > pInfo->iImageCount)
                          ? 0 : cySingle * (iStateId - 1);
            pInfo->rcSrc.top += iOffset;
            pInfo->rcSrc.bottom = pInfo->rcSrc.top + cySingle;
        }
    }

    if (IsRectEmpty(&pInfo->rcSrc))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    *ppImage = pImage;
    return S_OK;
}
