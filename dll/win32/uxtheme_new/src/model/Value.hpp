/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme property values
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include "../UxThemeP.hpp"

enum TPID
{
    TPID_INVALID = -1,
    TPID_BITMAPIMAGE = 0,
    TPID_BITMAPIMAGE1,
    TPID_BITMAPIMAGE2,
    TPID_BITMAPIMAGE3,
    TPID_BITMAPIMAGE4,
    TPID_BITMAPIMAGE5,
    TPID_BITMAPIMAGE6,
    TPID_BITMAPIMAGE7,
    TPID_STOCKBITMAPIMAGE,
    TPID_GLYPHIMAGE,
    TPID_ATLASINPUTIMAGE,
    TPID_ATLASIMAGE,
    TPID_ENUM,
    TPID_STRING,
    TPID_INT,
    TPID_BOOL,
    TPID_COLOR,
    TPID_MARGINS,
    TPID_FILENAME,
    TPID_SIZE,
    TPID_POSITION,
    TPID_RECT,
    TPID_FONT,
    TPID_INTLIST,
    TPID_DISKSTREAM,
    TPID_STREAM,
    TPID_ANIMATION,
    TPID_TIMINGFUNCTION,
    TPID_SIMPLIFIEDIMAGE,
    TPID_HIGHCONTRASTCOLORTYPE,
    TPID_BITMAPIMAGETYPE,
    TPID_COMPOSEDIMAGETYPE,
    TPID_FLOAT,
    TPID_FLOATLIST,
};

class CValue
{
public:
    CValue() : m_kind(TPID_INVALID), m_pb(NULL), m_cb(0), m_resId(0)
    {
        memset(&m_u, 0, sizeof(m_u));
    }

    ~CValue() { Free(); }

    void Free()
    {
        ThemeHeapFree(m_pb);
        m_pb = NULL;
        m_cb = 0;
    }

    TPID Kind() const { return m_kind; }
    void SetKind(TPID k) { m_kind = k; }
    bool IsNone() const { return m_kind == TPID_INVALID; }

    void SetInt(int v)        { m_u.i = v; }
    void SetBool(BOOL v)      { m_u.b = v; }
    void SetColor(COLORREF v) { m_u.cr = v; }
    void SetFloat(float v)    { m_u.f = v; }
    void SetMargins(const MARGINS& v) { m_u.mar = v; }
    void SetRect(const RECT& v)       { m_u.rc = v; }
    void SetPosition(const POINT& v)  { m_u.pt = v; }

    int       GetInt()      const { return m_u.i; }
    BOOL      GetBool()     const { return m_u.b; }
    COLORREF  GetColor()    const { return m_u.cr; }
    float     GetFloat()    const { return m_u.f; }
    const MARGINS& GetMargins()  const { return m_u.mar; }
    const RECT&    GetRect()     const { return m_u.rc; }
    const POINT&   GetPosition() const { return m_u.pt; }

    HRESULT SetData(const void* pv, int cb)
    {
        Free();
        if (cb <= 0)
            return S_OK;
        m_pb = (BYTE*)ThemeHeapAlloc(0, (SIZE_T)cb);
        if (!m_pb)
            return E_OUTOFMEMORY;
        memcpy(m_pb, pv, (SIZE_T)cb);
        m_cb = cb;
        return S_OK;
    }

    void AttachData(BYTE* pb, int cb)
    {
        Free();
        m_pb = pb;
        m_cb = cb;
    }

    const BYTE* GetData() const { return m_pb; }
    int         GetDataSize() const { return m_cb; }

    const WCHAR* GetString() const { return m_pb ? (const WCHAR*)m_pb : L""; }

    const int* GetIntList() const { return (const int*)m_pb; }
    int        GetIntListCount() const { return m_cb / (int)sizeof(int); }

    const LOGFONTW* GetFont() const
    {
        return (m_pb && m_cb >= (int)sizeof(LOGFONTW)) ? (const LOGFONTW*)m_pb : NULL;
    }

    UINT GetResourceId() const { return m_resId; }
    void SetResourceId(UINT id) { m_resId = id; }

private:
    CValue(const CValue&);
    CValue& operator=(const CValue&);

    TPID m_kind;

    union
    {
        int      i;
        BOOL     b;
        COLORREF cr;
        float    f;
        MARGINS  mar;
        RECT     rc;
        POINT    pt;
    } m_u;

    BYTE* m_pb;
    int   m_cb;
    UINT  m_resId;
};
