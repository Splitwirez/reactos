/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme background drawing
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "../api/ThemeApi.hpp"
#include "../image/Atlas.hpp"
#include "../model/Tmt.hpp"

typedef BOOL (WINAPI *PFN_AlphaBlend)(HDC, int, int, int, int,
                                      HDC, int, int, int, int, BLENDFUNCTION);

static PFN_AlphaBlend g_pfnAlphaBlend = NULL;
static BOOL           g_fAlphaBlendTried = FALSE;

static PFN_AlphaBlend GetAlphaBlend(void)
{
    if (g_fAlphaBlendTried)
        return g_pfnAlphaBlend;

    g_fAlphaBlendTried = TRUE;

    {
        HMODULE h = GetModuleHandleW(L"gdi32.dll");
        if (h)
            g_pfnAlphaBlend = (PFN_AlphaBlend)GetProcAddress(h, "GdiAlphaBlend");
    }

    if (!g_pfnAlphaBlend)
    {
        HMODULE h = GetModuleHandleW(L"msimg32.dll");
        if (!h)
            h = LoadLibraryW(L"msimg32.dll");
        if (h)
            g_pfnAlphaBlend = (PFN_AlphaBlend)GetProcAddress(h, "AlphaBlend");
    }

    if (!g_pfnAlphaBlend)
        UXTRACE(("uxtheme_new: no AlphaBlend - images cannot be drawn\n"));

    return g_pfnAlphaBlend;
}

class CImageDC
{
public:
    CImageDC() : m_hdc(NULL), m_hbm(NULL), m_hbmOld(NULL) { }
    ~CImageDC() { Free(); }

