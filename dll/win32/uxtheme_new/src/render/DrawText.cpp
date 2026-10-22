/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme text drawing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "../api/ThemeApi.hpp"
#include "../image/Atlas.hpp"
#include "../model/Tmt.hpp"

static HFONT CreatePartFont(CThemeFile* pFile, int iClass, int iPartId,
                            int iStateId, HDC hdc, int iFontPropId)
{
    const CThemeProperty* pProp;
    const LOGFONTW* plf;
    LOGFONTW lf;

    pProp = pFile->ResolveProperty(iClass, iPartId, iStateId,
                                   iFontPropId ? iFontPropId : TMT_FONT);
    if (!pProp)
        return NULL;

    plf = pProp->value.GetFont();
    if (!plf)
        return NULL;

    lf = *plf;

    if (hdc)
    {
        int nDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        if (nDpi > 0 && nDpi != 96)
            lf.lfHeight = MulDiv(lf.lfHeight, nDpi, 96);
    }

    return CreateFontIndirectW(&lf);
}

static COLORREF GetTextColor_(CThemeFile* pFile, int iClass, int iPartId,
                              int iStateId, int iColorPropId, BOOL* pfHave)
{
    const CThemeProperty* pProp =
        pFile->ResolveProperty(iClass, iPartId, iStateId,
                               iColorPropId ? iColorPropId : TMT_TEXTCOLOR);

    if (pProp)
    {
        *pfHave = TRUE;
        return pProp->value.GetColor();
    }

    *pfHave = FALSE;
    return 0;
}

static HRESULT DrawTextInternal(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                LPCWSTR pszText, int iCharCount, DWORD dwFlags,
                                RECT* pRect, const DTTOPTS* pOptions)
{
    THEMEHANDLE* p;
    HFONT hFont = NULL;
    HGDIOBJ hFontOld = NULL;
    COLORREF crText;
    COLORREF crOld;
    int nBkOld;
    BOOL fHaveColor = FALSE;
    int iFontProp = 0;
    int iColorProp = 0;
    int iStateForProps = iStateId;
    UINT uFormat = dwFlags;

    if (!pszText || !pRect)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    if (iCharCount < 0)
        iCharCount = -1;

    if (pOptions && pOptions->dwSize >= sizeof(DTTOPTS))
    {
        if (pOptions->dwFlags & DTT_STATEID)
            iStateForProps = pOptions->iStateId;
        if (pOptions->dwFlags & DTT_FONTPROP)
            iFontProp = pOptions->iFontPropId;
        if (pOptions->dwFlags & DTT_COLORPROP)
            iColorProp = pOptions->iColorPropId;
        if (pOptions->dwFlags & DTT_CALCRECT)
            uFormat |= DT_CALCRECT;

        if (pOptions->dwFlags & DTT_GLOWSIZE)
            UXGAP("DTT_GLOWSIZE - composited text glow is not drawn");
        if (pOptions->dwFlags & (DTT_SHADOWTYPE | DTT_SHADOWOFFSET | DTT_SHADOWCOLOR))
            UXGAP("DTT_SHADOW* - text shadow is not drawn");
        if (pOptions->dwFlags & (DTT_BORDERSIZE | DTT_BORDERCOLOR))
            UXGAP("DTT_BORDER* - text border is not drawn");
        if (pOptions->dwFlags & DTT_CALLBACK)
            UXGAP("DTT_CALLBACK - pfnDrawTextCallback is not called");
    }

    hFont = CreatePartFont(p->pFile, p->iClass, iPartId, iStateForProps,
                           hdc, iFontProp);
    if (hFont)
        hFontOld = SelectObject(hdc, hFont);

    if (pOptions && pOptions->dwSize >= sizeof(DTTOPTS) &&
        (pOptions->dwFlags & DTT_TEXTCOLOR))
    {
        crText = pOptions->crText;
        fHaveColor = TRUE;
    }
    else
    {
        crText = GetTextColor_(p->pFile, p->iClass, iPartId, iStateForProps,
                               iColorProp, &fHaveColor);
    }

    crOld = fHaveColor ? SetTextColor(hdc, crText) : CLR_INVALID;
    nBkOld = SetBkMode(hdc, TRANSPARENT);

    DrawTextW(hdc, pszText, iCharCount, pRect, uFormat);

    SetBkMode(hdc, nBkOld);
    if (crOld != CLR_INVALID)
        SetTextColor(hdc, crOld);
    if (hFont)
    {
        SelectObject(hdc, hFontOld);
        DeleteObject(hFont);
    }

    return S_OK;
}

extern "C" {

HRESULT WINAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                             DWORD dwTextFlags2, const RECT* pRect)
{
    RECT rc;

    (void)dwTextFlags2;

    if (!pRect)
        return E_POINTER;

    rc = *pRect;
    return DrawTextInternal(hTheme, hdc, iPartId, iStateId, pszText, iCharCount,
                            dwTextFlags, &rc, NULL);
}

HRESULT WINAPI DrawThemeTextEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                               LPCWSTR pszText, int iCharCount, DWORD dwFlags,
                               RECT* pRect, const DTTOPTS* pOptions)
{
    return DrawTextInternal(hTheme, hdc, iPartId, iStateId, pszText, iCharCount,
                            dwFlags, pRect, pOptions);
}

HRESULT WINAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                  LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                                  const RECT* pBoundingRect, RECT* pExtentRect)
{
    THEMEHANDLE* p;
    HFONT hFont;
    HGDIOBJ hFontOld = NULL;
    RECT rc;

    if (!pszText || !pExtentRect)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    if (pBoundingRect)
        rc = *pBoundingRect;
    else
        SetRect(&rc, 0, 0, 0x00FFFFFF, 0x00FFFFFF);

    hFont = CreatePartFont(p->pFile, p->iClass, iPartId, iStateId, hdc, 0);
    if (hFont)
        hFontOld = SelectObject(hdc, hFont);

    DrawTextW(hdc, pszText, iCharCount < 0 ? -1 : iCharCount, &rc,
              dwTextFlags | DT_CALCRECT);

    if (hFont)
    {
        SelectObject(hdc, hFontOld);
        DeleteObject(hFont);
    }

    *pExtentRect = rc;
    return S_OK;
}

HRESULT WINAPI GetThemeTextMetrics(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                   TEXTMETRICW* ptm)
{
    THEMEHANDLE* p;
    HFONT hFont;
    HGDIOBJ hFontOld = NULL;
    BOOL fOk;

    if (!ptm)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    hFont = CreatePartFont(p->pFile, p->iClass, iPartId, iStateId, hdc, 0);
    if (hFont)
        hFontOld = SelectObject(hdc, hFont);

    fOk = GetTextMetricsW(hdc, ptm);

    if (hFont)
    {
        SelectObject(hdc, hFontOld);
        DeleteObject(hFont);
    }

    return fOk ? S_OK : MakeErrorLast();
}

}
