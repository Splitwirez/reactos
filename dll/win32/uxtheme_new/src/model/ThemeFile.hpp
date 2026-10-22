/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme object model
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"
#include "../load/PeModule.hpp"
#include "../image/Decode.hpp"
#include "Value.hpp"

class CThemeProperty
{
public:
    CThemeProperty() : iClass(-1), iPartId(0), iStateId(0), lSymbolVal(0), lType(0) { }

    int    iClass;
    int    iPartId;
    int    iStateId;
    int    lSymbolVal;
    int    lType;
    CValue value;

private:
    CThemeProperty(const CThemeProperty&);
    CThemeProperty& operator=(const CThemeProperty&);
};

class CThemeState
{
public:
    CThemeState() : iStateId(0) { }

    const CThemeProperty* FindProperty(int lSymbolVal) const;

    int iStateId;
    CArray<CThemeProperty> properties;

private:
    CThemeState(const CThemeState&);
    CThemeState& operator=(const CThemeState&);
};

class CThemePart
{
public:
    CThemePart() : iPartId(0) { }

    CThemeState*       FindState(int iStateId);
    const CThemeState* FindState(int iStateId) const;
    CThemeState*       EnsureState(int iStateId);
    const CThemeProperty* FindProperty(int lSymbolVal) const;

    int iPartId;
    CArray<CThemeProperty> properties;
    CArray<CThemeState>    states;

private:
    CThemePart(const CThemePart&);
    CThemePart& operator=(const CThemePart&);
};

class CThemeClass
{
public:
    CThemeClass() : iBaseClass(-1) { }

    CThemePart*       FindPart(int iPartId);
    const CThemePart* FindPart(int iPartId) const;
    CThemePart*       EnsurePart(int iPartId);
    const CThemeProperty* FindProperty(int lSymbolVal) const;

    CStringW strFullName;
    CStringW strApp;
    CStringW strClass;
    int      iBaseClass;

    CArray<CThemeProperty> properties;
    CArray<CThemePart>     parts;

private:
    CThemeClass(const CThemeClass&);
    CThemeClass& operator=(const CThemeClass&);
};

class CVariantEntry
{
public:
    CStringW strName;
    CStringW strSize;
    CStringW strColor;
};

class CThemeFile
{
public:
    CThemeFile() : m_cRef(1), nVersion(0) { }

    LONG AddRef() { return InterlockedIncrement(&m_cRef); }
    LONG Release()
    {
        LONG c = InterlockedDecrement(&m_cRef);
        if (c == 0)
        {
            this->~CThemeFile();
            ThemeHeapFree(this);
        }
        return c;
    }

    int FindCmapIndex(const WCHAR* pszName) const;

    int FindClass(const WCHAR* pszName) const;

    int EnsureClass(const WCHAR* pszFullName);

    const CThemeProperty* ResolveProperty(int iClass, int iPartId, int iStateId,
                                          int lSymbolVal) const;

    int nVersion;

    CPeModule style;
    CPeModule mui;

    CImageCache images;

    CStringW strPath;
    CStringW strVariantName;
    CStringW strSizeName;
    CStringW strColorName;

    CArray<CStringW> classNames;

    CArray<CVariantEntry> variants;

    CArray<CThemeProperty> rootProperties;
    CArray<CThemeClass>    classes;

private:
    CThemeFile(const CThemeFile&);
    CThemeFile& operator=(const CThemeFile&);

    LONG m_cRef;
};