    HRESULT Create(HDC hdcCompat, const CUxImage* pBackground)
    {
        BITMAPINFO bmi;
        void* pvBits = NULL;

        Free();

        m_hdc = CreateCompatibleDC(hdcCompat);
        if (!m_hdc)
            return MakeErrorLast();

        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = pBackground->Width();
        bmi.bmiHeader.biHeight = -pBackground->Height();
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        m_hbm = CreateDIBSection(m_hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        if (!m_hbm || !pvBits)
        {
            Free();
            return E_OUTOFMEMORY;
        }

        memcpy(pvBits, pBackground->Bits(),
               (SIZE_T)pBackground->Height() * (SIZE_T)pBackground->Stride());

        m_hbmOld = (HBITMAP)SelectObject(m_hdc, m_hbm);
        return S_OK;
    }

    HDC Get() const { return m_hdc; }

    void Free()
    {
        if (m_hdc)
        {
            if (m_hbmOld)
                SelectObject(m_hdc, m_hbmOld);
            DeleteDC(m_hdc);
            m_hdc = NULL;
            m_hbmOld = NULL;
        }
        if (m_hbm)
        {
            DeleteObject(m_hbm);
            m_hbm = NULL;
        }
    }

private:
    CImageDC(const CImageDC&);
    CImageDC& operator=(const CImageDC&);

    HDC     m_hdc;
    HBITMAP m_hbm;
    HBITMAP m_hbmOld;
};

static BOOL BlendCell(HDC hdcDst, const RECT* prcDst,
                      HDC hdcSrc, const RECT* prcSrc)
{
    PFN_AlphaBlend pfn = GetAlphaBlend();
    BLENDFUNCTION bf;
    int cxDst = prcDst->right - prcDst->left;
    int cyDst = prcDst->bottom - prcDst->top;
    int cxSrc = prcSrc->right - prcSrc->left;
    int cySrc = prcSrc->bottom - prcSrc->top;

    if (cxDst <= 0 || cyDst <= 0 || cxSrc <= 0 || cySrc <= 0)
        return TRUE;

    if (!pfn)
        return FALSE;

    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 255;
    bf.AlphaFormat = AC_SRC_ALPHA;

    if (!pfn(hdcDst, prcDst->left, prcDst->top, cxDst, cyDst,
             hdcSrc, prcSrc->left, prcSrc->top, cxSrc, cySrc, bf))
    {
        static LONG s_fSaid = 0;
        if (!s_fSaid)
        {
            s_fSaid = 1;
            UxTrace("uxtheme_new: AlphaBlend FAILED (err %lu) dst %dx%d src %dx%d\n",
                    GetLastError(), cxDst, cyDst, cxSrc, cySrc);
        }
        return FALSE;
    }

    return TRUE;
}

static HRESULT DrawNineGrid(HDC hdcDst, const RECT* prcDst,
                            HDC hdcSrc, const RECT* prcSrc,
                            const MARGINS* pmar)
{
    MARGINS mSrc = *pmar;
    MARGINS mDst;
    int cxDst = prcDst->right - prcDst->left;
    int cyDst = prcDst->bottom - prcDst->top;
    int cxSrc = prcSrc->right - prcSrc->left;
    int cySrc = prcSrc->bottom - prcSrc->top;
    int ix, iy;
    int rgxDst[4], rgyDst[4], rgxSrc[4], rgySrc[4];

    if (mSrc.cxLeftWidth + mSrc.cxRightWidth > cxSrc &&
        mSrc.cxLeftWidth + mSrc.cxRightWidth > 0)
    {
        int cxTotal = mSrc.cxLeftWidth + mSrc.cxRightWidth;
        mSrc.cxLeftWidth = MulDiv(mSrc.cxLeftWidth, cxSrc, cxTotal);
        mSrc.cxRightWidth = cxSrc - mSrc.cxLeftWidth;
    }
    if (mSrc.cyTopHeight + mSrc.cyBottomHeight > cySrc &&
        mSrc.cyTopHeight + mSrc.cyBottomHeight > 0)
    {
        int cyTotal = mSrc.cyTopHeight + mSrc.cyBottomHeight;
        mSrc.cyTopHeight = MulDiv(mSrc.cyTopHeight, cySrc, cyTotal);
        mSrc.cyBottomHeight = cySrc - mSrc.cyTopHeight;
    }

    mDst = mSrc;

    if (mDst.cxLeftWidth + mDst.cxRightWidth > cxDst &&
        mDst.cxLeftWidth + mDst.cxRightWidth > 0)
    {
        int cxTotal = mDst.cxLeftWidth + mDst.cxRightWidth;
        mDst.cxLeftWidth = MulDiv(mDst.cxLeftWidth, cxDst, cxTotal);
        mDst.cxRightWidth = cxDst - mDst.cxLeftWidth;
    }
    if (mDst.cyTopHeight + mDst.cyBottomHeight > cyDst &&
        mDst.cyTopHeight + mDst.cyBottomHeight > 0)
    {
        int cyTotal = mDst.cyTopHeight + mDst.cyBottomHeight;
        mDst.cyTopHeight = MulDiv(mDst.cyTopHeight, cyDst, cyTotal);
        mDst.cyBottomHeight = cyDst - mDst.cyTopHeight;
    }

    rgxDst[0] = prcDst->left;
    rgxDst[1] = prcDst->left + mDst.cxLeftWidth;
    rgxDst[2] = prcDst->right - mDst.cxRightWidth;
    rgxDst[3] = prcDst->right;

    rgyDst[0] = prcDst->top;
    rgyDst[1] = prcDst->top + mDst.cyTopHeight;
    rgyDst[2] = prcDst->bottom - mDst.cyBottomHeight;
    rgyDst[3] = prcDst->bottom;

    rgxSrc[0] = prcSrc->left;
    rgxSrc[1] = prcSrc->left + mSrc.cxLeftWidth;
    rgxSrc[2] = prcSrc->right - mSrc.cxRightWidth;
    rgxSrc[3] = prcSrc->right;

    rgySrc[0] = prcSrc->top;
    rgySrc[1] = prcSrc->top + mSrc.cyTopHeight;
    rgySrc[2] = prcSrc->bottom - mSrc.cyBottomHeight;
    rgySrc[3] = prcSrc->bottom;

    for (iy = 0; iy < 3; iy++)
    {
        for (ix = 0; ix < 3; ix++)
        {
            RECT rcD, rcS;

            rcD.left = rgxDst[ix];
            rcD.right = rgxDst[ix + 1];
            rcD.top = rgyDst[iy];
            rcD.bottom = rgyDst[iy + 1];

            rcS.left = rgxSrc[ix];
            rcS.right = rgxSrc[ix + 1];
            rcS.top = rgySrc[iy];
            rcS.bottom = rgySrc[iy + 1];

            BlendCell(hdcDst, &rcD, hdcSrc, &rcS);
        }
    }

    return S_OK;
}

static HRESULT DrawBorderFill(HDC hdc, const RECT* prc, const UXDRAWINFO* pInfo)
{
    HBRUSH hbr;
    RECT rc = *prc;

    if (pInfo->eBgType == BT_NONE)
        return S_OK;

    if (pInfo->eFillType == FT_SOLID)
    {
        hbr = CreateSolidBrush(pInfo->crFill);
        if (hbr)
        {
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);
        }
    }
    else
    {
        UXGAP("FILLTYPE gradient/tile - area left unpainted");
    }

