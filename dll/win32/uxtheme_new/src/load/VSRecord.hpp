/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Property record layout
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "../model/Value.hpp"

#include <pshpack1.h>
typedef struct _VSRECORD
{
    int  lSymbolVal;
    int  lType;
    int  iClass;
    int  iPart;
    int  iState;
    UINT uResID;
    int  lReserved;
    int  cbData;
} VSRECORD;
#include <poppack.h>

C_ASSERT(sizeof(VSRECORD) == 32);

inline const BYTE* VSRecordInlineData(const VSRECORD* pRec)
{
    return (const BYTE*)(pRec + 1);
}

inline const VSRECORD* GetNextVSRecord(const VSRECORD* pRec, int cbBuf, int* pcbPos)
{
    int cbData = pRec->uResID ? 0 : pRec->cbData;
    unsigned int cbStride = 8u * ((unsigned int)(cbData + 39) >> 3);
    int cbNext = (int)cbStride + *pcbPos;

    if (cbNext >= cbBuf)
        return NULL;

    *pcbPos = cbNext;
    return (const VSRECORD*)((const BYTE*)pRec + cbStride);
}

typedef struct _PROPERTYMAP
{
    int lPrimVal;
    int lSymbolVal;
    int lType;
    int cbType;
} PROPERTYMAP;

extern const PROPERTYMAP g_property_map[24];

TPID GetThemePrimitiveID(int lSymbolVal, int lType);
