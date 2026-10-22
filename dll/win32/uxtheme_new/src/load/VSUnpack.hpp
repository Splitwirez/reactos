/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Property record stream walker
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "PeModule.hpp"
#include "Primitives.hpp"
#include "VSRecord.hpp"

class IParserCallBack
{
public:
    virtual HRESULT AddData(int iClass, int iPart, int iState,
                            int lSymbolVal, int lType, UINT uResID,
                            const void* pvData, int cbData) = 0;

    virtual HRESULT AddBaseClass(int idClass, int idBaseClass) = 0;

    virtual HRESULT AddVariant(const WCHAR* pszName, const WCHAR* pszSize,
                               const WCHAR* pszColor) = 0;
};

class CVSUnpack
{
public:
    CVSUnpack();
    ~CVSUnpack();

    HRESULT Initialize(const CPeModule* pStyle, const CPeModule* pMui);

    HRESULT LoadRootMap(IParserCallBack* pfnCB);

    HRESULT LoadClassDataMap(const WCHAR* pszColor, const WCHAR* pszSize,
                             IParserCallBack* pfnCB);

    HRESULT LoadBaseClassDataMap(IParserCallBack* pfnCB);

    HRESULT EnumVariants(IParserCallBack* pfnCB);

    int          GetClassCount() const { return m_rgClassNames.GetCount(); }
    const WCHAR* GetClassName(int i) const { return m_rgClassNames[i].Get(); }

    const WCHAR* GetVariantName() const { return m_strVariant.Get(); }
    const WCHAR* GetVariantSize() const { return m_strSize.Get(); }
    const WCHAR* GetVariantColor() const { return m_strColor.Get(); }

private:
    CVSUnpack(const CVSUnpack&);
    CVSUnpack& operator=(const CVSUnpack&);

    HRESULT _AcquireMap(const WCHAR* pszMap, const void** ppvMap, int* pcbMap);
    int     _FindClass(const WCHAR* pszClass) const;

    HRESULT _AddVSDataRecord(IParserCallBack* pfnCB, const VSRECORD* pRec);
    HRESULT _FixSymbolAndAddVSDataRecord(IParserCallBack* pfnCB,
                                         const VSRECORD* pRec, int lSymbolVal);

    static bool _DelayRecord(const VSRECORD* pRec);
    HRESULT _SaveRecord(const VSRECORD* pRec);
    HRESULT _FlushDelayedRecords(IParserCallBack* pfnCB);
    void    _ClearDpiRecords();

    int m_iCurClass;
    int m_iCurPart;
    int m_iCurState;

    VSLOADCTX m_ctx;
    const CPeModule* m_pStyle;

    CArray<CStringW> m_rgClassNames;

    CStringW m_strVariant;
    CStringW m_strSize;
    CStringW m_strColor;

    const VSRECORD* m_rgImageDpiRec[5];
    const VSRECORD* m_rgImageRec[5];
};

int GetScreenDpi(void);
