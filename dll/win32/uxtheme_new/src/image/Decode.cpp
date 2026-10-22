/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme image decoding
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "Decode.hpp"

HRESULT CUxImage::Create(int cx, int cy)
{
    SIZE_T cb;

    Free();

    if (cx <= 0 || cy <= 0)
        return E_INVALIDARG;

    if (cx > 0x10000 || cy > 0x10000)
        return E_INVALIDARG;

    cb = (SIZE_T)cx * (SIZE_T)cy * 4;
    m_pb = (BYTE*)ThemeHeapAlloc(HEAP_ZERO_MEMORY, cb);
    if (!m_pb)
        return E_OUTOFMEMORY;

    m_cx = cx;
    m_cy = cy;
    return S_OK;
}

static HRESULT DecodePackedDib(const void* pvBits, CUxImage* pOut)
{
    const BITMAPINFOHEADER* pbih = (const BITMAPINFOHEADER*)pvBits;
    const BYTE* pbSrc;
    int cx, cy, cbSrcStride;
    BOOL fTopDown;
    HRESULT hr;
    int y, x;

    if (pbih->biSize < sizeof(BITMAPINFOHEADER))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (pbih->biCompression != BI_RGB)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    if (pbih->biBitCount != 32 && pbih->biBitCount != 24)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    cx = pbih->biWidth;
    cy = pbih->biHeight;

    fTopDown = (cy < 0);
    if (fTopDown)
        cy = -cy;

    hr = pOut->Create(cx, cy);
    if (FAILED(hr))
        return hr;

    cbSrcStride = ((cx * pbih->biBitCount + 31) / 32) * 4;
    pbSrc = (const BYTE*)pvBits + pbih->biSize;

    for (y = 0; y < cy; y++)
    {
        const BYTE* pRow = pbSrc + (SIZE_T)(fTopDown ? y : (cy - 1 - y)) * cbSrcStride;
        BYTE* pDst = pOut->Bits() + (SIZE_T)y * pOut->Stride();

        if (pbih->biBitCount == 32)
        {
            memcpy(pDst, pRow, (SIZE_T)cx * 4);
        }
        else
        {
            for (x = 0; x < cx; x++)
            {
                pDst[x * 4 + 0] = pRow[x * 3 + 0];
                pDst[x * 4 + 1] = pRow[x * 3 + 1];
                pDst[x * 4 + 2] = pRow[x * 3 + 2];
                pDst[x * 4 + 3] = 0xFF;
            }
        }
    }

    return S_OK;
}

typedef struct _GPSTARTUPINPUT
{
    UINT32 GdiplusVersion;
    void*  DebugEventCallback;
    BOOL   SuppressBackgroundThread;
    BOOL   SuppressExternalCodecs;
} GPSTARTUPINPUT;

typedef struct _GPRECTI { INT X, Y, Width, Height; } GPRECTI;

typedef struct _GPBITMAPDATA
{
    UINT     Width;
    UINT     Height;
    INT      Stride;
    INT      PixelFormat;
    void*    Scan0;
    UINT_PTR Reserved;
} GPBITMAPDATA;

#define GP_PF_32BPP_PARGB   0x000E200B
#define GP_IMGLOCK_READ     0x0001

typedef INT  (WINAPI *PFN_GdiplusStartup)(ULONG_PTR*, const GPSTARTUPINPUT*, void*);
typedef void (WINAPI *PFN_GdiplusShutdown)(ULONG_PTR);
typedef INT  (WINAPI *PFN_GdipCreateBitmapFromStream)(IStream*, void**);
typedef INT  (WINAPI *PFN_GdipGetImageWidth)(void*, UINT*);
typedef INT  (WINAPI *PFN_GdipGetImageHeight)(void*, UINT*);
typedef INT  (WINAPI *PFN_GdipBitmapLockBits)(void*, const GPRECTI*, UINT, INT, GPBITMAPDATA*);
typedef INT  (WINAPI *PFN_GdipBitmapUnlockBits)(void*, GPBITMAPDATA*);
typedef INT  (WINAPI *PFN_GdipDisposeImage)(void*);

