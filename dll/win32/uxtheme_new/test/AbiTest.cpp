/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Export ABI test
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

typedef HANDLE HTHEME_;

typedef struct _UXMARGINS
{
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
} MARGINS;

typedef HRESULT (WINAPI *PFNOPENTHEMEFILE)(LPCWSTR, LPCWSTR, LPCWSTR, void**, DWORD);
typedef HRESULT (WINAPI *PFNCLOSETHEMEFILE)(void*);
typedef HTHEME_ (WINAPI *PFNOPENTHEMEDATAFROMFILE)(void*, HWND, LPCWSTR, DWORD);
typedef HRESULT (WINAPI *PFNCLOSETHEMEDATA)(HTHEME_);
typedef HRESULT (WINAPI *PFNGETTHEMEINT)(HTHEME_, int, int, int, int*);
typedef HRESULT (WINAPI *PFNGETTHEMECOLOR)(HTHEME_, int, int, int, COLORREF*);
typedef HRESULT (WINAPI *PFNGETTHEMERECT)(HTHEME_, int, int, int, RECT*);
typedef HRESULT (WINAPI *PFNGETTHEMEMARGINS)(HTHEME_, HDC, int, int, int, RECT*, MARGINS*);
typedef BOOL    (WINAPI *PFNISTHEMEPARTDEFINED)(HTHEME_, int, int);

typedef struct tagTHEMENAMES_
{
    WCHAR szName[MAX_PATH + 1];
    WCHAR szDisplayName[MAX_PATH + 1];
    WCHAR szTooltip[MAX_PATH + 1];
} THEMENAMES_;

typedef BOOL (CALLBACK *ENUMTHEMEPROC_)(LPVOID, LPCWSTR, LPCWSTR, LPCWSTR,
                                        LPVOID, LPVOID);
typedef HRESULT (WINAPI *PFNGETTHEMEDEFAULTS)(LPCWSTR, LPWSTR, DWORD, LPWSTR, DWORD);
typedef HRESULT (WINAPI *PFNENUMTHEMES)(LPCWSTR, ENUMTHEMEPROC_, LPVOID);
typedef HRESULT (WINAPI *PFNENUMTHEMECOLORS)(LPWSTR, LPWSTR, DWORD, THEMENAMES_*);
typedef HRESULT (WINAPI *PFNENUMTHEMESIZES)(LPWSTR, LPWSTR, DWORD, THEMENAMES_*);
typedef HRESULT (WINAPI *PFNGETTHEMESTREAM)(HTHEME_, int, int, int, void**, DWORD*, HINSTANCE);

static int   g_cEnumHits;
static WCHAR g_szEnumName[MAX_PATH];

static BOOL CALLBACK EnumProc(LPVOID, LPCWSTR pszFile, LPCWSTR pszName,
                              LPCWSTR pszTip, LPVOID, LPVOID)
{
    (void)pszFile;
    (void)pszTip;
    if (g_cEnumHits++ == 0 && pszName)
        StringCchCopyW(g_szEnumName, ARRAYSIZE(g_szEnumName), pszName);
    return TRUE;
}
typedef HRESULT (WINAPI *PFNDRAWTHEMEBACKGROUND)(HTHEME_, HDC, int, int, const RECT*, const RECT*);

static int g_cPass, g_cFail;

static void Check(bool ok, const char* what, const char* got)
{
    if (ok) { g_cPass++; printf("  ok    %-52s %s\n", what, got); }
    else    { g_cFail++; printf("  FAIL  %-52s %s\n", what, got); }
}

#define TMT_TEXTGLOWSIZE   2425
#define TMT_GLOWINTENSITY  2429
#define TMT_OPACITY        2430
#define TMT_GLOWCOLOR      3816
#define TMT_SIZINGMARGINS  3601
#define TMT_CONTENTMARGINS 3602
#define TMT_ATLASRECT      8002

