/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme handles and the process theme file
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"
#include "../load/ThemeLoader.hpp"

#include <uxundoc.h>

extern "C" ATOM atWindowTheme;
extern "C" ATOM atSubAppName;
extern "C" ATOM atSubIdList;

static CRITICAL_SECTION g_csTheme;
static BOOL             g_fCsInit = FALSE;
static CThemeFile*      g_pThemeFile = NULL;
static BOOL             g_fLoadAttempted = FALSE;
static DWORD            g_dwAppProperties = STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS |
                                            STAP_ALLOW_WEBCONTENT;

void UxThemeApiInit(void)
{
    if (!g_fCsInit)
    {
        InitializeCriticalSection(&g_csTheme);
        g_fCsInit = TRUE;
    }
}

void UxThemeApiTerm(void)
{
    if (g_pThemeFile)
    {
        g_pThemeFile->Release();
        g_pThemeFile = NULL;
    }
}

HRESULT GetCurrentThemeSettings(BOOL* pfActive, WCHAR* pszDll, int cchDll,
                                WCHAR* pszColor, int cchColor,
                                WCHAR* pszSize, int cchSize)
{
    static const WCHAR szThemeManager[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";
    HKEY hKey;
    DWORD cb;
    WCHAR szActive[8];
    LONG lRes;

    if (pfActive)
        *pfActive = FALSE;
    if (pszDll && cchDll)
        pszDll[0] = L'\0';
    if (pszColor && cchColor)
        pszColor[0] = L'\0';
    if (pszSize && cchSize)
        pszSize[0] = L'\0';

    lRes = RegOpenKeyExW(HKEY_CURRENT_USER, szThemeManager, 0, KEY_QUERY_VALUE, &hKey);
    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes);

    cb = sizeof(szActive);
    if (RegQueryValueExW(hKey, L"ThemeActive", NULL, NULL, (LPBYTE)szActive, &cb) == ERROR_SUCCESS)
    {
        if (pfActive)
            *pfActive = (szActive[0] != L'0');
    }

    if (pszDll)
    {
        cb = (DWORD)cchDll * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, L"DllName", NULL, NULL, (LPBYTE)pszDll, &cb) != ERROR_SUCCESS)
            pszDll[0] = L'\0';
    }
    if (pszColor)
    {
        cb = (DWORD)cchColor * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, L"ColorName", NULL, NULL, (LPBYTE)pszColor, &cb) != ERROR_SUCCESS)
            pszColor[0] = L'\0';
    }
    if (pszSize)
    {
        cb = (DWORD)cchSize * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, L"SizeName", NULL, NULL, (LPBYTE)pszSize, &cb) != ERROR_SUCCESS)
            pszSize[0] = L'\0';
    }

    RegCloseKey(hKey);
    return S_OK;
}

CThemeFile* GetProcessThemeFile(void)
{
    WCHAR szDll[MAX_PATH];
    WCHAR szColor[64];
    WCHAR szSize[64];
    WCHAR szExpanded[MAX_PATH];
    BOOL fActive = FALSE;
    HRESULT hr;

    if (!g_fCsInit)
        return NULL;

    EnterCriticalSection(&g_csTheme);

    if (g_pThemeFile || g_fLoadAttempted)
    {
        LeaveCriticalSection(&g_csTheme);
        return g_pThemeFile;
    }

    g_fLoadAttempted = TRUE;

    hr = GetCurrentThemeSettings(&fActive, szDll, ARRAYSIZE(szDll),
                                 szColor, ARRAYSIZE(szColor),
                                 szSize, ARRAYSIZE(szSize));
    if (FAILED(hr) || !fActive || !szDll[0])
    {
        UXTRACE(("uxtheme_new: no theme selected (active=%d dll='%ls')\n",
                 fActive, szDll));
        LeaveCriticalSection(&g_csTheme);
        return NULL;
    }

    if (!ExpandEnvironmentStringsW(szDll, szExpanded, ARRAYSIZE(szExpanded)))
        StringCchCopyW(szExpanded, ARRAYSIZE(szExpanded), szDll);

    hr = CThemeLoader::LoadTheme(szExpanded,
                                 szColor[0] ? szColor : NULL,
                                 szSize[0] ? szSize : NULL,
                                 &g_pThemeFile);
    if (FAILED(hr))
    {
        UXTRACE(("uxtheme_new: LoadTheme('%ls') failed: %08lx\n",
                 szExpanded, (unsigned long)hr));
        g_pThemeFile = NULL;
    }

    LeaveCriticalSection(&g_csTheme);
    return g_pThemeFile;
}

