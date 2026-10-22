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


static CUxImage* LoadPartImage(
    _In_ CThemeFile* pFile,
    _In_ int iClass,
    _In_ int iPartId,
    _In_ int iPropertyId)
{
    const CThemeProperty* p =
        pFile->ResolveProperty(iClass, iPartId, 0, iPropertyId);
    const VSIMAGEREF* pRef;

    if (!p || p->value.GetDataSize() < (int)sizeof(VSIMAGEREF))
        return NULL;

    pRef = (const VSIMAGEREF*)p->value.GetData();
    if (!pRef->pvBits)
        return NULL;

    return pFile->images.GetOrDecode(p->value.GetResourceId(),
                                        pRef->pvBits, pRef->cbBits);
}
static bool TryLoadPartImage(
    _In_ CThemeFile* pFile,
    _In_ int iClass,
    _In_ int iPartId,
    _In_ int iPropertyId,
    _Out_ CUxImage** ppImage)
{
    CUxImage* pImage = NULL;
    *ppImage = NULL;

    /*
    if (!ppImage)
        return false; //E_POINTER;
    */

    pImage = LoadPartImage(pFile, iClass, iPartId, iPropertyId);
    if (pImage == nullptr)
        return false; //HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    /*
    */

    *ppImage = pImage;
    return true;
}


static bool TryLoadNumberedPartImage(
    _In_ CThemeFile* pFile,
    _In_ int iClass,
    _In_ int iPartId,
    _Out_ CUxImage** ppImage)
{
    CUxImage* pImage = NULL;
    *ppImage = NULL;

    /*
    if (!ppImage)
        return false; //E_POINTER;
    */

    if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE1, &pImage))
        goto success;
    else if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE2, &pImage))
        goto success;
    else if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE3, &pImage))
        goto success;
    else if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE4, &pImage))
        goto success;
    else if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE5, &pImage))
        goto success;

    if (!pImage)
    {
        *ppImage = NULL;
        return false; //HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    success:
    *ppImage = pImage;
    return true;
}