int wmain(int argc, wchar_t** argv)
{
    const WCHAR* pszDll = (argc > 1) ? argv[1]
        : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev\\DwmReversingEnvironment"
          L"\\build-msvc-amd64\\DarkFiresReactOSModules\\uxtheme_new\\uxtheme.dll";
    const WCHAR* pszStyle = (argc > 2) ? argv[2]
        : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev\\DwmReversingEnvironment"
          L"\\build-msvc-amd64\\media\\themes\\Aero\\aero.msstyles";

    HMODULE hMod;
    void* hThemeFile = NULL;
    HTHEME_ hTheme = NULL;
    char szGot[256];
    HRESULT hr;
    int n;
    COLORREF cr;
    RECT rc;
    MARGINS mar;

    PFNOPENTHEMEFILE         pfnOpenThemeFile;
    PFNCLOSETHEMEFILE        pfnCloseThemeFile;
    PFNOPENTHEMEDATAFROMFILE pfnOpenThemeDataFromFile;
    PFNCLOSETHEMEDATA        pfnCloseThemeData;
    PFNGETTHEMEINT           pfnGetThemeInt;
    PFNGETTHEMECOLOR         pfnGetThemeColor;
    PFNGETTHEMERECT          pfnGetThemeRect;
    PFNGETTHEMEMARGINS       pfnGetThemeMargins;
    PFNISTHEMEPARTDEFINED    pfnIsThemePartDefined;
    PFNDRAWTHEMEBACKGROUND   pfnDrawThemeBackground;

    printf("uxtheme_new ABI test\n");
    printf("dll  : %ls\n", pszDll);
    printf("style: %ls\n\n", pszStyle);

    printf("[load]\n");
    hMod = LoadLibraryW(pszDll);
    sprintf(szGot, "hmod=%p err=%lu", (void*)hMod, GetLastError());
    Check(hMod != NULL, "LoadLibrary(uxtheme.dll) - DllMain succeeded", szGot);
    if (!hMod)
        return 1;

    pfnOpenThemeFile         = (PFNOPENTHEMEFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(2));
    pfnCloseThemeFile        = (PFNCLOSETHEMEFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(3));
    pfnOpenThemeDataFromFile = (PFNOPENTHEMEDATAFROMFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(16));
    pfnCloseThemeData        = (PFNCLOSETHEMEDATA)GetProcAddress(hMod, "CloseThemeData");
    pfnGetThemeInt           = (PFNGETTHEMEINT)GetProcAddress(hMod, "GetThemeInt");
    pfnGetThemeColor         = (PFNGETTHEMECOLOR)GetProcAddress(hMod, "GetThemeColor");
    pfnGetThemeRect          = (PFNGETTHEMERECT)GetProcAddress(hMod, "GetThemeRect");
    pfnGetThemeMargins       = (PFNGETTHEMEMARGINS)GetProcAddress(hMod, "GetThemeMargins");
    pfnIsThemePartDefined    = (PFNISTHEMEPARTDEFINED)GetProcAddress(hMod, "IsThemePartDefined");
    pfnDrawThemeBackground   = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(hMod, "DrawThemeBackground");

    sprintf(szGot, "%s %s %s %s %s",
            pfnOpenThemeFile ? "OpenThemeFile" : "-",
            pfnOpenThemeDataFromFile ? "OpenThemeDataFromFile" : "-",
            pfnGetThemeInt ? "GetThemeInt" : "-",
            pfnGetThemeMargins ? "GetThemeMargins" : "-",
            pfnDrawThemeBackground ? "DrawThemeBackground" : "-");
    Check(pfnOpenThemeFile && pfnCloseThemeFile && pfnOpenThemeDataFromFile &&
          pfnCloseThemeData && pfnGetThemeInt && pfnGetThemeColor &&
          pfnGetThemeRect && pfnGetThemeMargins && pfnIsThemePartDefined &&
          pfnDrawThemeBackground,
          "all exercised exports resolve (by name or ordinal)", szGot);

    if (!pfnOpenThemeFile || !pfnOpenThemeDataFromFile || !pfnGetThemeInt)
        return 1;

    printf("\n[open]\n");
    hr = pfnOpenThemeFile(pszStyle, NULL, NULL, &hThemeFile, 0);
    sprintf(szGot, "hr=%08lx file=%p", (unsigned long)hr, hThemeFile);
    Check(SUCCEEDED(hr) && hThemeFile, "OpenThemeFile(aero.msstyles)", szGot);
    if (FAILED(hr))
        return 1;

    hTheme = pfnOpenThemeDataFromFile(hThemeFile, NULL, L"DWMWindow", 0);
    sprintf(szGot, "htheme=%p", (void*)hTheme);
    Check(hTheme != NULL, "OpenThemeDataFromFile(L\"DWMWindow\")  <- phase 1 gate", szGot);
    if (!hTheme)
        return 1;

    printf("\n[properties through the real ABI]\n");

    hr = pfnGetThemeInt(hTheme, 57, 0, TMT_TEXTGLOWSIZE, &n);
    sprintf(szGot, "hr=%08lx n=%d (want 10)", (unsigned long)hr, n);
    Check(SUCCEEDED(hr) && n == 10, "GetThemeInt(57, 0, TEXTGLOWSIZE)  <- phase 1 gate", szGot);

    hr = pfnGetThemeInt(hTheme, 57, 0, TMT_GLOWINTENSITY, &n);
    sprintf(szGot, "hr=%08lx n=%d (want 336)", (unsigned long)hr, n);
    Check(SUCCEEDED(hr) && n == 336, "GetThemeInt(57, 0, GLOWINTENSITY)", szGot);

    hr = pfnGetThemeInt(hTheme, 40, 1, TMT_OPACITY, &n);
    sprintf(szGot, "hr=%08lx n=%d (want 90)", (unsigned long)hr, n);
    Check(SUCCEEDED(hr) && n == 90, "GetThemeInt(40, 1, OPACITY)", szGot);

    hr = pfnGetThemeColor(hTheme, 57, 3, TMT_GLOWCOLOR, &cr);
    sprintf(szGot, "hr=%08lx cr=%06lX (want 000000)", (unsigned long)hr, (unsigned long)cr);
    Check(SUCCEEDED(hr) && cr == RGB(0, 0, 0), "GetThemeColor(57, 3, GLOWCOLOR)", szGot);

    hr = pfnGetThemeRect(hTheme, 57, 0, TMT_ATLASRECT, &rc);
    sprintf(szGot, "hr=%08lx %ld,%ld,%ld,%ld (want 15,692,36,746)",
            (unsigned long)hr, rc.left, rc.top, rc.right, rc.bottom);
    Check(SUCCEEDED(hr) && rc.left == 15 && rc.top == 692 &&
          rc.right == 36 && rc.bottom == 746, "GetThemeRect(57, 0, ATLASRECT)", szGot);

    hr = pfnGetThemeMargins(hTheme, NULL, 57, 0, TMT_CONTENTMARGINS, NULL, &mar);
    sprintf(szGot, "hr=%08lx %d,%d,%d,%d (want 9,0,0,0)", (unsigned long)hr,
            mar.cxLeftWidth, mar.cxRightWidth, mar.cyTopHeight, mar.cyBottomHeight);
    Check(SUCCEEDED(hr) && mar.cxLeftWidth == 9 && mar.cxRightWidth == 0 &&
          mar.cyTopHeight == 0 && mar.cyBottomHeight == 0,
          "GetThemeMargins(57, 0, CONTENTMARGINS)", szGot);

    sprintf(szGot, "57=%d  9999=%d (want 1, 0)",
            pfnIsThemePartDefined(hTheme, 57, 0) ? 1 : 0,
            pfnIsThemePartDefined(hTheme, 9999, 0) ? 1 : 0);
    Check(pfnIsThemePartDefined(hTheme, 57, 0) &&
          !pfnIsThemePartDefined(hTheme, 9999, 0), "IsThemePartDefined", szGot);

    n = 0x5A5A5A5A;
    hr = pfnGetThemeInt(hTheme, 57, 0, TMT_OPACITY, &n);
    sprintf(szGot, "hr=%08lx (want 80070490)", (unsigned long)hr);
    Check(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
          "undefined property -> E_PROP_ID_UNSUPPORTED", szGot);

    hr = pfnDrawThemeBackground(hTheme, NULL, 57, 1, NULL, NULL);
    sprintf(szGot, "hr=%08lx (want 80004003)", (unsigned long)hr);
    Check(hr == E_POINTER, "DrawThemeBackground(NULL rect) -> E_POINTER", szGot);

    printf("\n[GetThemeStream - the non-client atlas]\n");
    {
        PFNGETTHEMESTREAM pfnGetThemeStream =
            (PFNGETTHEMESTREAM)GetProcAddress(hMod, "GetThemeStream");

        sprintf(szGot, "%p", (void*)pfnGetThemeStream);
        Check(pfnGetThemeStream != NULL,
              "GetThemeStream resolves by name (ordinal 102)", szGot);

        if (pfnGetThemeStream)
        {
            HMODULE hStyle = LoadLibraryExW(pszStyle, NULL, LOAD_LIBRARY_AS_DATAFILE);
            void* pvPng = NULL;
            DWORD cbPng = 0;

            sprintf(szGot, "hStyle=%p", (void*)hStyle);
            Check(hStyle != NULL, "caller maps the style itself", szGot);

            hr = pfnGetThemeStream(hTheme, 57, 0, 213, &pvPng, &cbPng, hStyle);
            sprintf(szGot, "hr=%08lx p=%p cb=%lu", (unsigned long)hr, pvPng,
                    (unsigned long)cbPng);
            Check(SUCCEEDED(hr) && pvPng && cbPng > 0,
                  "GetThemeStream(part 57, TMT_DISKSTREAM)", szGot);

            if (SUCCEEDED(hr) && pvPng && cbPng >= 8)
            {
                const BYTE* pb = (const BYTE*)pvPng;
                sprintf(szGot, "%02X %02X %02X %02X (want 89 50 4E 47)",
                        pb[0], pb[1], pb[2], pb[3]);
                Check(pb[0] == 0x89 && pb[1] == 'P' && pb[2] == 'N' && pb[3] == 'G',
                      "the stream really is a PNG", szGot);
            }

            {
                void* pv2 = NULL;
                DWORD cb2 = 0;
                hr = pfnGetThemeStream(hTheme, 40, 0, 213, &pv2, &cb2, hStyle);
                sprintf(szGot, "part57=%p/%lu part40=%p/%lu", pvPng,
                        (unsigned long)cbPng, pv2, (unsigned long)cb2);
                Check(SUCCEEDED(hr) && pv2 == pvPng && cb2 == cbPng,
                      "every part resolves to the one class-level atlas", szGot);
            }

            {
                void* pv3 = NULL;
                DWORD cb3 = 0;
                hr = pfnGetThemeStream(hTheme, 57, 0, 213, &pv3, &cb3, NULL);
                sprintf(szGot, "hr=%08lx (want 80070057)", (unsigned long)hr);
                Check(hr == E_INVALIDARG, "no hInst -> E_INVALIDARG", szGot);
            }

            if (hStyle)
                FreeLibrary(hStyle);
        }
    }

    printf("\n[enumeration - what desk.cpl calls]\n");
    {
        PFNGETTHEMEDEFAULTS pfnGetThemeDefaults =
            (PFNGETTHEMEDEFAULTS)GetProcAddress(hMod, MAKEINTRESOURCEA(7));
        PFNENUMTHEMES       pfnEnumThemes =
            (PFNENUMTHEMES)GetProcAddress(hMod, MAKEINTRESOURCEA(8));
        PFNENUMTHEMECOLORS  pfnEnumThemeColors =
            (PFNENUMTHEMECOLORS)GetProcAddress(hMod, MAKEINTRESOURCEA(9));
        PFNENUMTHEMESIZES   pfnEnumThemeSizes =
            (PFNENUMTHEMESIZES)GetProcAddress(hMod, MAKEINTRESOURCEA(10));

        sprintf(szGot, "%p %p %p %p", (void*)pfnGetThemeDefaults,
                (void*)pfnEnumThemes, (void*)pfnEnumThemeColors,
                (void*)pfnEnumThemeSizes);
        Check(pfnGetThemeDefaults && pfnEnumThemes && pfnEnumThemeColors &&
              pfnEnumThemeSizes, "enumeration exports resolve (ordinals 7-10)", szGot);

        if (pfnGetThemeDefaults && pfnEnumThemeColors && pfnEnumThemeSizes)
        {
            WCHAR szColor[MAX_PATH] = {0};
            WCHAR szSize[MAX_PATH] = {0};
            THEMENAMES_ names;

            hr = pfnGetThemeDefaults(pszStyle, szColor, ARRAYSIZE(szColor),
                                     szSize, ARRAYSIZE(szSize));
            sprintf(szGot, "hr=%08lx color=\"%ls\" size=\"%ls\"",
                    (unsigned long)hr, szColor, szSize);
            Check(SUCCEEDED(hr) && wcscmp(szColor, L"NormalColor") == 0 &&
                  wcscmp(szSize, L"NormalSize") == 0,
                  "GetThemeDefaults -> NormalColor / NormalSize", szGot);

            memset(&names, 0, sizeof(names));
            hr = pfnEnumThemeColors((LPWSTR)pszStyle, NULL, 0, &names);
            sprintf(szGot, "hr=%08lx name=\"%ls\" display=\"%ls\"",
                    (unsigned long)hr, names.szName, names.szDisplayName);
            Check(SUCCEEDED(hr) && wcscmp(names.szName, L"NormalColor") == 0 &&
                  names.szDisplayName[0] != L'\0',
                  "EnumThemeColors(0) -> NormalColor with a display name", szGot);

            memset(&names, 0, sizeof(names));
            hr = pfnEnumThemeColors((LPWSTR)pszStyle, NULL, 1, &names);
            sprintf(szGot, "hr=%08lx (want 80070490)", (unsigned long)hr);
            Check(hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                  "EnumThemeColors(1) -> E_PROP_ID_UNSUPPORTED", szGot);

            memset(&names, 0, sizeof(names));
            hr = pfnEnumThemeSizes((LPWSTR)pszStyle, NULL, 0, &names);
            sprintf(szGot, "hr=%08lx name=\"%ls\" display=\"%ls\"",
                    (unsigned long)hr, names.szName, names.szDisplayName);
            Check(SUCCEEDED(hr) && wcscmp(names.szName, L"NormalSize") == 0 &&
                  names.szDisplayName[0] != L'\0',
                  "EnumThemeSizes(0) -> NormalSize with a display name", szGot);
        }

        if (pfnEnumThemes)
        {
            WCHAR szThemes[MAX_PATH];
            WCHAR* pszCut;
            g_cEnumHits = 0;
            g_szEnumName[0] = L'\0';

            StringCchCopyW(szThemes, ARRAYSIZE(szThemes), pszStyle);
            pszCut = wcsrchr(szThemes, L'\\');
            if (pszCut) { *pszCut = L'\0'; pszCut = wcsrchr(szThemes, L'\\'); }
            if (pszCut) *pszCut = L'\0';

            hr = pfnEnumThemes(szThemes, EnumProc, NULL);
            sprintf(szGot, "hr=%08lx hits=%d first=\"%ls\" in %ls",
                    (unsigned long)hr, g_cEnumHits, g_szEnumName, szThemes);
            Check(SUCCEEDED(hr) && g_cEnumHits >= 1,
                  "EnumThemes finds the built Aero", szGot);
        }
    }

    printf("\n[close]\n");
    hr = pfnCloseThemeData(hTheme);
    sprintf(szGot, "hr=%08lx", (unsigned long)hr);
    Check(SUCCEEDED(hr), "CloseThemeData", szGot);

    hr = pfnCloseThemeFile(hThemeFile);
    sprintf(szGot, "hr=%08lx", (unsigned long)hr);
    Check(SUCCEEDED(hr), "CloseThemeFile", szGot);

    FreeLibrary(hMod);

    printf("\n%d passed, %d failed\n", g_cPass, g_cFail);
    return g_cFail ? 1 : 0;
}
