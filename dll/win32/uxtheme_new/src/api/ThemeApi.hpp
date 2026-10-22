/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme handle definitions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "../model/ThemeFile.hpp"

#define THEMEHANDLE_MAGIC 0x554E5834

struct THEMEHANDLE
{
    DWORD       dwMagic;
    CThemeFile* pFile;
    int         iClass;
    HWND        hwnd;
};

THEMEHANDLE* ValidateThemeHandle(HTHEME hTheme);

CThemeFile* GetProcessThemeFile(void);
void        ReleaseProcessThemeFile(void);

HRESULT GetCurrentThemeSettings(BOOL* pfActive, WCHAR* pszDll, int cchDll,
                                WCHAR* pszColor, int cchColor,
                                WCHAR* pszSize, int cchSize);