static bool TryLoadPartBackgroundImage(
    _In_ CThemeFile* pFile,
    _In_ int iClass,
    _In_ int iPartId,
    _Out_ CUxImage** ppImage)
{
    CUxImage* pImage = NULL;
    *ppImage = NULL;

    /*
    if (!ppImage)
        return false; //E_POINTER;
    */

    if (TryLoadPartImage(pFile, iClass, iPartId, TMT_IMAGEFILE, &pImage))
        goto success;
    else if (TryLoadNumberedPartImage(pFile, iClass, iPartId, &pImage))
        goto success;

    if (!pImage)
    {
        *ppImage = NULL;
        return false; //HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    success:
    *ppImage = pImage;
    return true;
}


static bool TryLoadPartGlyphImage(
    _In_ CThemeFile* pFile,
    _In_ int iClass,
    _In_ int iPartId,
    _Out_ CUxImage** ppImage)
{
    CUxImage* pImage = NULL;
    *ppImage = NULL;

    /*
    if (!ppImage)
        return false; //E_POINTER;
    */

    if (TryLoadPartImage(pFile, iClass, iPartId, TMT_GLYPHIMAGEFILE, &pImage))
        goto success;
    else if (TryLoadNumberedPartImage(pFile, iClass, iPartId, &pImage))
        goto success;

    if (!pImage)
    {
        *ppImage = NULL;
        return false; //HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    success:
    *ppImage = pImage;
    return true;
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


static void UxAdjustRect(
    _Inout_ RECT* prc,
    _In_ bool fHaveAtlasRect,
    _In_ IMAGELAYOUT eImageLayout,
    _In_ int iImageCount,
    _In_ int iStateId,
    _In_ CUxImage* pImage,
    _In_ RECT* prcAtlas)
{
    /*
    if (!pImage)
        return E_POINTER;
    else if (!prcAtlas)
        return E_POINTER;
    */

    int width = pImage->Width();
    int height = pImage->Height();

    if (fHaveAtlasRect)
    {
        prc->left = prcAtlas->left;
        prc->top = prcAtlas->top;
        prc->right = prcAtlas->right;
        prc->bottom = prcAtlas->bottom;

        if (prc->right > width)
            prc->right = width;
        if (prc->bottom > height)
            prc->bottom = height;
        if (prc->left < 0)
            prc->left = 0;
        if (prc->top < 0)
            prc->top = 0;
    }
    else
    {
        prc->left = 0;
        prc->top = 0;
        prc->right = width;
        prc->bottom = height;
    }

    if (iImageCount > 1)
    {
        int cxStrip = prc->right - prc->left;
        int cyStrip = prc->bottom - prc->top;

        if (eImageLayout == IL_HORIZONTAL)
        {
            int cxSingle = cxStrip / iImageCount;
            int iOffset = (iStateId <= 0 || iStateId > iImageCount)
                          ? 0 : cxSingle * (iStateId - 1);
            prc->left += iOffset;
            prc->right = prc->left + cxSingle;
        }
        else
        {
            int cySingle = cyStrip / iImageCount;
            int iOffset = (iStateId <= 0 || iStateId > iImageCount)
                          ? 0 : cySingle * (iStateId - 1);
            prc->top += iOffset;
            prc->bottom = prc->top + cySingle;
        }
    }
    //return S_OK;
}
HRESULT UxResolveDrawInfo(CThemeFile* pFile, int iClass, int iPartId, int iStateId,
                          UXDRAWINFO* pInfo, CUxImage** ppBackground, CUxImage** ppGlyph)
{
    const CThemeProperty* pProp;
    CUxImage* pBackground = NULL;
    CUxImage* pGlyph = NULL;
    RECT rcAtlas;
    BOOL fHaveAtlasRect = FALSE;

    if (!pFile || !pInfo || !ppBackground)
        return E_INVALIDARG;

    memset(pInfo, 0, sizeof(*pInfo));
    *ppBackground = NULL;
    if (ppGlyph != nullptr)
        *ppGlyph = NULL;

    pInfo->eBgType = (BGTYPE)GetInt(pFile, iClass, iPartId, iStateId, TMT_BGTYPE, BT_BORDERFILL);
    pInfo->eSizingType = (SIZINGTYPE)GetInt(pFile, iClass, iPartId, iStateId, TMT_SIZINGTYPE, ST_STRETCH);
    pInfo->eImageLayout = (IMAGELAYOUT)GetInt(pFile, iClass, iPartId, iStateId, TMT_IMAGELAYOUT, IL_HORIZONTAL);
    pInfo->eGlyphType = (GLYPHTYPE)GetInt(pFile, iClass, iPartId, iStateId, TMT_GLYPHTYPE, GT_NONE);
    pInfo->eGlyphHAlignment = (HALIGN)GetInt(pFile, iClass, iPartId, iStateId, TMT_HALIGN, HA_CENTER);
    pInfo->eGlyphVAlignment = (VALIGN)GetInt(pFile, iClass, iPartId, iStateId, TMT_VALIGN, VA_CENTER);
    pInfo->iImageCount = GetInt(pFile, iClass, iPartId, iStateId, TMT_IMAGECOUNT, 1);

    pInfo->fUniformSizing = GetBool(pFile, iClass, iPartId, iStateId, TMT_UNIFORMSIZING, FALSE);
    pInfo->fMirrorImage = GetBool(pFile, iClass, iPartId, iStateId, TMT_MIRRORIMAGE, FALSE);
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
        pInfo->eBorderType = (BORDERTYPE)GetInt(pFile, iClass, iPartId, iStateId, TMT_BORDERTYPE, BT_RECT);
        pInfo->eFillType   = (FILLTYPE)GetInt(pFile, iClass, iPartId, iStateId, TMT_FILLTYPE, FT_SOLID);
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


    if (!TryLoadPartBackgroundImage(pFile, iClass, iPartId, &pBackground))
    {
        if (fHaveAtlasRect)
            pBackground = LoadClassAtlas(pFile, iClass, iPartId);
    }

    if (!pBackground)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    UxAdjustRect(&pInfo->rcSrc, fHaveAtlasRect, pInfo->eImageLayout, pInfo->iImageCount, iStateId, pBackground, &rcAtlas);

    if (IsRectEmpty(&pInfo->rcSrc))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    *ppBackground = pBackground;



    if ((ppGlyph != NULL) && (!fHaveAtlasRect) && TryLoadPartGlyphImage(pFile, iClass, iPartId, &pGlyph))
    {
        *ppGlyph = pGlyph;
        UxAdjustRect(&pInfo->rcGlyphSrc, false, pInfo->eImageLayout, pInfo->iImageCount, iStateId, pGlyph, &rcAtlas);
    }


    return S_OK;
}
