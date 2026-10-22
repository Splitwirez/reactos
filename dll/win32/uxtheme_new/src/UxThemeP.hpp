/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Private header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#include <windowsx.h>
#include <winerror.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <strsafe.h>

void*  ThemeHeapAlloc(DWORD dwFlags, SIZE_T cb);
void*  ThemeHeapReAlloc(void* pv, SIZE_T cb);
void   ThemeHeapFree(void* pv);
HRESULT ThemeHeapInit(void);
void   ThemeHeapTerm(void);

struct UxPlacement { };
inline void* operator new(size_t, UxPlacement, void* pv) { return pv; }
inline void  operator delete(void*, UxPlacement, void*) { }
#define UX_NEW(pv) new (UxPlacement(), (pv))

template <class T>
class CArray
{
public:
    CArray() : m_p(NULL), m_c(0), m_cAlloc(0) { }
    ~CArray() { Clear(); }

    int      GetCount() const { return m_c; }
    bool     IsEmpty()  const { return m_c == 0; }
    T&       operator[](int i)       { return m_p[i]; }
    const T& operator[](int i) const { return m_p[i]; }
    T*       GetData()              { return m_p; }
    const T* GetData() const        { return m_p; }

    T* Add()
    {
        if (m_c == m_cAlloc && FAILED(Grow(m_cAlloc ? m_cAlloc * 2 : 8)))
            return NULL;
        T* p = &m_p[m_c++];
        UX_NEW(p) T();
        return p;
    }

    HRESULT SetCount(int c)
    {
        if (c > m_cAlloc)
        {
            HRESULT hr = Grow(c);
            if (FAILED(hr))
                return hr;
        }
        while (m_c < c)
            UX_NEW(&m_p[m_c++]) T();
        while (m_c > c)
            m_p[--m_c].~T();
        return S_OK;
    }

    void Clear()
    {
        for (int i = 0; i < m_c; i++)
            m_p[i].~T();
        ThemeHeapFree(m_p);
        m_p = NULL;
        m_c = 0;
        m_cAlloc = 0;
    }

private:
    CArray(const CArray&);
    CArray& operator=(const CArray&);

    HRESULT Grow(int cNew)
    {
        T* pNew = (T*)ThemeHeapAlloc(0, (SIZE_T)cNew * sizeof(T));
        if (!pNew)
            return E_OUTOFMEMORY;
        if (m_p)
        {
            memcpy((void*)pNew, (void*)m_p, (SIZE_T)m_c * sizeof(T));
            ThemeHeapFree(m_p);
        }
        m_p = pNew;
        m_cAlloc = cNew;
        return S_OK;
    }

    T*  m_p;
    int m_c;
    int m_cAlloc;
};

class CStringW
{
public:
    CStringW() : m_psz(NULL), m_cch(0), m_cchAlloc(0) { }
    ~CStringW() { Free(); }

    CStringW(const CStringW& o) : m_psz(NULL), m_cch(0), m_cchAlloc(0) { Set(o.m_psz); }
    CStringW& operator=(const CStringW& o) { if (this != &o) Set(o.m_psz); return *this; }

    void Free()
    {
        ThemeHeapFree(m_psz);
        m_psz = NULL;
        m_cch = 0;
        m_cchAlloc = 0;
    }

    const WCHAR* Get() const { return m_psz ? m_psz : L""; }
    int          Length() const { return m_cch; }
    bool         IsEmpty() const { return m_cch == 0; }

    HRESULT Set(const WCHAR* psz)
    {
        m_cch = 0;
        if (m_psz)
            m_psz[0] = L'\0';
        if (!psz)
            return S_OK;
        return Append(psz, (int)wcslen(psz));
    }

    HRESULT Append(const WCHAR* psz, int cch)
    {
        HRESULT hr = Reserve(m_cch + cch + 1);
        if (FAILED(hr))
            return hr;
        memcpy(m_psz + m_cch, psz, (SIZE_T)cch * sizeof(WCHAR));
        m_cch += cch;
        m_psz[m_cch] = L'\0';
        return S_OK;
    }

    HRESULT AppendChar(WCHAR ch) { return Append(&ch, 1); }

    bool EqualsI(const WCHAR* psz) const
    {
        if (!psz)
            return m_cch == 0;
        return _wcsicmp(Get(), psz) == 0;
    }

    void SplitClassName(CStringW& app, CStringW& cls) const
    {
        const WCHAR* p = Get();
        const WCHAR* sep = wcsstr(p, L"::");
        if (sep)
        {
            app.Set(L"");
            app.Append(p, (int)(sep - p));
            cls.Set(sep + 2);
        }
        else
        {
            app.Set(L"");
            cls.Set(p);
        }
    }

private:
    HRESULT Reserve(int cch)
    {
        if (cch <= m_cchAlloc)
            return S_OK;
        int cNew = m_cchAlloc ? m_cchAlloc : 16;
        while (cNew < cch)
            cNew *= 2;
        WCHAR* pNew = (WCHAR*)ThemeHeapAlloc(0, (SIZE_T)cNew * sizeof(WCHAR));
        if (!pNew)
            return E_OUTOFMEMORY;
        if (m_psz)
        {
            memcpy(pNew, m_psz, (SIZE_T)(m_cch + 1) * sizeof(WCHAR));
            ThemeHeapFree(m_psz);
        }
        else
        {
            pNew[0] = L'\0';
        }
        m_psz = pNew;
        m_cchAlloc = cNew;
        return S_OK;
    }

    WCHAR* m_psz;
    int    m_cch;
    int    m_cchAlloc;
};

void UxTrace(const char* fmt, ...);

#if DBG
#define UXTRACE(x) UxTrace x
#else
#define UXTRACE(x) ((void)0)
#endif

#define UXSTUB()                                                               \
    do {                                                                       \
        static LONG s_fSaid = 0;                                               \
        if (!s_fSaid) {                                                        \
            s_fSaid = 1;                                                       \
            UxTrace("uxtheme_new: STUB %s - returned without doing anything\n", \
                    __FUNCTION__);                                             \
        }                                                                      \
    } while (0)

#define UXGAP(what)                                                            \
    do {                                                                       \
        static LONG s_fSaid = 0;                                               \
        if (!s_fSaid) {                                                        \
            s_fSaid = 1;                                                       \
            UxTrace("uxtheme_new: NOT IMPLEMENTED in %s: %s\n",                \
                    __FUNCTION__, (what));                                     \
        }                                                                      \
    } while (0)
