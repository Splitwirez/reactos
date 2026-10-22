/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme enumeration and selection
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"
#include "../load/ThemeLoader.hpp"
#include "../model/Tmt.hpp"

#include <uxundoc.h>


static BOOL CALLBACK UXTHEME_send_theme_changed (HWND hWnd, LPARAM enable)
{
    SendMessageW(hWnd, WM_THEMECHANGED, enable, 0);
    return TRUE;
}

/* Broadcast WM_THEMECHANGED to *all* windows, including children */
BOOL CALLBACK UXTHEME_broadcast_theme_changed (HWND hWnd, LPARAM enable)
{
    if (hWnd == NULL)
    {
        EnumWindows (UXTHEME_broadcast_theme_changed, enable);
    }
    else
    {
        UXTHEME_send_theme_changed(hWnd, enable);
        EnumChildWindows (hWnd, UXTHEME_send_theme_changed, enable);
    }
    return TRUE;
}


void ReleaseProcessThemeFile(void);

static void GetVariantNames(CThemeFile* pFile, const WCHAR* pszPrefix,
                            const WCHAR* pszVariant, PTHEMENAMES pNames)
{
    WCHAR szCmap[MAX_PATH];
    int iCmap;
    int i;

    StringCchPrintfW(szCmap, ARRAYSIZE(szCmap), L"%s%s", pszPrefix, pszVariant);
    iCmap = pFile->FindCmapIndex(szCmap);

    for (i = 0; i < pFile->rootProperties.GetCount(); i++)
    {
        const CThemeProperty& prop = pFile->rootProperties[i];

        if (prop.iClass != iCmap || iCmap < 0)
            continue;

        if (prop.lSymbolVal == TMT_DISPLAYNAME)
            StringCchCopyW(pNames->szDisplayName, ARRAYSIZE(pNames->szDisplayName),
                           prop.value.GetString());
        else if (prop.lSymbolVal == TMT_TOOLTIP)
            StringCchCopyW(pNames->szTooltip, ARRAYSIZE(pNames->szTooltip),
                           prop.value.GetString());
    }

    if (!pNames->szDisplayName[0])
        StringCchCopyW(pNames->szDisplayName, ARRAYSIZE(pNames->szDisplayName),
                       pszVariant);
}

