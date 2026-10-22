/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Visual style module resource access
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "PeModule.hpp"

HRESULT MakeErrorLast(void)
{
    DWORD dwErr = GetLastError();

    if (dwErr == ERROR_SUCCESS)
        return E_FAIL;

    return HRESULT_FROM_WIN32(dwErr);
}

HRESULT CPeModule::Open(const WCHAR* pszPath)
{
    const void* pv;
    DWORD cb;
    HRESULT hr;
    SHORT nVersion;

    Close();

    if (!pszPath || !*pszPath)
        return E_INVALIDARG;

    m_hInst = LoadLibraryExW(pszPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!m_hInst)
        return MakeErrorLast();

    hr = GetResource(L"PACKTHEM_VERSION", (UINT)1, &pv, &cb);
    if (FAILED(hr) || cb != sizeof(SHORT))
    {
        Close();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    memcpy(&nVersion, pv, sizeof(nVersion));
    m_nVersion = nVersion;

    if (m_nVersion != 4)
    {
        UXTRACE(("uxtheme_new: '%ls' is PACKTHEM_VERSION %d, not 4 - rejecting\n",
                 pszPath, m_nVersion));
        Close();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    return S_OK;
}

void CPeModule::Close()
{
    if (m_hInst)
    {
        FreeLibrary(m_hInst);
        m_hInst = NULL;
    }
    m_nVersion = 0;
}

HRESULT CPeModule::GetResource(const WCHAR* pszType, const WCHAR* pszName,
                               const void** ppv, DWORD* pcb) const
{
    HRSRC hRes;
    DWORD cb;
    HGLOBAL hData;
    const void* pv;

    if (ppv)
        *ppv = NULL;
    if (pcb)
        *pcb = 0;

    if (!m_hInst)
        return E_UNEXPECTED;

    hRes = FindResourceW(m_hInst, pszName, pszType);
    if (!hRes)
        return MakeErrorLast();

    cb = SizeofResource(m_hInst, hRes);
    if (!cb)
        return MakeErrorLast();

    hData = LoadResource(m_hInst, hRes);
    if (!hData)
        return MakeErrorLast();

    pv = LockResource(hData);
    if (!pv)
        return MakeErrorLast();

    *ppv = pv;
    if (pcb)
        *pcb = cb;
    return S_OK;
}

HRESULT CPeModule::GetResource(const WCHAR* pszType, UINT id,
                               const void** ppv, DWORD* pcb) const
{
    return GetResource(pszType, MAKEINTRESOURCEW(id), ppv, pcb);
}

HRESULT CPeModule::GetStringResource(UINT id, const WCHAR** ppsz, int* pcch) const
{
    const void* pv;
    DWORD cb;
    HRESULT hr;
    const WCHAR* p;
    const WCHAR* pEnd;
    UINT iBlock;
    UINT iIndex;
    UINT i;

    if (ppsz)
        *ppsz = NULL;
    if (pcch)
        *pcch = 0;

    iBlock = (id >> 4) + 1;
    iIndex = id & 0x0F;

    hr = GetResource((const WCHAR*)RT_STRING, iBlock, &pv, &cb);
    if (FAILED(hr))
        return hr;

    p = (const WCHAR*)pv;
    pEnd = p + cb / sizeof(WCHAR);

    for (i = 0; i < iIndex; i++)
    {
        if (p >= pEnd)
            return HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND);
        p += 1 + *p;
    }

    if (p >= pEnd || p + 1 + *p > pEnd)
        return HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND);

    if (*p == 0)
        return HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND);

    if (ppsz)
        *ppsz = p + 1;
    if (pcch)
        *pcch = *p;
    return S_OK;
}