    if (pInfo->eBorderType == BT_RECT && pInfo->iBorderSize > 0)
    {
        hbr = CreateSolidBrush(pInfo->crBorder);
        if (hbr)
        {
            RECT rcEdge;
            int cb = pInfo->iBorderSize;

            rcEdge = rc; rcEdge.bottom = rcEdge.top + cb;    FillRect(hdc, &rcEdge, hbr);
            rcEdge = rc; rcEdge.top = rcEdge.bottom - cb;    FillRect(hdc, &rcEdge, hbr);
            rcEdge = rc; rcEdge.right = rcEdge.left + cb;    FillRect(hdc, &rcEdge, hbr);
            rcEdge = rc; rcEdge.left = rcEdge.right - cb;    FillRect(hdc, &rcEdge, hbr);

            DeleteObject(hbr);
        }
    }

    return S_OK;
}

static void AlignGlyphRectAxis(
    _In_ int alignment,
    //_In_ int glyphExtent,
    _In_ int glyphSrcNear,
    _In_ int glyphSrcFar,
    _In_ int boundSrcNear,
    _In_ int boundSrcFar,
    _Out_ long int* glyphDstNear,
    _Out_ long int* glyphDstFar
)
{
    int glyphSrcExtent = glyphSrcFar - glyphSrcNear;

    if (alignment == 0) //HA_LEFT, VA_TOP
    {
        *glyphDstNear = boundSrcNear;
        *glyphDstFar = boundSrcNear + glyphSrcExtent;
    }
    else if (alignment == 2) //HA_RIGHT, VA_BOTTOM
    {
        *glyphDstNear = boundSrcFar - glyphSrcExtent;
        *glyphDstFar = boundSrcFar;
    }
    else
    {
        int boundSrcCenter = boundSrcNear + ((boundSrcFar - boundSrcNear) / 2);
        int glyphSrcExtentHalfNear = glyphSrcExtent / 2;
        int glyphSrcExtentHalfFar = glyphSrcExtentHalfNear;

        if ((glyphSrcExtentHalfFar * 2) < glyphSrcExtent)
            glyphSrcExtentHalfFar = glyphSrcExtentHalfFar + 1;

        *glyphDstNear = boundSrcCenter - glyphSrcExtentHalfFar;
        *glyphDstFar = boundSrcCenter + glyphSrcExtentHalfFar;
    }
}


