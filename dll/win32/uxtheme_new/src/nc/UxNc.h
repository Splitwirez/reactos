/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Non-client layer private header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#ifndef _UXTHEME_NEW_NC_H_
#define _UXTHEME_NEW_NC_H_

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <undocuser.h>
#include <uxundoc.h>
#include <strsafe.h>

#define UXNC_NOOP_PRINTF(...) do { if (0) (void)wprintf(L""); } while (0)
#ifndef TRACE
#define TRACE(...)  UXNC_NOOP_PRINTF(__VA_ARGS__)
#endif
#ifndef WARN
#define WARN(...)   UXNC_NOOP_PRINTF(__VA_ARGS__)
#endif
#ifndef ERR
#define ERR(...)    UXNC_NOOP_PRINTF(__VA_ARGS__)
#endif
#ifndef FIXME
#define FIXME(...)  UXNC_NOOP_PRINTF(__VA_ARGS__)
#endif

#ifndef WM_UAHINIT
#define WM_UAHINIT 0x031F
#endif

#ifndef WM_CTLCOLORMSGBOX
#define WM_CTLCOLORMSGBOX 0x0132
#endif

enum SCROLL_HITTEST
{
    SCROLL_NOWHERE,
    SCROLL_TOP_ARROW,
    SCROLL_TOP_RECT,
    SCROLL_THUMB,
    SCROLL_BOTTOM_RECT,
    SCROLL_BOTTOM_ARROW
};

typedef struct _WND_DATA
{
    HTHEME hthemeWindow;
    HTHEME hthemeScrollbar;

    RECT rcCaptionButtons[4];
    UINT lastHitTest;
    BOOL HasAppDefinedRgn;
    BOOL HasThemeRgn;
    BOOL UpdatingRgn;
    BOOL DirtyThemeRegion;
    HBRUSH hTabBackgroundBrush;
    HBITMAP hTabBackgroundBmp;

    BOOL SCROLL_trackVertical;
    enum SCROLL_HITTEST SCROLL_trackHitTest;
    BOOL SCROLL_MovingThumb;
    HWND SCROLL_TrackingWin;
    INT  SCROLL_TrackingBar;
    INT  SCROLL_TrackingPos;
    INT  SCROLL_TrackingVal;
} WND_DATA, *PWND_DATA;

typedef struct _DRAW_CONTEXT
{
    HWND hWnd;
    HDC hDC;
    HTHEME theme;
    HTHEME scrolltheme;
    HTHEME hPrevTheme;
    WINDOWINFO wi;
    BOOL Active;
    HRGN hRgn;
    int CaptionHeight;

    HDC hDCScreen;
    HBITMAP hbmpOld;
} DRAW_CONTEXT, *PDRAW_CONTEXT;

typedef enum
{
    CLOSEBUTTON,
    MAXBUTTON,
    MINBUTTON,
    HELPBUTTON
} CAPTIONBUTTON;

typedef enum {
    BUTTON_NORMAL = 1,
    BUTTON_HOT,
    BUTTON_PRESSED,
    BUTTON_DISABLED,
    BUTTON_INACTIVE,
    BUTTON_INACTIVE_HOT,
    BUTTON_INACTIVE_PRESSED,
    BUTTON_INACTIVE_DISABLED
} THEME_BUTTON_STATES;

#define HT_ISBUTTON(ht) ((ht) == HTMINBUTTON || (ht) == HTMAXBUTTON || \
                         (ht) == HTCLOSE || (ht) == HTHELP)

#define HASSIZEGRIP(Style, ExStyle, ParentStyle, WindowRect, ParentClientRect) \
            ((!(Style & WS_CHILD) && (Style & WS_THICKFRAME) && !(Style & WS_MAXIMIZE))  || \
             ((Style & WS_CHILD) && (ParentStyle & WS_THICKFRAME) && !(ParentStyle & WS_MAXIMIZE) && \
             (WindowRect.right - WindowRect.left == ParentClientRect.right) && \
             (WindowRect.bottom - WindowRect.top == ParentClientRect.bottom)))

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))

#define BUTTON_GAP_SIZE 2
#define MENU_BAR_ITEMS_SPACE (12)

#define SCROLL_TIMER   0
#define SCROLL_ARROW_THUMB_OVERLAP 0
#define SCROLL_FIRST_DELAY   200
#define SCROLL_REPEAT_DELAY  50
#define SCROLL_MIN_THUMB 6
#define SCROLL_MIN_RECT  4

#include "UxNcRes.h"

LRESULT CALLBACK ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam,
                              WNDPROC DefWndProc);
void ThemeCalculateCaptionButtonsPos(HWND hWnd, HTHEME htheme);
LONG SCROLL_getObjectId(INT nBar);
void ThemeDrawScrollBarEx(PDRAW_CONTEXT pcontext, INT nBar, PSCROLLBARINFO psbi, POINT* pt);
void ThemeDrawScrollBar(PDRAW_CONTEXT pcontext, INT Bar, POINT* pt);
VOID NC_TrackScrollBar(HWND Wnd, WPARAM wParam, POINT Pt);
void ThemeInitDrawContext(PDRAW_CONTEXT pcontext, HWND hWnd, HRGN hRgn);
void ThemeCleanupDrawContext(PDRAW_CONTEXT pcontext);
PWND_DATA ThemeGetWndData(HWND hWnd);
void ThemeDestroyWndData(HWND hWnd);
HTHEME GetNCCaptionTheme(HWND hWnd, DWORD style);
HTHEME GetNCScrollbarTheme(HWND hWnd, DWORD style);

extern USERAPIHOOK g_user32ApiHook;
extern HINSTANCE hDllInst;
extern ATOM atWindowTheme;
extern ATOM atWndContext;
extern BOOL g_bThemeHooksActive;

BOOL CALLBACK UXTHEME_broadcast_theme_changed(HWND hWnd, LPARAM enable);

void UXTHEME_LoadTheme(BOOL bLoad);

HRESULT UXTHEME_LoadImage(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                          const RECT *pRect, BOOL glyph, HBITMAP *phBitmap,
                          RECT *pRectDst, BOOL *pfHasAlpha);

void UxNcInitSystem(HINSTANCE hInst);
void UxNcUnInitSystem(void);

#endif
