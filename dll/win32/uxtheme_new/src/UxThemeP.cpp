/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Private heap and tracing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "UxThemeP.hpp"

#include <stdio.h>
#include <stdarg.h>

static HANDLE g_hThemeHeap = NULL;

HRESULT ThemeHeapInit(void)
{
    if (g_hThemeHeap)
        return S_OK;

    g_hThemeHeap = HeapCreate(0, 0, 0);
    if (!g_hThemeHeap)
        return E_OUTOFMEMORY;

    return S_OK;
}

void ThemeHeapTerm(void)
{
    g_hThemeHeap = NULL;
}

void* ThemeHeapAlloc(DWORD dwFlags, SIZE_T cb)
{
    if (!g_hThemeHeap && FAILED(ThemeHeapInit()))
        return NULL;

    if (cb == 0)
        cb = 1;

    return HeapAlloc(g_hThemeHeap, dwFlags, cb);
}

void* ThemeHeapReAlloc(void* pv, SIZE_T cb)
{
    if (!pv)
        return ThemeHeapAlloc(0, cb);
    if (!g_hThemeHeap)
        return NULL;

    return HeapReAlloc(g_hThemeHeap, 0, pv, cb ? cb : 1);
}

void ThemeHeapFree(void* pv)
{
    if (pv && g_hThemeHeap)
        HeapFree(g_hThemeHeap, 0, pv);
}

void* operator new(size_t cb)                { return ThemeHeapAlloc(0, cb); }
void* operator new[](size_t cb)              { return ThemeHeapAlloc(0, cb); }
void  operator delete(void* pv)              { ThemeHeapFree(pv); }
void  operator delete[](void* pv)            { ThemeHeapFree(pv); }
void  operator delete(void* pv, size_t)      { ThemeHeapFree(pv); }
void  operator delete[](void* pv, size_t)    { ThemeHeapFree(pv); }

void UxTrace(const char* fmt, ...)
{
    char szBuf[1024];
    va_list ap;

    va_start(ap, fmt);
    _vsnprintf(szBuf, sizeof(szBuf) - 1, fmt, ap);
    va_end(ap);

    szBuf[sizeof(szBuf) - 1] = '\0';
    OutputDebugStringA(szBuf);
}
