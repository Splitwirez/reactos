/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Per-window theme association
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"
#include "../load/ThemeLoader.hpp"

#include <uxundoc.h>

extern "C" ATOM atWindowTheme;
extern "C" ATOM atSubAppName;
extern "C" ATOM atSubIdList;
extern "C" ATOM atDialogThemeEnabled;

extern "C" BOOL CALLBACK UXTHEME_broadcast_theme_changed(HWND hWnd, LPARAM enable);

static HRESULT SetWindowStringProperty(HWND hwnd, ATOM atProp, LPCWSTR pszValue)
{
    LPWSTR pszOld;

    if (!atProp)
        return E_UNEXPECTED;

    pszOld = (LPWSTR)GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atProp));

    if (pszValue && *pszValue)
    {
        int cch = (int)wcslen(pszValue) + 1;
        LPWSTR pszNew = (LPWSTR)ThemeHeapAlloc(0, (SIZE_T)cch * sizeof(WCHAR));

        if (!pszNew)
            return E_OUTOFMEMORY;

        memcpy(pszNew, pszValue, (SIZE_T)cch * sizeof(WCHAR));

        if (!SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atProp), pszNew))
        {
            ThemeHeapFree(pszNew);
            return MakeErrorLast();
        }
    }
    else
    {
        RemovePropW(hwnd, (LPCWSTR)MAKEINTATOM(atProp));
    }

    ThemeHeapFree(pszOld);
    return S_OK;
}

extern "C" {

HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
{
    HRESULT hr;

    if (!IsWindow(hwnd))
        return E_HANDLE;

    hr = SetWindowStringProperty(hwnd, atSubAppName, pszSubAppName);
    if (FAILED(hr))
        return hr;

    hr = SetWindowStringProperty(hwnd, atSubIdList, pszSubIdList);
    if (FAILED(hr))
        return hr;

    UXTHEME_broadcast_theme_changed(hwnd, TRUE);
    return S_OK;
}

HRESULT WINAPI EnableThemeDialogTexture(HWND hwnd, DWORD dwFlags)
{
    static const WCHAR szTab[] = L"Tab";

    if (!IsWindow(hwnd))
        return E_HANDLE;

    if (!SetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled),
                  UlongToHandle(dwFlags | 0x80000000)))
    {
        return MakeErrorLast();
    }

    if (dwFlags & ETDT_USETABTEXTURE)
        return SetWindowTheme(hwnd, NULL, szTab);

    return S_OK;
}

BOOL WINAPI IsThemeDialogTextureEnabled(HWND hwnd)
{
    DWORD dwFlags = HandleToUlong(GetPropW(hwnd,
                                           (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled)));

    if (dwFlags == 0)
        return FALSE;

    return (dwFlags & ETDT_ENABLE) && !(dwFlags & ETDT_DISABLE);
}

HRESULT WINAPI DrawThemeParentBackground(HWND hwnd, HDC hdc, RECT* prc)
{
    RECT rt;
    POINT org;
    HWND hParent;
    HRGN hrgnClip = NULL;
    int nHasClip = -1;

    if (!IsWindow(hwnd) || !hdc)
        return E_HANDLE;

    hParent = GetParent(hwnd);
    if (!hParent)
        return S_OK;

    if (prc)
    {
        rt = *prc;
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);

        hrgnClip = CreateRectRgn(0, 0, 1, 1);
        nHasClip = GetClipRgn(hdc, hrgnClip);
        if (nHasClip != -1)
            IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }
    else
    {
        GetClientRect(hwnd, &rt);
        MapWindowPoints(hwnd, hParent, (LPPOINT)&rt, 2);
    }

    OffsetViewportOrgEx(hdc, -rt.left, -rt.top, &org);

    SendMessageW(hParent, WM_ERASEBKGND, (WPARAM)hdc, 0);
    SendMessageW(hParent, WM_PRINTCLIENT, (WPARAM)hdc, PRF_CLIENT);

    SetViewportOrgEx(hdc, org.x, org.y, NULL);

    if (prc)
    {
        if (nHasClip == 0)
            SelectClipRgn(hdc, NULL);
        else if (nHasClip == 1)
            SelectClipRgn(hdc, hrgnClip);
        DeleteObject(hrgnClip);
    }

    return S_OK;
}

}
