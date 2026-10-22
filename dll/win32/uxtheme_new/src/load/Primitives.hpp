/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Property record loaders
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "PeModule.hpp"
#include "VSRecord.hpp"

struct VSLOADCTX
{
    const CPeModule* pStyle;
    const CPeModule* pMui;
};

HRESULT LoadVSRecordData(const VSLOADCTX* pCtx, const VSRECORD* pRec,
                         void* pvBuf, int* pcbBuf);

typedef struct _VSIMAGEREF
{
    const void* pvBits;
    UINT        cbBits;
    UINT        uReserved;
} VSIMAGEREF;

HRESULT ParseIntegerToken(const WCHAR** ppsz, const WCHAR* pszDelim, int* pnValue);

HRESULT ParseIntegerTokenList(const WCHAR** ppsz, int* prgVal, int* pcVal);

HRESULT ParseBool(const WCHAR* pszValue, int* pf);