typedef IStream* (WINAPI *PFN_SHCreateMemStream)(const BYTE*, UINT);

struct GDIPLUSFNS
{
    HMODULE hGdiPlus;
    HMODULE hShlwapi;
    ULONG_PTR ulToken;

    PFN_GdiplusStartup             pfnStartup;
    PFN_GdiplusShutdown            pfnShutdown;
    PFN_GdipCreateBitmapFromStream pfnFromStream;
    PFN_GdipGetImageWidth          pfnGetWidth;
    PFN_GdipGetImageHeight         pfnGetHeight;
    PFN_GdipBitmapLockBits         pfnLockBits;
    PFN_GdipBitmapUnlockBits       pfnUnlockBits;
    PFN_GdipDisposeImage           pfnDispose;
    PFN_SHCreateMemStream          pfnCreateMemStream;
};

static GDIPLUSFNS g_gp;
static BOOL       g_fGpTried = FALSE;

static bool EnsureGdiPlus(void)
{
    GPSTARTUPINPUT si;

    if (g_gp.ulToken)
        return true;
    if (g_fGpTried)
        return false;

    g_fGpTried = TRUE;

    g_gp.hGdiPlus = GetModuleHandleW(L"gdiplus.dll");
    if (!g_gp.hGdiPlus)
        g_gp.hGdiPlus = LoadLibraryW(L"gdiplus.dll");
    if (!g_gp.hGdiPlus)
    {
        UXTRACE(("uxtheme_new: gdiplus.dll unavailable - no image decoding\n"));
        return false;
    }

    g_gp.hShlwapi = GetModuleHandleW(L"shlwapi.dll");
    if (!g_gp.hShlwapi)
        g_gp.hShlwapi = LoadLibraryW(L"shlwapi.dll");

    g_gp.pfnStartup      = (PFN_GdiplusStartup)GetProcAddress(g_gp.hGdiPlus, "GdiplusStartup");
    g_gp.pfnShutdown     = (PFN_GdiplusShutdown)GetProcAddress(g_gp.hGdiPlus, "GdiplusShutdown");
    g_gp.pfnFromStream   = (PFN_GdipCreateBitmapFromStream)GetProcAddress(g_gp.hGdiPlus, "GdipCreateBitmapFromStream");
    g_gp.pfnGetWidth     = (PFN_GdipGetImageWidth)GetProcAddress(g_gp.hGdiPlus, "GdipGetImageWidth");
    g_gp.pfnGetHeight    = (PFN_GdipGetImageHeight)GetProcAddress(g_gp.hGdiPlus, "GdipGetImageHeight");
    g_gp.pfnLockBits     = (PFN_GdipBitmapLockBits)GetProcAddress(g_gp.hGdiPlus, "GdipBitmapLockBits");
    g_gp.pfnUnlockBits   = (PFN_GdipBitmapUnlockBits)GetProcAddress(g_gp.hGdiPlus, "GdipBitmapUnlockBits");
    g_gp.pfnDispose      = (PFN_GdipDisposeImage)GetProcAddress(g_gp.hGdiPlus, "GdipDisposeImage");

    if (g_gp.hShlwapi)
    {
        g_gp.pfnCreateMemStream =
            (PFN_SHCreateMemStream)GetProcAddress(g_gp.hShlwapi, "SHCreateMemStream");
        if (!g_gp.pfnCreateMemStream)
            g_gp.pfnCreateMemStream =
                (PFN_SHCreateMemStream)GetProcAddress(g_gp.hShlwapi, MAKEINTRESOURCEA(12));
    }

    if (!g_gp.pfnStartup || !g_gp.pfnFromStream || !g_gp.pfnGetWidth ||
        !g_gp.pfnGetHeight || !g_gp.pfnLockBits || !g_gp.pfnUnlockBits ||
        !g_gp.pfnDispose || !g_gp.pfnCreateMemStream)
    {
        UXTRACE(("uxtheme_new: gdiplus/shlwapi entry points missing\n"));
        return false;
    }

    memset(&si, 0, sizeof(si));
    si.GdiplusVersion = 1;

    if (g_gp.pfnStartup(&g_gp.ulToken, &si, NULL) != 0)
    {
        UXTRACE(("uxtheme_new: GdiplusStartup failed\n"));
        g_gp.ulToken = 0;
        return false;
    }

    return true;
}

