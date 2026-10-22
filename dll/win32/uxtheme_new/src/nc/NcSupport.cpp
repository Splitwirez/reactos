/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Engine helpers for the non-client layer
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "../UxThemeP.hpp"
#include "../api/ThemeApi.hpp"
#include "../image/Atlas.hpp"

extern "C" {
#include "UxNc.h"
}

void ReleaseProcessThemeFile(void);

extern "C" {
HINSTANCE   hDllInst;
ATOM        atWindowTheme;
ATOM        atWndContext;

ATOM        atSubAppName;
ATOM        atSubIdList;
ATOM        atDialogThemeEnabled;

void UxNcInitSystem(HINSTANCE hInst)
{
    hDllInst = hInst;

    if (!atWindowTheme)
        atWindowTheme = GlobalAddAtomW(L"ux_theme");
    if (!atWndContext)
        atWndContext = GlobalAddAtomW(L"ux_WndContext");
    if (!atSubAppName)
        atSubAppName = GlobalAddAtomW(L"ux_subapp");
    if (!atSubIdList)
        atSubIdList = GlobalAddAtomW(L"ux_subidlist");
    if (!atDialogThemeEnabled)
        atDialogThemeEnabled = GlobalAddAtomW(L"ux_dialogtheme");
}

void UxNcUnInitSystem(void)
{
    if (atWindowTheme)
    {
        GlobalDeleteAtom(atWindowTheme);
        atWindowTheme = 0;
    }
    if (atWndContext)
    {
        GlobalDeleteAtom(atWndContext);
        atWndContext = 0;
    }
    if (atSubAppName)
    {
        GlobalDeleteAtom(atSubAppName);
        atSubAppName = 0;
    }
    if (atSubIdList)
    {
        GlobalDeleteAtom(atSubIdList);
        atSubIdList = 0;
    }
    if (atDialogThemeEnabled)
    {
        GlobalDeleteAtom(atDialogThemeEnabled);
        atDialogThemeEnabled = 0;
    }
}

static BOOL CALLBACK UXTHEME_send_theme_changed(HWND hWnd, LPARAM enable)
{
    SendMessageW(hWnd, WM_THEMECHANGED, (WPARAM)enable, 0);
    return TRUE;
}

BOOL CALLBACK UXTHEME_broadcast_theme_changed(HWND hWnd, LPARAM enable)
{
    if (hWnd == NULL)
    {
        EnumWindows(UXTHEME_broadcast_theme_changed, enable);
    }
    else
    {
        UXTHEME_send_theme_changed(hWnd, enable);
        EnumChildWindows(hWnd, UXTHEME_send_theme_changed, enable);
    }
    return TRUE;
}

void UXTHEME_LoadTheme(BOOL bLoad)
{
    (void)bLoad;
    ReleaseProcessThemeFile();
}

HRESULT UXTHEME_LoadImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                          const RECT* pRect, BOOL glyph, HBITMAP* phBitmap,
                          RECT* pRectDst, BOOL* pfHasAlpha)
{
    THEMEHANDLE* p;
    UXDRAWINFO info;
    CUxImage* pImage = NULL;
    HRESULT hr;
    HDC hdcRef;
    BITMAPINFO bmi;
    void* pvBits = NULL;
    HBITMAP hbm;
    int cx, cy, y;

    (void)pRect;
    (void)glyph;

    if (!phBitmap)
        return E_POINTER;

    *phBitmap = NULL;
    if (pfHasAlpha)
        *pfHasAlpha = FALSE;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    hr = UxResolveDrawInfo(p->pFile, p->iClass, iPartId, iStateId, &info, &pImage, NULL);
    if (FAILED(hr))
        return hr;
    if (!pImage)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    cx = info.rcSrc.right - info.rcSrc.left;
    cy = info.rcSrc.bottom - info.rcSrc.top;
    if (cx <= 0 || cy <= 0)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    hdcRef = hdc ? hdc : GetDC(NULL);
    if (!hdcRef)
        return MakeErrorLast();

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbm = CreateDIBSection(hdcRef, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

    if (hbm && pvBits)
    {
        BOOL fPartial = FALSE;

        for (y = 0; y < cy; y++)
        {
            const BYTE* pSrc = pImage->Bits() +
                               (SIZE_T)(info.rcSrc.top + y) * pImage->Stride() +
                               (SIZE_T)info.rcSrc.left * 4;
            BYTE* pDst = (BYTE*)pvBits + (SIZE_T)y * cx * 4;
            int x;

            memcpy(pDst, pSrc, (SIZE_T)cx * 4);

            for (x = 0; x < cx && !fPartial; x++)
                if (pDst[x * 4 + 3] != 0xFF)
                    fPartial = TRUE;
        }

        if (pfHasAlpha)
            *pfHasAlpha = fPartial;

        *phBitmap = hbm;

        if (pRectDst)
        {
            pRectDst->left = 0;
            pRectDst->top = 0;
            pRectDst->right = cx;
            pRectDst->bottom = cy;
        }

        hr = S_OK;
    }
    else
    {
        if (hbm)
            DeleteObject(hbm);
        hr = E_OUTOFMEMORY;
    }

    if (!hdc)
        ReleaseDC(NULL, hdcRef);

    return hr;
}

}
