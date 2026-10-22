/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme image decoding
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"

class CUxImage
{
public:
    CUxImage() : m_pb(NULL), m_cx(0), m_cy(0) { }
    ~CUxImage() { Free(); }

    void Free()
    {
        ThemeHeapFree(m_pb);
        m_pb = NULL;
        m_cx = 0;
        m_cy = 0;
    }

    bool  IsValid() const { return m_pb != NULL && m_cx > 0 && m_cy > 0; }
    int   Width() const { return m_cx; }
    int   Height() const { return m_cy; }
    BYTE* Bits() { return m_pb; }
    const BYTE* Bits() const { return m_pb; }
    int   Stride() const { return m_cx * 4; }

    HRESULT Create(int cx, int cy);

    void Attach(BYTE* pb, int cx, int cy)
    {
        Free();
        m_pb = pb;
        m_cx = cx;
        m_cy = cy;
    }

private:
    CUxImage(const CUxImage&);
    CUxImage& operator=(const CUxImage&);

    BYTE* m_pb;
    int   m_cx;
    int   m_cy;
};

HRESULT UxDecodeImage(const void* pvBits, DWORD cbBits, CUxImage* pOut);

class CCachedImage
{
public:
    CCachedImage() : uResId(0) { }

    UINT     uResId;
    CUxImage image;

private:
    CCachedImage(const CCachedImage&);
    CCachedImage& operator=(const CCachedImage&);
};

class CImageCache
{
public:
    CUxImage* GetOrDecode(UINT uResId, const void* pvBits, DWORD cbBits);

private:
    CArray<CCachedImage> m_images;
};

void UxDecodeTerm(void);
