/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Visual style module resource access
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"

class CPeModule
{
public:
    CPeModule() : m_hInst(NULL), m_nVersion(0) { }
    ~CPeModule() { Close(); }

    HRESULT Open(const WCHAR* pszPath);
    void    Close();

    void AttachRaw(HINSTANCE hInst) { Close(); m_hInst = hInst; m_nVersion = 0; }

    bool     IsOpen() const { return m_hInst != NULL; }
    HINSTANCE GetInstance() const { return m_hInst; }
    int      GetVersion() const { return m_nVersion; }

    HRESULT GetResource(const WCHAR* pszType, const WCHAR* pszName,
                        const void** ppv, DWORD* pcb) const;
    HRESULT GetResource(const WCHAR* pszType, UINT id,
                        const void** ppv, DWORD* pcb) const;

    HRESULT GetStringResource(UINT id, const WCHAR** ppsz, int* pcch) const;

private:
    CPeModule(const CPeModule&);
    CPeModule& operator=(const CPeModule&);

    HINSTANCE m_hInst;
    int       m_nVersion;
};

HRESULT MakeErrorLast(void);
