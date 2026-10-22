/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     DLL entry point
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "UxThemeP.hpp"

void UxThemeApiInit(void);
void UxThemeApiTerm(void);
void UxDecodeTerm(void);
extern "C" void UxNcInitSystem(HINSTANCE hInst);
extern "C" void UxNcUnInitSystem(void);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);

        if (FAILED(ThemeHeapInit()))
            return FALSE;

        UxThemeApiInit();

        UxNcInitSystem(hInstDLL);
        break;

    case DLL_PROCESS_DETACH:
        if (lpvReserved)
            break;

        UxNcUnInitSystem();
        UxThemeApiTerm();
        UxDecodeTerm();
        ThemeHeapTerm();
        break;
    }

    return TRUE;
}
