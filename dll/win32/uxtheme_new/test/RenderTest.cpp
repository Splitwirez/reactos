/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Rendering test
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include <windows.h>
#include <objidl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

typedef HANDLE HTHEME_;

typedef HRESULT (WINAPI *PFNOPENTHEMEFILE)(LPCWSTR, LPCWSTR, LPCWSTR, void**, DWORD);
typedef HRESULT (WINAPI *PFNCLOSETHEMEFILE)(void*);
typedef HTHEME_ (WINAPI *PFNOPENTHEMEDATAFROMFILE)(void*, HWND, LPCWSTR, DWORD);
typedef HRESULT (WINAPI *PFNCLOSETHEMEDATA)(HTHEME_);
typedef HRESULT (WINAPI *PFNGETTHEMERECT)(HTHEME_, int, int, int, RECT*);
typedef HRESULT (WINAPI *PFNGETTHEMEINT)(HTHEME_, int, int, int, int*);
typedef HRESULT (WINAPI *PFNDRAWTHEMEBACKGROUND)(HTHEME_, HDC, int, int, const RECT*, const RECT*);
typedef HRESULT (WINAPI *PFNGETTHEMESTREAM)(HTHEME_, int, int, int, void**, DWORD*, HINSTANCE);
typedef HRESULT (WINAPI *PFNGETTHEMEPARTSIZE)(HTHEME_, HDC, int, int, RECT*, int, SIZE*);

#define TMT_IMAGECOUNT  2401
#define TMT_ATLASRECT   8002
#define TMT_DISKSTREAM  213
#define TS_TRUE         2

static int g_cPass, g_cFail;

static void Check(bool ok, const char* what, const char* got)
{
    if (ok) { g_cPass++; printf("  ok    %-50s %s\n", what, got); }
    else    { g_cFail++; printf("  FAIL  %-50s %s\n", what, got); }
}

typedef struct { UINT32 v; void* cb; BOOL bg; BOOL ec; } GPSI;
typedef struct { INT X, Y, W, H; } GPRC;
typedef struct { UINT w, h; INT stride; INT fmt; void* scan0; UINT_PTR res; } GPBD;

static BYTE* DecodePng(const void* pv, DWORD cb, int* pcx, int* pcy)
{
    HMODULE hGp = LoadLibraryW(L"gdiplus.dll");
    HMODULE hSh = LoadLibraryW(L"shlwapi.dll");
    ULONG_PTR tok = 0;
    GPSI si = { 1, NULL, FALSE, FALSE };
    IStream* pStm;
    void* pBmp = NULL;
    UINT cx = 0, cy = 0;
    GPRC rc;
    GPBD bd;
    BYTE* pbOut = NULL;

    INT (WINAPI *pStart)(ULONG_PTR*, const GPSI*, void*) =
        (INT (WINAPI*)(ULONG_PTR*, const GPSI*, void*))GetProcAddress(hGp, "GdiplusStartup");
    INT (WINAPI *pFrom)(IStream*, void**) =
        (INT (WINAPI*)(IStream*, void**))GetProcAddress(hGp, "GdipCreateBitmapFromStream");
    INT (WINAPI *pW)(void*, UINT*) = (INT (WINAPI*)(void*, UINT*))GetProcAddress(hGp, "GdipGetImageWidth");
    INT (WINAPI *pH)(void*, UINT*) = (INT (WINAPI*)(void*, UINT*))GetProcAddress(hGp, "GdipGetImageHeight");
    INT (WINAPI *pLock)(void*, const GPRC*, UINT, INT, GPBD*) =
        (INT (WINAPI*)(void*, const GPRC*, UINT, INT, GPBD*))GetProcAddress(hGp, "GdipBitmapLockBits");
    INT (WINAPI *pUnlock)(void*, GPBD*) = (INT (WINAPI*)(void*, GPBD*))GetProcAddress(hGp, "GdipBitmapUnlockBits");
    IStream* (WINAPI *pMem)(const BYTE*, UINT) =
        (IStream* (WINAPI*)(const BYTE*, UINT))GetProcAddress(hSh, "SHCreateMemStream");

    if (!pStart || !pFrom || !pMem || pStart(&tok, &si, NULL) != 0)
        return NULL;

    pStm = pMem((const BYTE*)pv, cb);
    if (!pStm || pFrom(pStm, &pBmp) != 0 || !pBmp)
        return NULL;

    pW(pBmp, &cx);
    pH(pBmp, &cy);
    rc.X = rc.Y = 0; rc.W = (INT)cx; rc.H = (INT)cy;
    memset(&bd, 0, sizeof(bd));

    if (pLock(pBmp, &rc, 1, 0x000E200B, &bd) == 0)
    {
        pbOut = (BYTE*)malloc((size_t)cx * cy * 4);
        if (pbOut)
        {
            for (UINT y = 0; y < cy; y++)
                memcpy(pbOut + (size_t)y * cx * 4,
                       (BYTE*)bd.scan0 + (size_t)y * bd.stride, (size_t)cx * 4);
        }
        pUnlock(pBmp, &bd);
    }

    *pcx = (int)cx;
    *pcy = (int)cy;
    pStm->Release();
    return pbOut;
}