extern "C" {

HRESULT WINAPI DrawThemeBackgroundEx(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                     const RECT* pRect, const DTBGOPTS* pOptions)
{
    const DTBGOPTS defaultOpts = {sizeof(DTBGOPTS), 0, {0,0,0,0}};
    const DTBGOPTS opts = pOptions != nullptr
        ? *pOptions
        : defaultOpts
    ;

    if (opts.dwFlags & ~(DWORD)DTBG_CLIPRECT)
        UXGAP("DTBG_* flags other than CLIPRECT are ignored");

    THEMEHANDLE* p;
    UXDRAWINFO info;
    CUxImage* pBackground = NULL;
    CUxImage* pGlyph = NULL;
    CImageDC srcDC;
    CImageDC srcGlyphDC;
    HRESULT hr;
    int iSaved = 0;

    RECT glyphRect = { 0, 0, 0, 0 };

    if (!pRect)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    hr = UxResolveDrawInfo(p->pFile, p->iClass, iPartId, iStateId, &info, &pBackground, &pGlyph);

    static LONG s_cDraws = 0;
    LONG n = InterlockedIncrement(&s_cDraws);

    if (n <= 300)
    {
        const WCHAR* pszClass = L"?";
        if (p->iClass >= 0 && p->iClass < p->pFile->classes.GetCount())
            pszClass = p->pFile->classes[p->iClass].strFullName.Get();

        UxTrace("uxtheme_new: draw %-28ls part %-3d state %-3d "
                "dst %ld,%ld %ldx%ld  src %ld,%ld %ldx%ld  res %u  "
                "bg %d cnt %d lay %d  mar %d,%d,%d,%d%s\n",
                pszClass, iPartId, iStateId,
                pRect->left, pRect->top,
                pRect->right - pRect->left, pRect->bottom - pRect->top,
                info.rcSrc.left, info.rcSrc.top,
                info.rcSrc.right - info.rcSrc.left,
                info.rcSrc.bottom - info.rcSrc.top,
                pBackground ? 1u : 0u,
                info.eBgType, info.iImageCount, info.eImageLayout,
                info.marSizing.cxLeftWidth, info.marSizing.cxRightWidth,
                info.marSizing.cyTopHeight, info.marSizing.cyBottomHeight,
                FAILED(hr) ? "  <- RESOLVE FAILED" : "");
    }

    if (FAILED(hr))
        return hr;

    if (opts.dwFlags & DTBG_CLIPRECT)
    {
        iSaved = SaveDC(hdc);
        IntersectClipRect(hdc, opts.rcClip.left, opts.rcClip.top,
                          opts.rcClip.right, opts.rcClip.bottom);
    }

    if (info.eBgType != BT_IMAGEFILE || !pBackground)
    {
        hr = DrawBorderFill(hdc, pRect, &info);
    }
    else
    {
        hr = srcDC.Create(hdc, pBackground);
        if (SUCCEEDED(hr))
        {
            if (info.fSizingMargins)
            {
                hr = DrawNineGrid(hdc, pRect, srcDC.Get(), &info.rcSrc,
                                  &info.marSizing);
            }
            else
            {
                if (info.eSizingType == ST_TILE)
                    UXGAP("SIZINGTYPE TILE - stretched instead of tiled");
                RECT rcD = *pRect;
                if (!BlendCell(hdc, &rcD, srcDC.Get(), &info.rcSrc))
                    hr = E_FAIL;
            }
        }
    }

    if (pGlyph)
    {
        switch (info.eGlyphType)
        {
            case GT_IMAGEGLYPH:
                if (pGlyph == NULL)
                    break;

                AlignGlyphRectAxis((int)info.eGlyphHAlignment,
                    info.rcGlyphSrc.left,
                    info.rcGlyphSrc.right,

                    pRect->left,
                    pRect->right,

                    &glyphRect.left,
                    &glyphRect.right
                );

                AlignGlyphRectAxis((int)info.eGlyphVAlignment,
                    info.rcGlyphSrc.top,
                    info.rcGlyphSrc.bottom,

                    pRect->top,
                    pRect->bottom,

                    &glyphRect.top,
                    &glyphRect.bottom
                );
                

                hr = srcGlyphDC.Create(hdc, pGlyph);
                if (SUCCEEDED(hr))
                {
                    if (!BlendCell(hdc, &glyphRect, srcGlyphDC.Get(), &info.rcGlyphSrc))
                        hr = E_FAIL;
                }

                break;

            case GT_FONTGLYPH:
                UXGAP("GT_FONTGLYPH unimplemented");
                break;

            default:
                break;
        }
    }

    if (iSaved)
        RestoreDC(hdc, iSaved);

    return hr;
}
HRESULT WINAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                   const RECT* pRect, const RECT* pClipRect)
{
    DTBGOPTS opts;
    opts.dwSize = sizeof(DTBGOPTS);
    opts.dwFlags = 0;
    if (pClipRect)
    {
        opts.dwFlags |= DTBG_CLIPRECT;
        opts.rcClip = *pClipRect;
    }
    return DrawThemeBackgroundEx(hTheme, hdc, iPartId, iStateId, pRect, &opts);
}

