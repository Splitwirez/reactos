/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Builds the theme object model
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "../model/ThemeFile.hpp"
#include "PeModule.hpp"
#include "VSUnpack.hpp"

class CThemeLoader : public IParserCallBack
{
public:
    CThemeLoader();
    ~CThemeLoader();

    static HRESULT LoadTheme(const WCHAR* pszPath, const WCHAR* pszColor,
                             const WCHAR* pszSize, CThemeFile** ppFile);

    virtual HRESULT AddData(int iClass, int iPart, int iState,
                            int lSymbolVal, int lType, UINT uResID,
                            const void* pvData, int cbData);
    virtual HRESULT AddBaseClass(int idClass, int idBaseClass);
    virtual HRESULT AddVariant(const WCHAR* pszName, const WCHAR* pszSize,
                               const WCHAR* pszColor);

private:
    CThemeLoader(const CThemeLoader&);
    CThemeLoader& operator=(const CThemeLoader&);

    HRESULT Load(const WCHAR* pszPath, const WCHAR* pszColor, const WCHAR* pszSize);

    CThemeFile* m_pFile;
    CVSUnpack*  m_pUnpack;

    bool m_fLoadingRoot;
};

struct THEMEMODULES
{
    CPeModule style;
    CPeModule mui;
};

HRESULT DeriveMuiPath(const WCHAR* pszStylePath, WCHAR* pszOut, int cchOut);
