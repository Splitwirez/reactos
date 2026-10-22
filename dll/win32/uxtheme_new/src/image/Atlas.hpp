/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Part and state image resolution
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "../model/ThemeFile.hpp"
#include "Decode.hpp"

struct UXDRAWINFO
{
    BGTYPE eBgType;
    SIZINGTYPE eSizingType;
    IMAGELAYOUT eImageLayout;
    GLYPHTYPE eGlyphType;
    HALIGN eGlyphHAlignment;
    VALIGN eGlyphVAlignment;
    int iImageCount;

    MARGINS     marSizing;
    MARGINS     marContent;
    BOOL        fSizingMargins;
    BOOL        fUniformSizing;
    BOOL        fMirrorImage;

    RECT        rcSrc;
    RECT        rcGlyphSrc;

    BOOL        fTransparent;
    COLORREF    crTransparent;

    BORDERTYPE  eBorderType;
    COLORREF    crBorder;
    int         iBorderSize;

    FILLTYPE    eFillType;
    COLORREF    crFill;
};

HRESULT UxResolveDrawInfo(CThemeFile* pFile, int iClass, int iPartId, int iStateId,
                          UXDRAWINFO* pInfo, CUxImage** ppImage, CUxImage** ppGlyphImage);
