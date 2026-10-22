/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Unimplemented exports
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"

#include <uxundoc.h>

extern "C" {
HRESULT WINAPI DrawThemeEdge(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             const RECT* pDestRect, UINT uEdge, UINT uFlags,
                             RECT* pContentRect)
{
    UXSTUB();
    (void)hTheme; (void)hdc; (void)iPartId; (void)iStateId;
    (void)pDestRect; (void)uEdge; (void)uFlags; (void)pContentRect;
    return E_NOTIMPL;
}

HRESULT WINAPI DrawThemeIcon(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             const RECT* pRect, HIMAGELIST himl, int iImageIndex)
{
    UXSTUB();
    (void)hTheme; (void)hdc; (void)iPartId; (void)iStateId;
    (void)pRect; (void)himl; (void)iImageIndex;
    return E_NOTIMPL;
}

HRESULT WINAPI HitTestThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                      DWORD dwOptions, const RECT* pRect, HRGN hrgn,
                                      POINT ptTest, WORD* pwHitTestCode)
{
    UXSTUB();
    (void)hTheme; (void)hdc; (void)iPartId; (void)iStateId; (void)dwOptions;
    (void)pRect; (void)hrgn; (void)ptTest; (void)pwHitTestCode;
    return E_NOTIMPL;
}

HRESULT WINAPI GetThemeTransitionDuration(HTHEME hTheme, int iPartId, int iStateIdFrom,
                                          int iStateIdTo, int iPropId, DWORD* pdwDuration)
{
    UXSTUB();
    (void)hTheme; (void)iPartId; (void)iStateIdFrom; (void)iStateIdTo;
    (void)iPropId; (void)pdwDuration;
    return E_NOTIMPL;
}

HRESULT WINAPI SetWindowThemeAttribute(HWND hwnd, int eAttribute,
                                       PVOID pvAttribute, DWORD cbAttribute)
{
    UXSTUB();
    (void)hwnd; (void)eAttribute; (void)pvAttribute; (void)cbAttribute;
    return E_NOTIMPL;
}

HRESULT WINAPI EnableTheming(BOOL fEnable)
{
    UXSTUB();
    (void)fEnable;
    return E_NOTIMPL;
}

DWORD WINAPI QueryThemeServices(void)
{
    UXSTUB();
    return 0;
}

HRESULT WINAPI ParseThemeIniFile(LPCWSTR pszIniFileName, LPWSTR pszUnknown,
                                 PARSETHEMEINIFILEPROC callback, LPVOID lpData)
{
    UXSTUB();
    (void)pszIniFileName; (void)pszUnknown; (void)callback; (void)lpData;
    return E_NOTIMPL;
}

HRESULT WINAPI GetThemeParseErrorInfo(PPARSE_ERROR_INFO pInfo)
{
    UXSTUB();
    (void)pInfo;
    return E_NOTIMPL;
}

HRESULT WINAPI BufferedPaintInit(void)
{
    UXSTUB();
    return E_NOTIMPL;
}
HRESULT WINAPI BufferedPaintUnInit(void)
{
    UXSTUB();
    return E_NOTIMPL;
}

HPAINTBUFFER WINAPI BeginBufferedPaint(HDC hdcTarget, const RECT* prcTarget,
                                       BP_BUFFERFORMAT dwFormat,
                                       BP_PAINTPARAMS* pPaintParams, HDC* phdc)
{
    UXSTUB();
    (void)hdcTarget; (void)prcTarget; (void)dwFormat; (void)pPaintParams;
    if (phdc)
        *phdc = NULL;
    return NULL;
}

HRESULT WINAPI EndBufferedPaint(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget)
{
    UXSTUB();
    (void)hBufferedPaint; (void)fUpdateTarget;
    return E_NOTIMPL;
}

HRESULT WINAPI BufferedPaintClear(HPAINTBUFFER hBufferedPaint, const RECT* prc)
{
    UXSTUB();
    (void)hBufferedPaint; (void)prc;
    return E_NOTIMPL;
}

HRESULT WINAPI BufferedPaintSetAlpha(HPAINTBUFFER hBufferedPaint, const RECT* prc,
                                     BYTE alpha)
{
    UXSTUB();
    (void)hBufferedPaint; (void)prc; (void)alpha;
    return E_NOTIMPL;
}

HRESULT WINAPI GetBufferedPaintBits(HPAINTBUFFER hBufferedPaint, RGBQUAD** ppbBuffer,
                                    int* pcxRow)
{
    UXSTUB();
    (void)hBufferedPaint; (void)ppbBuffer; (void)pcxRow;
    return E_NOTIMPL;
}

HDC WINAPI GetBufferedPaintDC(HPAINTBUFFER hBufferedPaint)
{
    UXSTUB();
    (void)hBufferedPaint;
    return NULL;
}

HDC WINAPI GetBufferedPaintTargetDC(HPAINTBUFFER hBufferedPaint)
{
    UXSTUB();
    (void)hBufferedPaint;
    return NULL;
}

HRESULT WINAPI GetBufferedPaintTargetRect(HPAINTBUFFER hBufferedPaint, RECT* prc)
{
    UXSTUB();
    (void)hBufferedPaint; (void)prc;
    return E_NOTIMPL;
}

HANIMATIONBUFFER WINAPI BeginBufferedAnimation(HWND hwnd, HDC hdcTarget,
                                               const RECT* prcTarget,
                                               BP_BUFFERFORMAT dwFormat,
                                               BP_PAINTPARAMS* pPaintParams,
                                               BP_ANIMATIONPARAMS* pAnimationParams,
                                               HDC* phdcFrom, HDC* phdcTo)
{
    UXSTUB();
    (void)hwnd; (void)hdcTarget; (void)prcTarget; (void)dwFormat;
    (void)pPaintParams; (void)pAnimationParams;
    if (phdcFrom)
        *phdcFrom = NULL;
    if (phdcTo)
        *phdcTo = NULL;
    return NULL;
}

HRESULT WINAPI EndBufferedAnimation(HANIMATIONBUFFER hbpAnimation, BOOL fUpdateTarget)
{
    UXSTUB();
    (void)hbpAnimation; (void)fUpdateTarget;
    return E_NOTIMPL;
}

BOOL WINAPI BufferedPaintRenderAnimation(HWND hwnd, HDC hdcTarget)
{
    UXSTUB();
    (void)hwnd; (void)hdcTarget;
    return FALSE;
}

HRESULT WINAPI BufferedPaintStopAllAnimations(HWND hwnd)
{
    UXSTUB();
    (void)hwnd;
    return E_NOTIMPL;
}

BOOL WINAPI BeginPanningFeedback(HWND hwnd)
{
    UXSTUB();
    (void)hwnd;
    return FALSE;
}

BOOL WINAPI UpdatePanningFeedback(HWND hwnd, LONG lTotalOverpanOffsetX,
                                  LONG lTotalOverpanOffsetY, BOOL fInInertia)
{
    UXSTUB();
    (void)hwnd; (void)lTotalOverpanOffsetX; (void)lTotalOverpanOffsetY; (void)fInInertia;
    return FALSE;
}

BOOL WINAPI EndPanningFeedback(HWND hwnd, BOOL fAnimateBack)
{
    UXSTUB();
    (void)hwnd; (void)fAnimateBack;
    return FALSE;
}

}