extern "C"
HRESULT WINAPI GetThemeBackgroundRegion(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT* pRect,
                                        HRGN* pRegion)
{
    THEMEHANDLE* p;
    UXDRAWINFO info;
    CUxImage* pBackground = NULL;
    HRESULT hr;
    int cx, cy;

    if (!pRect || !pRegion)
        return E_POINTER;

    *pRegion = NULL;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    cx = pRect->right - pRect->left;
    cy = pRect->bottom - pRect->top;
    if (cx <= 0 || cy <= 0)
        return E_INVALIDARG;

    hr = UxResolveDrawInfo(p->pFile, p->iClass, iPartId, iStateId, &info, &pBackground, NULL);

    if (FAILED(hr) || !pBackground || info.eBgType != BT_IMAGEFILE)
    {
        *pRegion = CreateRectRgnIndirect(pRect);
        return *pRegion ? S_OK : E_OUTOFMEMORY;
    }

    {
        HDC hdcRef = hdc ? hdc : GetDC(NULL);
        HDC hdcMem = NULL;
        HBITMAP hbm = NULL;
        HGDIOBJ hOld = NULL;
        void* pvBits = NULL;
        BITMAPINFO bmi;
        int nThreshold;

        if (!hdcRef)
            return MakeErrorLast();

        {
            const CThemeProperty* pProp = p->pFile->ResolveProperty(
                p->iClass, iPartId, iStateId, TMT_ALPHATHRESHOLD);
            nThreshold = pProp ? pProp->value.GetInt() : 0;
        }

        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cx;
        bmi.bmiHeader.biHeight = -cy;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        hdcMem = CreateCompatibleDC(hdcRef);
        if (hdcMem)
            hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

        if (hbm && pvBits)
        {
            RECT rcDraw = { 0, 0, cx, cy };

            hOld = SelectObject(hdcMem, hbm);
            memset(pvBits, 0, (SIZE_T)cx * cy * 4);

            hr = DrawThemeBackground(hTheme, hdcMem, iPartId, iStateId, &rcDraw, NULL);
            GdiFlush();

            if (SUCCEEDED(hr))
            {
                int cRectsMax = cx * cy / 2 + 8;
                SIZE_T cbData = sizeof(RGNDATAHEADER) + (SIZE_T)cRectsMax * sizeof(RECT);
                RGNDATA* prd = (RGNDATA*)ThemeHeapAlloc(0, cbData);

                if (prd)
                {
                    RECT* prc = (RECT*)prd->Buffer;
                    int cRects = 0;
                    int x, y;

                    for (y = 0; y < cy; y++)
                    {
                        const BYTE* pRow = (const BYTE*)pvBits + (SIZE_T)y * cx * 4;
                        x = 0;
                        while (x < cx)
                        {
                            int xStart;

                            while (x < cx && pRow[x * 4 + 3] <= nThreshold)
                                x++;
                            if (x >= cx)
                                break;

                            xStart = x;
                            while (x < cx && pRow[x * 4 + 3] > nThreshold)
                                x++;

                            if (cRects < cRectsMax)
                            {
                                prc[cRects].left   = pRect->left + xStart;
                                prc[cRects].top    = pRect->top + y;
                                prc[cRects].right  = pRect->left + x;
                                prc[cRects].bottom = pRect->top + y + 1;
                                cRects++;
                            }
                        }
                    }

                    memset(&prd->rdh, 0, sizeof(prd->rdh));
                    prd->rdh.dwSize = sizeof(RGNDATAHEADER);
                    prd->rdh.iType = RDH_RECTANGLES;
                    prd->rdh.nCount = cRects;
                    prd->rdh.nRgnSize = cRects * sizeof(RECT);
                    prd->rdh.rcBound = *pRect;

                    if (cRects > 0)
                        *pRegion = ExtCreateRegion(NULL,
                                                   (DWORD)(sizeof(RGNDATAHEADER) +
                                                           cRects * sizeof(RECT)),
                                                   prd);

                    ThemeHeapFree(prd);
                }
            }

            SelectObject(hdcMem, hOld);
        }

        if (hbm)
            DeleteObject(hbm);
        if (hdcMem)
            DeleteDC(hdcMem);
        if (!hdc)
            ReleaseDC(NULL, hdcRef);
    }

    if (!*pRegion)
        *pRegion = CreateRectRgnIndirect(pRect);

    return *pRegion ? S_OK : E_OUTOFMEMORY;
}

