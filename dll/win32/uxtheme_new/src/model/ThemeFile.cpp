/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme object model lookups
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeFile.hpp"

const CThemeProperty* CThemeState::FindProperty(int lSymbolVal) const
{
    for (int i = 0; i < properties.GetCount(); i++)
        if (properties[i].lSymbolVal == lSymbolVal)
            return &properties[i];
    return NULL;
}

CThemeState* CThemePart::FindState(int iStateId)
{
    for (int i = 0; i < states.GetCount(); i++)
        if (states[i].iStateId == iStateId)
            return &states[i];
    return NULL;
}

const CThemeState* CThemePart::FindState(int iStateId) const
{
    return const_cast<CThemePart*>(this)->FindState(iStateId);
}

CThemeState* CThemePart::EnsureState(int iStateId)
{
    CThemeState* p = FindState(iStateId);
    if (p)
        return p;
    p = states.Add();
    if (p)
        p->iStateId = iStateId;
    return p;
}

const CThemeProperty* CThemePart::FindProperty(int lSymbolVal) const
{
    for (int i = 0; i < properties.GetCount(); i++)
        if (properties[i].lSymbolVal == lSymbolVal)
            return &properties[i];
    return NULL;
}

CThemePart* CThemeClass::FindPart(int iPartId)
{
    for (int i = 0; i < parts.GetCount(); i++)
        if (parts[i].iPartId == iPartId)
            return &parts[i];
    return NULL;
}

const CThemePart* CThemeClass::FindPart(int iPartId) const
{
    return const_cast<CThemeClass*>(this)->FindPart(iPartId);
}

CThemePart* CThemeClass::EnsurePart(int iPartId)
{
    CThemePart* p = FindPart(iPartId);
    if (p)
        return p;
    p = parts.Add();
    if (p)
        p->iPartId = iPartId;
    return p;
}

const CThemeProperty* CThemeClass::FindProperty(int lSymbolVal) const
{
    for (int i = 0; i < properties.GetCount(); i++)
        if (properties[i].lSymbolVal == lSymbolVal)
            return &properties[i];
    return NULL;
}

int CThemeFile::FindCmapIndex(const WCHAR* pszName) const
{
    for (int i = 0; i < classNames.GetCount(); i++)
        if (classNames[i].EqualsI(pszName))
            return i;
    return -1;
}

int CThemeFile::FindClass(const WCHAR* pszName) const
{
    int i;

    if (!pszName || !*pszName)
        return -1;

    for (i = 0; i < classes.GetCount(); i++)
        if (classes[i].strFullName.EqualsI(pszName))
            return i;

    for (i = 0; i < classes.GetCount(); i++)
        if (classes[i].strApp.IsEmpty() && classes[i].strClass.EqualsI(pszName))
            return i;

    for (i = 0; i < classes.GetCount(); i++)
        if (classes[i].strClass.EqualsI(pszName))
            return i;

    return -1;
}

int CThemeFile::EnsureClass(const WCHAR* pszFullName)
{
    for (int i = 0; i < classes.GetCount(); i++)
        if (classes[i].strFullName.EqualsI(pszFullName))
            return i;

    CThemeClass* p = classes.Add();
    if (!p)
        return -1;

    p->strFullName.Set(pszFullName);
    p->strFullName.SplitClassName(p->strApp, p->strClass);
    return classes.GetCount() - 1;
}

const CThemeProperty* CThemeFile::ResolveProperty(int iClass, int iPartId, int iStateId,
                                                  int lSymbolVal) const
{
    int cDepth = 0;

    while (iClass >= 0 && iClass < classes.GetCount() && cDepth++ < 64)
    {
        const CThemeClass& cls = classes[iClass];
        const CThemePart* pPart = cls.FindPart(iPartId);

        if (pPart)
        {
            if (iStateId != 0)
            {
                const CThemeState* pState = pPart->FindState(iStateId);
                if (pState)
                {
                    const CThemeProperty* pProp = pState->FindProperty(lSymbolVal);
                    if (pProp)
                        return pProp;
                }
            }

            const CThemeProperty* pProp = pPart->FindProperty(lSymbolVal);
            if (pProp)
                return pProp;
        }

        const CThemeProperty* pProp = cls.FindProperty(lSymbolVal);
        if (pProp)
            return pProp;

        iClass = cls.iBaseClass;
    }

    return NULL;
}