void ReleaseProcessThemeFile(void)
{
    if (!g_fCsInit)
        return;

    EnterCriticalSection(&g_csTheme);
    if (g_pThemeFile)
    {
        g_pThemeFile->Release();
        g_pThemeFile = NULL;
    }
    g_fLoadAttempted = FALSE;
    LeaveCriticalSection(&g_csTheme);
}

THEMEHANDLE* ValidateThemeHandle(HTHEME hTheme)
{
    THEMEHANDLE* p = (THEMEHANDLE*)hTheme;

    if (!p)
        return NULL;

    if (IsBadReadPtr(p, sizeof(*p)) || p->dwMagic != THEMEHANDLE_MAGIC)
        return NULL;

    return p;
}

static HTHEME OpenThemeDataInternal(HWND hwnd, LPCWSTR pszClassList)
{
    CThemeFile* pFile;
    THEMEHANDLE* pHandle;
    const WCHAR* psz;
    const WCHAR* pszSubApp = NULL;
    int iClass = -1;

    if (!pszClassList || !*pszClassList)
    {
        SetLastError(E_INVALIDARG);
        return NULL;
    }

    pFile = GetProcessThemeFile();
    if (!pFile)
        return NULL;

    if (hwnd)
    {
        if (atSubIdList)
        {
            const WCHAR* pszIdList = (const WCHAR*)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atSubIdList));
            if (pszIdList && *pszIdList)
                pszClassList = pszIdList;
        }
        if (atSubAppName)
            pszSubApp = (const WCHAR*)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atSubAppName));
    }

    for (psz = pszClassList; *psz && iClass < 0; )
    {
        WCHAR szClass[128];
        const WCHAR* pszEnd = wcschr(psz, L';');
        int cch = pszEnd ? (int)(pszEnd - psz) : (int)wcslen(psz);

        if (cch > 0 && cch < ARRAYSIZE(szClass))
        {
            memcpy(szClass, psz, (SIZE_T)cch * sizeof(WCHAR));
            szClass[cch] = L'\0';

            if (pszSubApp && *pszSubApp && !wcschr(szClass, L':'))
            {
                WCHAR szQualified[ARRAYSIZE(szClass) + 64];
                if (SUCCEEDED(StringCchPrintfW(szQualified, ARRAYSIZE(szQualified), L"%s::%s", pszSubApp, szClass)))
                    iClass = pFile->FindClass(szQualified);
            }

            if (iClass < 0)
                iClass = pFile->FindClass(szClass);
        }

        psz = pszEnd ? pszEnd + 1 : psz + cch;
    }

    if (iClass < 0)
    {
        UXTRACE(("uxtheme_new: OpenThemeData('%ls') - no such class\n", pszClassList));
        return NULL;
    }

    pHandle = (THEMEHANDLE*)ThemeHeapAlloc(HEAP_ZERO_MEMORY, sizeof(THEMEHANDLE));
    if (!pHandle)
        return NULL;

    pHandle->dwMagic = THEMEHANDLE_MAGIC;
    pHandle->pFile = pFile;
    pHandle->iClass = iClass;
    pHandle->hwnd = hwnd;

    pFile->AddRef();

    if (hwnd && atWindowTheme)
        SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme), (HANDLE)pHandle);

    return (HTHEME)pHandle;
}

extern "C" {

HTHEME WINAPI OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
    return OpenThemeDataInternal(hwnd, pszClassList);
}

HTHEME WINAPI OpenThemeDataEx(HWND hwnd, LPCWSTR pszClassList, DWORD dwFlags)
{
    (void)dwFlags;
    return OpenThemeDataInternal(hwnd, pszClassList);
}

HTHEME WINAPI OpenThemeDataForDpi(HWND hwnd, LPCWSTR pszClassList, UINT dpi)
{
    if (dpi && dpi != 96)
        UXGAP("OpenThemeDataForDpi ignores the requested DPI");

    return OpenThemeDataInternal(hwnd, pszClassList);
}