extern "C" {

HRESULT WINAPI EnumThemes(LPCWSTR pszThemePath, ENUMTHEMEPROC callback,
                          LPVOID lpData)
{
    WCHAR szDir[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    WCHAR szName[MAX_PATH];
    WCHAR szTip[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATAW wfd;
    size_t cchDir;

    if (!pszThemePath || !callback)
        return E_POINTER;

    StringCchCopyW(szDir, ARRAYSIZE(szDir), pszThemePath);
    cchDir = wcslen(szDir);
    if (cchDir > 0 && cchDir < ARRAYSIZE(szDir) - 2 && szDir[cchDir - 1] != L'\\')
    {
        szDir[cchDir] = L'\\';
        szDir[cchDir + 1] = L'\0';
    }

    StringCchPrintfW(szPath, ARRAYSIZE(szPath), L"%s*.*", szDir);

    hFind = FindFirstFileW(szPath, &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
        return S_OK;

    do
    {
        if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (wfd.cFileName[0] == L'.' &&
            (wfd.cFileName[1] == L'\0' ||
             (wfd.cFileName[1] == L'.' && wfd.cFileName[2] == L'\0')))
            continue;

        StringCchPrintfW(szPath, ARRAYSIZE(szPath), L"%s%s\\%s.msstyles",
                         szDir, wfd.cFileName, wfd.cFileName);

        szName[0] = L'\0';
        szTip[0] = L'\0';

        if (FAILED(GetThemeDocumentationProperty(szPath, L"displayname",
                                                 szName, ARRAYSIZE(szName))))
        {
            UXTRACE(("uxtheme_new: EnumThemes skipping '%ls' - not a v4 style\n",
                     szPath));
            continue;
        }

        GetThemeDocumentationProperty(szPath, L"tooltip", szTip, ARRAYSIZE(szTip));

        if (!callback(NULL, szPath, szName, szTip, NULL, lpData))
            break;
    }
    while (FindNextFileW(hFind, &wfd));

    FindClose(hFind);
    return S_OK;
}

HRESULT WINAPI EnumThemeColors(LPWSTR pszThemeFileName, LPWSTR pszSizeName,
                               DWORD dwColorNum, PTHEMENAMES pszColorNames)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;
    int i;
    DWORD dwSeen = 0;

    if (!pszColorNames)
        return E_POINTER;

    memset(pszColorNames, 0, sizeof(*pszColorNames));

    hr = CThemeLoader::LoadTheme(pszThemeFileName, NULL, NULL, &pFile);
    if (FAILED(hr))
        return hr;

    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    for (i = 0; i < pFile->variants.GetCount(); i++)
    {
        const CVariantEntry& v = pFile->variants[i];

        if (pszSizeName && *pszSizeName && !v.strSize.EqualsI(pszSizeName))
            continue;

        if (dwSeen++ != dwColorNum)
            continue;

        StringCchCopyW(pszColorNames->szName, ARRAYSIZE(pszColorNames->szName),
                       v.strColor.Get());
        GetVariantNames(pFile, L"colorvariant.", v.strColor.Get(), pszColorNames);
        hr = S_OK;
        break;
    }

    pFile->Release();
    return hr;
}

HRESULT WINAPI EnumThemeSizes(LPWSTR pszThemeFileName, LPWSTR pszColorName,
                              DWORD dwSizeNum, PTHEMENAMES pszSizeNames)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;
    int i;
    DWORD dwSeen = 0;

    if (!pszSizeNames)
        return E_POINTER;

    memset(pszSizeNames, 0, sizeof(*pszSizeNames));

    hr = CThemeLoader::LoadTheme(pszThemeFileName, NULL, NULL, &pFile);
    if (FAILED(hr))
        return hr;

    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    for (i = 0; i < pFile->variants.GetCount(); i++)
    {
        const CVariantEntry& v = pFile->variants[i];

        if (pszColorName && *pszColorName && !v.strColor.EqualsI(pszColorName))
            continue;

        if (dwSeen++ != dwSizeNum)
            continue;

        StringCchCopyW(pszSizeNames->szName, ARRAYSIZE(pszSizeNames->szName),
                       v.strSize.Get());
        GetVariantNames(pFile, L"sizevariant.", v.strSize.Get(), pszSizeNames);
        hr = S_OK;
        break;
    }

    pFile->Release();
    return hr;
}

HRESULT WINAPI GetThemeDefaults(LPCWSTR pszThemeFileName, LPWSTR pszColorName,
                                DWORD dwColorNameLen, LPWSTR pszSizeName,
                                DWORD dwSizeNameLen)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;

    hr = CThemeLoader::LoadTheme(pszThemeFileName, NULL, NULL, &pFile);
    if (FAILED(hr))
        return hr;

    if (pszColorName && dwColorNameLen)
        StringCchCopyW(pszColorName, dwColorNameLen, pFile->strColorName.Get());
    if (pszSizeName && dwSizeNameLen)
        StringCchCopyW(pszSizeName, dwSizeNameLen, pFile->strSizeName.Get());

    pFile->Release();
    return S_OK;
}

static HRESULT WriteThemeSettings(const WCHAR* pszDll, const WCHAR* pszColor,
                                  const WCHAR* pszSize, BOOL fActive)
{
    static const WCHAR szThemeManager[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager";
    HKEY hKey;
    LONG lRes;
    const WCHAR* pszOne = fActive ? L"1" : L"0";

    lRes = RegCreateKeyExW(HKEY_CURRENT_USER, szThemeManager, 0, NULL, 0,
                           KEY_SET_VALUE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRes);

    RegSetValueExW(hKey, L"ThemeActive", 0, REG_SZ,
                   (const BYTE*)pszOne, 2 * sizeof(WCHAR));

    if (fActive)
    {
        RegSetValueExW(hKey, L"DllName", 0, REG_SZ, (const BYTE*)pszDll,
                       (DWORD)((wcslen(pszDll) + 1) * sizeof(WCHAR)));
        RegSetValueExW(hKey, L"ColorName", 0, REG_SZ, (const BYTE*)pszColor,
                       (DWORD)((wcslen(pszColor) + 1) * sizeof(WCHAR)));
        RegSetValueExW(hKey, L"SizeName", 0, REG_SZ, (const BYTE*)pszSize,
                       (DWORD)((wcslen(pszSize) + 1) * sizeof(WCHAR)));
    }

    RegCloseKey(hKey);
    return S_OK;
}

HRESULT WINAPI ApplyTheme(HTHEMEFILE hThemeFile, UINT Flags, HWND hWnd)
{
    CThemeFile* pFile = (CThemeFile*)hThemeFile;
    HRESULT hr;

    (void)Flags;
    (void)hWnd;

    if (pFile)
        hr = WriteThemeSettings(pFile->strPath.Get(), pFile->strColorName.Get(),
                                pFile->strSizeName.Get(), TRUE);
    else
        hr = WriteThemeSettings(L"", L"", L"", FALSE);

    if (FAILED(hr))
        return hr;

    ReleaseProcessThemeFile();

    UXTHEME_broadcast_theme_changed(NULL, !!pFile);

    UXTRACE(("uxtheme_new: ApplyTheme -> '%ls' (%ls/%ls)\n",
             pFile ? pFile->strPath.Get() : L"<classic>",
             pFile ? pFile->strColorName.Get() : L"",
             pFile ? pFile->strSizeName.Get() : L""));

    return S_OK;
}

HRESULT WINAPI SetSystemVisualStyle(PCWSTR pszThemeFileName, PCWSTR pszColor,
                                    PCWSTR pszSize, UINT dwReserved)
{
    CThemeFile* pFile = NULL;
    HRESULT hr;

    if (!pszThemeFileName || !*pszThemeFileName)
        return ApplyTheme(NULL, dwReserved, NULL);

    hr = CThemeLoader::LoadTheme(pszThemeFileName,
                                 (pszColor && *pszColor) ? pszColor : NULL,
                                 (pszSize && *pszSize) ? pszSize : NULL,
                                 &pFile);
    if (FAILED(hr))
        return hr;

    hr = ApplyTheme((HTHEMEFILE)pFile, dwReserved, NULL);

    pFile->Release();
    return hr;
}

}
