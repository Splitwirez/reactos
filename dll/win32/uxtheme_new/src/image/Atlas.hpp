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
    int eBgType;
    int eSizingType;
    int eImageLayout;
    int iImageCount;

    MARGINS marSizing;
    MARGINS marContent;
    BOOL    fSizingMargins;

    RECT rcSrc;

    BOOL     fTransparent;
    COLORREF crTransparent;

    int      eBorderType;
    int      eFillType;
    int      iBorderSize;
    COLORREF crBorder;
    COLORREF crFill;
};

HRESULT UxResolveDrawInfo(CThemeFile* pFile, int iClass, int iPartId, int iStateId,
                          UXDRAWINFO* pInfo, CUxImage** ppImage);