int wmain(int argc, wchar_t** argv)
{
    const WCHAR* pszDll = (argc > 1) ? argv[1]
        : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev\\DwmReversingEnvironment"
          L"\\build-msvc-amd64\\DarkFiresReactOSModules\\uxtheme_new\\uxtheme.dll";
    const WCHAR* pszStyle = (argc > 2) ? argv[2]
        : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev\\DwmReversingEnvironment"
          L"\\build-msvc-amd64\\media\\themes\\Aero\\aero.msstyles";

    HMODULE hMod, hStyle;
    void* hThemeFile = NULL;
    HTHEME_ hTheme = NULL;
    char szGot[256];
    HRESULT hr;
    RECT rcAtlas;
    void* pvPng = NULL;
    DWORD cbPng = 0;
    BYTE* pbRef = NULL;
    int cxRef = 0, cyRef = 0;

    printf("uxtheme_new render test\n\n");

    hMod = LoadLibraryW(pszDll);
    if (!hMod) { printf("LoadLibrary failed\n"); return 1; }

    PFNOPENTHEMEFILE         pOpenFile = (PFNOPENTHEMEFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(2));
    PFNCLOSETHEMEFILE        pCloseFile = (PFNCLOSETHEMEFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(3));
    PFNOPENTHEMEDATAFROMFILE pOpenData = (PFNOPENTHEMEDATAFROMFILE)GetProcAddress(hMod, MAKEINTRESOURCEA(16));
    PFNCLOSETHEMEDATA        pCloseData = (PFNCLOSETHEMEDATA)GetProcAddress(hMod, "CloseThemeData");
    PFNGETTHEMERECT          pGetRect = (PFNGETTHEMERECT)GetProcAddress(hMod, "GetThemeRect");
    PFNGETTHEMEINT           pGetInt = (PFNGETTHEMEINT)GetProcAddress(hMod, "GetThemeInt");
    PFNDRAWTHEMEBACKGROUND   pDraw = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(hMod, "DrawThemeBackground");
    PFNGETTHEMESTREAM        pGetStream = (PFNGETTHEMESTREAM)GetProcAddress(hMod, "GetThemeStream");
    PFNGETTHEMEPARTSIZE      pPartSize = (PFNGETTHEMEPARTSIZE)GetProcAddress(hMod, "GetThemePartSize");

    if (!pOpenFile || !pOpenData || !pDraw || !pGetStream || !pGetRect)
    { printf("exports missing\n"); return 1; }

    hr = pOpenFile(pszStyle, NULL, NULL, &hThemeFile, 0);
    if (FAILED(hr)) { printf("OpenThemeFile failed %08lx\n", (unsigned long)hr); return 1; }

    hTheme = pOpenData(hThemeFile, NULL, L"DWMWindow", 0);
    if (!hTheme) { printf("OpenThemeDataFromFile failed\n"); return 1; }

    hStyle = LoadLibraryExW(pszStyle, NULL, LOAD_LIBRARY_AS_DATAFILE);

    printf("[reference - this test's own decode of the atlas]\n");

    hr = pGetStream(hTheme, 57, 0, TMT_DISKSTREAM, &pvPng, &cbPng, hStyle);
    if (SUCCEEDED(hr))
        pbRef = DecodePng(pvPng, cbPng, &cxRef, &cyRef);

    sprintf(szGot, "%dx%d from %lu bytes", cxRef, cyRef, (unsigned long)cbPng);
    Check(pbRef != NULL && cxRef == 1199 && cyRef == 774,
          "atlas decodes to 1199x774", szGot);

    hr = pGetRect(hTheme, 57, 0, TMT_ATLASRECT, &rcAtlas);
    sprintf(szGot, "%ld,%ld,%ld,%ld", rcAtlas.left, rcAtlas.top, rcAtlas.right, rcAtlas.bottom);
    Check(SUCCEEDED(hr) && rcAtlas.left == 15 && rcAtlas.top == 692 &&
          rcAtlas.right == 36 && rcAtlas.bottom == 746,
          "part 57 ATLASRECT = 15,692,36,746", szGot);

    printf("\n[DrawThemeBackground at 1:1 - every cell unscaled]\n");
    if (pbRef)
    {
        int cImage = 1;
        pGetInt(hTheme, 57, 0, TMT_IMAGECOUNT, &cImage);

        int cxSlice = (int)((rcAtlas.right - rcAtlas.left) / (cImage ? cImage : 1));
        int cySlice = (int)(rcAtlas.bottom - rcAtlas.top);

        if (pPartSize)
        {
            SIZE sz = {0, 0};
            hr = pPartSize(hTheme, NULL, 57, 1, NULL, TS_TRUE, &sz);
            sprintf(szGot, "%ldx%ld (want %dx%d)", sz.cx, sz.cy, cxSlice, cySlice);
            Check(SUCCEEDED(hr) && sz.cx == cxSlice && sz.cy == cySlice,
                  "GetThemePartSize(TS_TRUE) = the state slice", szGot);
        }

        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        BITMAPINFO bmi;
        void* pvBits = NULL;

        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cxSlice;
        bmi.bmiHeader.biHeight = -cySlice;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        HGDIOBJ hOld = SelectObject(hdcMem, hbm);
        memset(pvBits, 0, (size_t)cxSlice * cySlice * 4);

        RECT rcDst = { 0, 0, cxSlice, cySlice };
        hr = pDraw(hTheme, hdcMem, 57, 1, &rcDst, NULL);
        sprintf(szGot, "hr=%08lx into %dx%d", (unsigned long)hr, cxSlice, cySlice);
        Check(SUCCEEDED(hr), "DrawThemeBackground(DWMWindow, 57, 1)", szGot);

        GdiFlush();

        if (SUCCEEDED(hr))
        {
            const BYTE* pbGot = (const BYTE*)pvBits;
            int cDiff = 0, cNonZero = 0, cPartialAlpha = 0;
            int xFirst = -1, yFirst = -1;

            for (int y = 0; y < cySlice; y++)
            {
                for (int x = 0; x < cxSlice; x++)
                {
                    const BYTE* pG = pbGot + ((size_t)y * cxSlice + x) * 4;
                    const BYTE* pR = pbRef + ((size_t)(rcAtlas.top + y) * cxRef +
                                              (rcAtlas.left + x)) * 4;

                    if (pG[0] || pG[1] || pG[2] || pG[3])
                        cNonZero++;
                    if (pG[3] != 0 && pG[3] != 255)
                        cPartialAlpha++;

                    if (memcmp(pG, pR, 4) != 0)
                    {
                        if (cDiff == 0) { xFirst = x; yFirst = y; }
                        cDiff++;
                    }
                }
            }

            sprintf(szGot, "%d of %d pixels differ (first at %d,%d)",
                    cDiff, cxSlice * cySlice, xFirst, yFirst);
            Check(cDiff == 0, "output == atlas sub-rect, byte for byte", szGot);

            sprintf(szGot, "%d non-zero of %d", cNonZero, cxSlice * cySlice);
            Check(cNonZero > 0, "something was actually drawn", szGot);

            sprintf(szGot, "%d pixels with 0 < alpha < 255", cPartialAlpha);
            Check(cPartialAlpha > 0, "per-pixel alpha survived the blit", szGot);
        }

        SelectObject(hdcMem, hOld);
        DeleteObject(hbm);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
    }

    printf("\n[IMAGECOUNT - states are different slices]\n");
    {
        HTHEME_ hBtn = pOpenData(hThemeFile, NULL, L"Button", 0);
        if (!hBtn)
        {
            Check(false, "OpenThemeDataFromFile(Button)", "<null>");
        }
        else
        {
            SIZE sz = { 0, 0 };
            int cImage = 0;
            pGetInt(hBtn, 1, 0, TMT_IMAGECOUNT, &cImage);

            hr = pPartSize ? pPartSize(hBtn, NULL, 1, 1, NULL, TS_TRUE, &sz) : E_FAIL;
            sprintf(szGot, "imagecount=%d slice=%ldx%ld", cImage, sz.cx, sz.cy);
            Check(SUCCEEDED(hr) && cImage == 6 && sz.cy > 0,
                  "Button part 1 has 6 stacked states", szGot);

            if (SUCCEEDED(hr) && sz.cx > 0 && sz.cy > 0)
            {
                HDC hdcScreen = GetDC(NULL);
                BYTE* rgpv[2] = { NULL, NULL };
                HDC hdc[2]; HBITMAP hbm[2]; HGDIOBJ hOld[2];
                BITMAPINFO bmi;
                int i;
                BOOL fDrew = TRUE;

                memset(&bmi, 0, sizeof(bmi));
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = sz.cx;
                bmi.bmiHeader.biHeight = -sz.cy;
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                for (i = 0; i < 2; i++)
                {
                    void* pv = NULL;
                    hdc[i] = CreateCompatibleDC(hdcScreen);
                    hbm[i] = CreateDIBSection(hdc[i], &bmi, DIB_RGB_COLORS, &pv, NULL, 0);
                    hOld[i] = SelectObject(hdc[i], hbm[i]);
                    memset(pv, 0, (size_t)sz.cx * sz.cy * 4);
                    rgpv[i] = (BYTE*)pv;

                    RECT rcD = { 0, 0, (LONG)sz.cx, (LONG)sz.cy };
                    if (FAILED(pDraw(hBtn, hdc[i], 1, i == 0 ? 1 : 4, &rcD, NULL)))
                        fDrew = FALSE;
                }

                GdiFlush();

                if (fDrew)
                {
                    int cDiff = 0;
                    for (int k = 0; k < sz.cx * sz.cy * 4; k++)
                        if (rgpv[0][k] != rgpv[1][k])
                            cDiff++;

                    sprintf(szGot, "%d bytes differ between state 1 and state 4", cDiff);
                    Check(cDiff > 0, "different states draw different pixels", szGot);
                }
                else
                {
                    Check(false, "different states draw different pixels", "draw failed");
                }

                for (i = 0; i < 2; i++)
                {
                    SelectObject(hdc[i], hOld[i]);
                    DeleteObject(hbm[i]);
                    DeleteDC(hdc[i]);
                }
                ReleaseDC(NULL, hdcScreen);
            }

            pCloseData(hBtn);
        }
    }

    printf("\n%d passed, %d failed\n", g_cPass, g_cFail);

    if (pbRef) free(pbRef);
    if (hStyle) FreeLibrary(hStyle);
    pCloseData(hTheme);
    pCloseFile(hThemeFile);
    return g_cFail ? 1 : 0;
}