HRESULT WINAPI CloseThemeData(HTHEME hTheme)
{
    THEMEHANDLE* p = ValidateThemeHandle(hTheme);

    if (!p)
        return E_HANDLE;

    if (p->hwnd && atWindowTheme &&
        GetPropW(p->hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme)) == (HANDLE)p)
    {
        RemovePropW(p->hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme));
    }

    p->dwMagic = 0;
    if (p->pFile)
        p->pFile->Release();
    p->pFile = NULL;
    ThemeHeapFree(p);
    return S_OK;
}

BOOL WINAPI IsThemeActive(void)
{
    BOOL fActive = FALSE;
    WCHAR szDll[MAX_PATH];

    if (FAILED(GetCurrentThemeSettings(&fActive, szDll, ARRAYSIZE(szDll),
                                       NULL, 0, NULL, 0)))
        return FALSE;

    return fActive && szDll[0] != L'\0';
}

BOOL WINAPI IsAppThemed(void)
{
    return IsThemeActive();
}

BOOL WINAPI IsCompositionActive(void)
{
    return FALSE;
}

HTHEME WINAPI GetWindowTheme(HWND hwnd)
{
    if (!atWindowTheme)
        return NULL;

    return (HTHEME)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atWindowTheme));
}

DWORD WINAPI GetThemeAppProperties(void)
{
    return g_dwAppProperties;
}

void WINAPI SetThemeAppProperties(DWORD dwFlags)
{
    g_dwAppProperties = dwFlags;
}

HRESULT WINAPI GetCurrentThemeName(LPWSTR pszThemeFileName, int cchMaxNameChars,
                                   LPWSTR pszColorBuff, int cchMaxColorChars,
                                   LPWSTR pszSizeBuff, int cchMaxSizeChars)
{
    BOOL fActive = FALSE;
    HRESULT hr;

    hr = GetCurrentThemeSettings(&fActive,
                                 pszThemeFileName, cchMaxNameChars,
                                 pszColorBuff, cchMaxColorChars,
                                 pszSizeBuff, cchMaxSizeChars);
    if (FAILED(hr))
        return hr;

    if (!fActive)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    return S_OK;
}

HRESULT WINAPI OpenThemeFile(LPCWSTR pszThemeFileName, LPCWSTR pszColorName,
                             LPCWSTR pszSizeName, HTHEMEFILE* phThemeFile,
                             DWORD dwFlags)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;

    (void)dwFlags;

    if (!phThemeFile)
        return E_POINTER;

    *phThemeFile = NULL;

    hr = CThemeLoader::LoadTheme(pszThemeFileName,
                                 (pszColorName && *pszColorName) ? pszColorName : NULL,
                                 (pszSizeName && *pszSizeName) ? pszSizeName : NULL,
                                 &pFile);
    if (FAILED(hr))
        return hr;

    *phThemeFile = (HTHEMEFILE)pFile;
    return S_OK;
}

HRESULT WINAPI CloseThemeFile(HTHEMEFILE hThemeFile)
{
    CThemeFile* pFile = (CThemeFile*)hThemeFile;

    if (!pFile)
        return E_INVALIDARG;

    pFile->Release();
    return S_OK;
}

HTHEME WINAPI OpenThemeDataFromFile(HTHEMEFILE hThemeFile, HWND hwnd,
                                    LPCWSTR pszClassList, DWORD dwFlags)
{
    CThemeFile* pFile = (CThemeFile*)hThemeFile;
    THEMEHANDLE* pHandle;
    int iClass;

    (void)dwFlags;

    if (!pFile || !pszClassList)
        return NULL;

    iClass = pFile->FindClass(pszClassList);
    if (iClass < 0)
        return NULL;

    pHandle = (THEMEHANDLE*)ThemeHeapAlloc(HEAP_ZERO_MEMORY, sizeof(THEMEHANDLE));
    if (!pHandle)
        return NULL;

    pHandle->dwMagic = THEMEHANDLE_MAGIC;
    pHandle->pFile = pFile;
    pHandle->iClass = iClass;
    pHandle->hwnd = hwnd;
    pFile->AddRef();

    return (HTHEME)pHandle;
}

HRESULT WINAPI CheckThemeSignature(LPCWSTR pszThemeFileName)
{
    CPeModule mod;
    HRESULT hr = mod.Open(pszThemeFileName);

    return hr;
}

}