HRESULT WINAPI GetThemePartSize(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                                RECT* prc, THEMESIZE eSize, SIZE* psz)
{
    THEMEHANDLE* p;
    UXDRAWINFO info;
    CUxImage* pBackground = NULL;
    HRESULT hr;

    (void)hdc;

    if (!psz)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    hr = UxResolveDrawInfo(p->pFile, p->iClass, iPartId, iStateId, &info, &pBackground, NULL);
    if (FAILED(hr))
        return hr;

    if (eSize == TS_MIN)
    {
        const CThemeProperty* pProp =
            p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, TMT_MINSIZE);
        if (pProp)
        {
            POINT pt = pProp->value.GetPosition();
            psz->cx = pt.x;
            psz->cy = pt.y;
            return S_OK;
        }
    }

    if (!pBackground)
    {
        if (prc)
        {
            psz->cx = prc->right - prc->left;
            psz->cy = prc->bottom - prc->top;
            return S_OK;
        }
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    psz->cx = info.rcSrc.right - info.rcSrc.left;
    psz->cy = info.rcSrc.bottom - info.rcSrc.top;
    return S_OK;
}

HRESULT WINAPI GetThemeBackgroundContentRect(HTHEME hTheme, HDC hdc, int iPartId,
                                             int iStateId, const RECT* pBoundingRect,
                                             RECT* pContentRect)
{
    THEMEHANDLE* p;
    const CThemeProperty* pProp;

    (void)hdc;

    if (!pBoundingRect || !pContentRect)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    *pContentRect = *pBoundingRect;

    pProp = p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, TMT_CONTENTMARGINS);
    if (pProp)
    {
        MARGINS m = pProp->value.GetMargins();
        pContentRect->left   += m.cxLeftWidth;
        pContentRect->right  -= m.cxRightWidth;
        pContentRect->top    += m.cyTopHeight;
        pContentRect->bottom -= m.cyBottomHeight;
    }

    return S_OK;
}

HRESULT WINAPI GetThemeBackgroundExtent(HTHEME hTheme, HDC hdc, int iPartId,
                                        int iStateId, const RECT* pContentRect,
                                        RECT* pExtentRect)
{
    THEMEHANDLE* p;
    const CThemeProperty* pProp;

    (void)hdc;

    if (!pContentRect || !pExtentRect)
        return E_POINTER;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    *pExtentRect = *pContentRect;

    pProp = p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, TMT_CONTENTMARGINS);
    if (pProp)
    {
        MARGINS m = pProp->value.GetMargins();
        pExtentRect->left   -= m.cxLeftWidth;
        pExtentRect->right  += m.cxRightWidth;
        pExtentRect->top    -= m.cyTopHeight;
        pExtentRect->bottom += m.cyBottomHeight;
    }

    return S_OK;
}

BOOL WINAPI IsThemeBackgroundPartiallyTransparent(HTHEME hTheme, int iPartId, int iStateId)
{
    THEMEHANDLE* p = ValidateThemeHandle(hTheme);
    const CThemeProperty* pProp;

    if (!p || !p->pFile)
        return FALSE;

    pProp = p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, TMT_TRANSPARENT);
    return pProp ? pProp->value.GetBool() : FALSE;
}

}
