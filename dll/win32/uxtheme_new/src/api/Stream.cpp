/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     GetThemeStream
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ThemeApi.hpp"
#include "../load/VSRecord.hpp"
#include "../model/Tmt.hpp"

#include <uxundoc.h>

extern "C" {

HRESULT WINAPI GetThemeStream(HTHEME hTheme, int iPartId, int iStateId,
                              int iPropId, void** ppvStream, DWORD* pcbStream,
                              HINSTANCE hInst)
{
    THEMEHANDLE* p;
    const CThemeProperty* pProp;

    if (!ppvStream)
        return E_POINTER;

    *ppvStream = NULL;
    if (pcbStream)
        *pcbStream = 0;

    p = ValidateThemeHandle(hTheme);
    if (!p || !p->pFile)
        return E_HANDLE;

    pProp = p->pFile->ResolveProperty(p->iClass, iPartId, iStateId, iPropId);
    if (!pProp)
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (iPropId != TMT_DISKSTREAM)
    {
        *ppvStream = (void*)pProp->value.GetData();
        if (pcbStream)
            *pcbStream = (DWORD)pProp->value.GetDataSize();
        return S_OK;
    }

    if (!hInst)
        return E_INVALIDARG;

    if (pProp->value.GetDataSize() < 8)
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    {
        ULARGE_INTEGER locator;
        HRSRC hRes;
        HGLOBAL hData;
        void* pv;

        memcpy(&locator, pProp->value.GetData(), sizeof(locator));

        hRes = (HRSRC)((BYTE*)hInst + locator.LowPart);
        hData = LoadResource(hInst, hRes);
        pv = hData ? LockResource(hData) : NULL;

        if (pv)
        {
            *ppvStream = pv;
            if (pcbStream)
                *pcbStream = locator.HighPart;
            return S_OK;
        }

        if (pProp->value.GetResourceId())
        {
            hRes = FindResourceW(hInst,
                                 MAKEINTRESOURCEW(LOWORD(pProp->value.GetResourceId())),
                                 L"STREAM");
            if (hRes)
            {
                DWORD cb = SizeofResource(hInst, hRes);
                hData = LoadResource(hInst, hRes);
                pv = hData ? LockResource(hData) : NULL;
                if (pv && cb)
                {
                    UXTRACE(("uxtheme_new: GetThemeStream fell back to "
                             "FindResource for id %u - caller's hInst %p is not "
                             "a LOAD_LIBRARY_AS_DATAFILE mapping of our style\n",
                             pProp->value.GetResourceId(), (void*)hInst));
                    *ppvStream = pv;
                    if (pcbStream)
                        *pcbStream = cb;
                    return S_OK;
                }
            }
        }
    }

    return MakeErrorLast();
}

}