void UxDecodeTerm(void)
{
    if (g_gp.ulToken && g_gp.pfnShutdown)
        g_gp.pfnShutdown(g_gp.ulToken);
    g_gp.ulToken = 0;
}

static HRESULT DecodeWithGdiPlus(const void* pvBits, DWORD cbBits, CUxImage* pOut)
{
    IStream* pStream;
    void* pBitmap = NULL;
    UINT cx = 0, cy = 0;
    GPRECTI rc;
    GPBITMAPDATA bd;
    HRESULT hr = E_FAIL;
    int y;

    if (!EnsureGdiPlus())
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    pStream = g_gp.pfnCreateMemStream((const BYTE*)pvBits, cbBits);
    if (!pStream)
        return E_OUTOFMEMORY;

    if (g_gp.pfnFromStream(pStream, &pBitmap) != 0 || !pBitmap)
    {
        pStream->Release();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    g_gp.pfnGetWidth(pBitmap, &cx);
    g_gp.pfnGetHeight(pBitmap, &cy);

    if (cx == 0 || cy == 0)
    {
        g_gp.pfnDispose(pBitmap);
        pStream->Release();
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }

    hr = pOut->Create((int)cx, (int)cy);
    if (SUCCEEDED(hr))
    {
        rc.X = 0;
        rc.Y = 0;
        rc.Width = (INT)cx;
        rc.Height = (INT)cy;
        memset(&bd, 0, sizeof(bd));

        if (g_gp.pfnLockBits(pBitmap, &rc, GP_IMGLOCK_READ,
                             GP_PF_32BPP_PARGB, &bd) == 0)
        {
            for (y = 0; y < (int)cy; y++)
            {
                memcpy(pOut->Bits() + (SIZE_T)y * pOut->Stride(),
                       (const BYTE*)bd.Scan0 + (SIZE_T)y * bd.Stride,
                       (SIZE_T)cx * 4);
            }
            g_gp.pfnUnlockBits(pBitmap, &bd);
            hr = S_OK;
        }
        else
        {
            pOut->Free();
            hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }

    g_gp.pfnDispose(pBitmap);
    pStream->Release();
    return hr;
}

HRESULT UxDecodeImage(const void* pvBits, DWORD cbBits, CUxImage* pOut)
{
    if (!pvBits || !pOut)
        return E_INVALIDARG;

    if (cbBits == 0)
        return DecodePackedDib(pvBits, pOut);

    return DecodeWithGdiPlus(pvBits, cbBits, pOut);
}

CUxImage* CImageCache::GetOrDecode(UINT uResId, const void* pvBits, DWORD cbBits)
{
    CCachedImage* p;
    int i;

    for (i = 0; i < m_images.GetCount(); i++)
        if (m_images[i].uResId == uResId)
            return m_images[i].image.IsValid() ? &m_images[i].image : NULL;

    p = m_images.Add();
    if (!p)
        return NULL;

    p->uResId = uResId;

    if (FAILED(UxDecodeImage(pvBits, cbBits, &p->image)))
    {
        UXTRACE(("uxtheme_new: image resource %u failed to decode\n", uResId));
        return NULL;
    }

    UXTRACE(("uxtheme_new: decoded image %u -> %dx%d\n",
             uResId, p->image.Width(), p->image.Height()));
    return &p->image;
}
