/*
 * PROJECT:     ReactOS uxtheme_new
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Theme loader test
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "../src/UxThemeP.hpp"
#include "../src/model/ThemeFile.hpp"
#include "../src/model/Tmt.hpp"
#include "../src/load/ThemeLoader.hpp"

#include <stdio.h>

static int g_cPass = 0;
static int g_cFail = 0;

static void Check(bool fOk, const char* pszWhat, const char* pszGot)
{
    if (fOk)
    {
        g_cPass++;
        printf("  ok    %-58s %s\n", pszWhat, pszGot);
    }
    else
    {
        g_cFail++;
        printf("  FAIL  %-58s %s\n", pszWhat, pszGot);
    }
}

static CThemeFile* g_pFile = NULL;

static const CThemeProperty* Prop(const WCHAR* pszClass, int iPart, int iState, int sym)
{
    int iClass = g_pFile->FindClass(pszClass);
    if (iClass < 0)
        return NULL;
    return g_pFile->ResolveProperty(iClass, iPart, iState, sym);
}

static void CheckInt(const WCHAR* pszClass, int iPart, int iState, int sym,
                     int nExpect, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[128];

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    sprintf(szGot, "= %d (want %d)", p->value.GetInt(), nExpect);
    Check(p->value.GetInt() == nExpect, pszWhat, szGot);
}

static void CheckColor(const WCHAR* pszClass, int iPart, int iState, int sym,
                       COLORREF crExpect, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[128];

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    sprintf(szGot, "= %02X%02X%02X (want %02X%02X%02X)",
            GetRValue(p->value.GetColor()), GetGValue(p->value.GetColor()),
            GetBValue(p->value.GetColor()),
            GetRValue(crExpect), GetGValue(crExpect), GetBValue(crExpect));
    Check(p->value.GetColor() == crExpect, pszWhat, szGot);
}

static void CheckMargins(const WCHAR* pszClass, int iPart, int iState, int sym,
                         int l, int r, int t, int b, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[160];
    MARGINS m;

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    m = p->value.GetMargins();
    sprintf(szGot, "= %d,%d,%d,%d (want %d,%d,%d,%d)",
            m.cxLeftWidth, m.cxRightWidth, m.cyTopHeight, m.cyBottomHeight,
            l, r, t, b);
    Check(m.cxLeftWidth == l && m.cxRightWidth == r &&
          m.cyTopHeight == t && m.cyBottomHeight == b, pszWhat, szGot);
}

static void CheckRect(const WCHAR* pszClass, int iPart, int iState, int sym,
                      int l, int t, int r, int b, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[160];
    RECT rc;

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    rc = p->value.GetRect();
    sprintf(szGot, "= %ld,%ld,%ld,%ld (want %d,%d,%d,%d)",
            rc.left, rc.top, rc.right, rc.bottom, l, t, r, b);
    Check(rc.left == l && rc.top == t && rc.right == r && rc.bottom == b,
          pszWhat, szGot);
}

static void CheckPosition(const WCHAR* pszClass, int iPart, int iState, int sym,
                          int x, int y, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[128];
    POINT pt;

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    pt = p->value.GetPosition();
    sprintf(szGot, "= %ld,%ld (want %d,%d)", pt.x, pt.y, x, y);
    Check(pt.x == x && pt.y == y, pszWhat, szGot);
}

static void CheckBool(const WCHAR* pszClass, int iPart, int iState, int sym,
                      BOOL fExpect, const char* pszWhat)
{
    const CThemeProperty* p = Prop(pszClass, iPart, iState, sym);
    char szGot[128];

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    sprintf(szGot, "= %d (want %d)", p->value.GetBool() ? 1 : 0, fExpect ? 1 : 0);
    Check((p->value.GetBool() != 0) == (fExpect != 0), pszWhat, szGot);
}

static void CheckString(const CThemeProperty* p, const WCHAR* pszExpect,
                        const char* pszWhat)
{
    char szGot[256];

    if (!p)
    {
        Check(false, pszWhat, "<not found>");
        return;
    }

    sprintf(szGot, "= \"%ls\"", p->value.GetString());
    Check(wcscmp(p->value.GetString(), pszExpect) == 0, pszWhat, szGot);
}

static const CThemeProperty* RootProp(int sym, int nSkip)
{
    for (int i = 0; i < g_pFile->rootProperties.GetCount(); i++)
    {
        if (g_pFile->rootProperties[i].lSymbolVal == sym)
        {
            if (nSkip-- == 0)
                return &g_pFile->rootProperties[i];
        }
    }
    return NULL;
}

int wmain(int argc, wchar_t** argv)
{
    const WCHAR* pszPath =
        (argc > 1) ? argv[1]
                   : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev\\DwmReversingEnvironment"
                     L"\\build-msvc-amd64\\media\\themes\\Aero\\aero.msstyles";
    HRESULT hr;
    char szGot[256];

    printf("uxtheme_new anchor test\n");
    printf("style: %ls\n\n", pszPath);

    hr = CThemeLoader::LoadTheme(pszPath, NULL, NULL, &g_pFile);
    if (FAILED(hr))
    {
        printf("LoadTheme failed: %08lx\n", (unsigned long)hr);
        return 1;
    }

    printf("[file]\n");
    sprintf(szGot, "= %d", g_pFile->nVersion);
    Check(g_pFile->nVersion == 4, "PACKTHEM_VERSION is 4", szGot);

    sprintf(szGot, "= \"%ls\"", g_pFile->strVariantName.Get());
    Check(g_pFile->strVariantName.EqualsI(L"NORMAL"), "VMAP variant name", szGot);

    sprintf(szGot, "= \"%ls\"", g_pFile->strColorName.Get());
    Check(g_pFile->strColorName.EqualsI(L"NormalColor"), "VMAP color", szGot);

    sprintf(szGot, "= \"%ls\"", g_pFile->strSizeName.Get());
    Check(g_pFile->strSizeName.EqualsI(L"NormalSize"), "VMAP size", szGot);

    sprintf(szGot, "= %d", g_pFile->classes.GetCount());
    Check(g_pFile->classes.GetCount() == 247, "classes carrying data (vtheme: 247)", szGot);

    sprintf(szGot, "= %d entries", g_pFile->classNames.GetCount());
    Check(g_pFile->classNames.GetCount() >= 6, "CMAP has the 6-entry prologue", szGot);

    if (g_pFile->classNames.GetCount() >= 6)
    {
        sprintf(szGot, "= \"%ls\"", g_pFile->classNames[0].Get());
        Check(g_pFile->classNames[0].EqualsI(L"documentation"), "CMAP[0] documentation", szGot);
        sprintf(szGot, "= \"%ls\"", g_pFile->classNames[4].Get());
        Check(g_pFile->classNames[4].EqualsI(L"globals"), "CMAP[4] globals", szGot);
        sprintf(szGot, "= \"%ls\"", g_pFile->classNames[5].Get());
        Check(g_pFile->classNames[5].EqualsI(L"sysmetrics"), "CMAP[5] sysmetrics", szGot);
    }

    printf("\n[RMAP - STRING primitives]\n");
    CheckString(RootProp(TMT_NAME, 0), L"Aero", "RMAP NAME");
    CheckString(RootProp(TMT_DISPLAYNAME, 0), L"Aero style", "RMAP DISPLAYNAME");
    CheckString(RootProp(TMT_AUTHOR, 0), L"MSX", "RMAP AUTHOR");
    CheckString(RootProp(TMT_VERSION, 0), L"1.0", "RMAP VERSION");
    CheckString(RootProp(TMT_URL, 0), L"http://www.microsoft.com", "RMAP URL");

    printf("\n[globals - INT, COLOR]\n");
    CheckInt(L"globals", 0, 0, TMT_TEXTGLOWSIZE, 0, "globals TEXTGLOWSIZE");
    CheckInt(L"globals", 0, 0, TMT_GLOWINTENSITY, 250, "globals GLOWINTENSITY");
    CheckInt(L"globals", 0, 0, TMT_COLORIZATIONCOLOR, -2105377, "globals COLORIZATIONCOLOR");
    CheckColor(L"globals", 0, 0, TMT_EDGEHIGHLIGHTCOLOR, RGB(0xF4, 0xF7, 0xFC),
               "globals EDGEHIGHLIGHTCOLOR");
    CheckColor(L"globals", 0, 0, TMT_EDGEDKSHADOWCOLOR, RGB(0x64, 0x64, 0x64),
               "globals EDGEDKSHADOWCOLOR");

    printf("\n[DWMWindow part 57 - the text-glow bitmap]\n");
    CheckInt(L"DWMWindow", 57, 0, TMT_IMAGECOUNT, 2, "part 57 IMAGECOUNT");
    CheckInt(L"DWMWindow", 57, 0, TMT_TEXTGLOWSIZE, 10, "part 57 TEXTGLOWSIZE  <- phase 1 gate");
    CheckInt(L"DWMWindow", 57, 0, TMT_GLOWINTENSITY, 336, "part 57 GLOWINTENSITY");
    CheckInt(L"DWMWindow", 57, 0, TMT_BGTYPE, BT_IMAGEFILE, "part 57 BGTYPE");
    CheckMargins(L"DWMWindow", 57, 0, TMT_SIZINGMARGINS, 0, 0, 6, 2, "part 57 SIZINGMARGINS");
    CheckMargins(L"DWMWindow", 57, 0, TMT_CONTENTMARGINS, 9, 0, 0, 0, "part 57 CONTENTMARGINS");
    CheckRect(L"DWMWindow", 57, 0, TMT_ATLASRECT, 15, 692, 36, 746, "part 57 ATLASRECT");

    CheckInt(L"DWMWindow", 57, 1, TMT_OPACITY, 80, "part 57 state 1 OPACITY");
    CheckInt(L"DWMWindow", 57, 1, TMT_COLORIZATIONOPACITY, 100, "part 57 state 1 COLORIZATIONOPACITY");
    CheckColor(L"DWMWindow", 57, 1, TMT_GLOWCOLOR, RGB(0xFF, 0xFF, 0xFF), "part 57 state 1 GLOWCOLOR");
    CheckInt(L"DWMWindow", 57, 2, TMT_OPACITY, 45, "part 57 state 2 OPACITY");
    CheckColor(L"DWMWindow", 57, 3, TMT_TEXTCOLOR, RGB(0xFF, 0xFF, 0xFF), "part 57 state 3 TEXTCOLOR");
    CheckColor(L"DWMWindow", 57, 3, TMT_GLOWCOLOR, RGB(0x00, 0x00, 0x00), "part 57 state 3 GLOWCOLOR");
    CheckInt(L"DWMWindow", 57, 3, TMT_COLORIZATIONCOLOR, -16777216, "part 57 state 3 COLORIZATIONCOLOR");

    printf("\n[DWMWindow frame parts - the ids uDWM asks for]\n");
    CheckRect(L"DWMWindow", 40, 0, TMT_ATLASRECT, 397, 0, 1199, 604, "part 40 ATLASRECT");
    CheckInt(L"DWMWindow", 40, 1, TMT_OPACITY, 90, "part 40 state 1 OPACITY");
    CheckInt(L"DWMWindow", 40, 2, TMT_OPACITY, 40, "part 40 state 2 OPACITY");
    CheckInt(L"DWMWindow", 40, 3, TMT_OPACITY, 50, "part 40 state 3 OPACITY");
    CheckInt(L"DWMWindow", 40, 4, TMT_OPACITY, 20, "part 40 state 4 OPACITY");
    CheckRect(L"DWMWindow", 34, 0, TMT_ATLASRECT, 169, 0, 397, 350, "part 34 ATLASRECT");
    CheckInt(L"DWMWindow", 34, 1, TMT_OPACITY, 75, "part 34 state 1 OPACITY");
    CheckRect(L"DWMWindow", 42, 0, TMT_ATLASRECT, 169, 350, 397, 700, "part 42 ATLASRECT");
    CheckMargins(L"DWMWindow", 44, 0, TMT_SIZINGMARGINS, 0, 0, 0, 2, "part 44 SIZINGMARGINS");
    CheckInt(L"DWMWindow", 44, 2, TMT_OPACITY, 50, "part 44 state 2 OPACITY");

    CheckInt(L"DWMWindow", 34, 1, TMT_IMAGECOUNT, 2, "part 34 state 1 IMAGECOUNT (falls back to part)");

    printf("\n[Button part 1 - BOOL, POSITION, INTLIST, ENUM]\n");
    CheckBool(L"Button", 1, 0, TMT_TRANSPARENT, TRUE, "Button TRANSPARENT");
    CheckBool(L"Button", 1, 0, TMT_TEXTGLOW, TRUE, "Button TEXTGLOW");
    CheckInt(L"Button", 1, 0, TMT_IMAGECOUNT, 6, "Button IMAGECOUNT");
    CheckPosition(L"Button", 1, 0, TMT_MINSIZE, 10, 5, "Button MINSIZE");
    CheckMargins(L"Button", 1, 0, TMT_SIZINGMARGINS, 4, 4, 4, 4, "Button SIZINGMARGINS");
    CheckColor(L"Button", 1, 0, TMT_TEXTBORDERCOLOR, RGB(0xFF, 0xFF, 0xFF), "Button TEXTBORDERCOLOR");
    CheckInt(L"Button", 1, 0, TMT_SIZINGTYPE, ST_STRETCH, "Button SIZINGTYPE");
    CheckInt(L"Button", 1, 0, TMT_IMAGELAYOUT, IL_VERTICAL, "Button IMAGELAYOUT");
    CheckColor(L"Button", 1, 4, TMT_TEXTCOLOR, RGB(0x83, 0x83, 0x83), "Button state 4 TEXTCOLOR");

    {
        const CThemeProperty* p = Prop(L"Button", 1, 0, TMT_TRANSITIONDURATIONS);
        if (!p)
        {
            Check(false, "Button TRANSITIONDURATIONS", "<not found>");
        }
        else
        {
            const int* pn = p->value.GetIntList();
            int c = p->value.GetIntListCount();
            bool fOk = (c == 38 && pn[0] == 37 && pn[1] == 6 && pn[2] == 0 &&
                        pn[3] == 225 && pn[37] == 0);
            sprintf(szGot, "= %d ints, [0..3] = %d,%d,%d,%d (want 38, 37,6,0,225)",
                    c, c > 0 ? pn[0] : -1, c > 1 ? pn[1] : -1,
                    c > 2 ? pn[2] : -1, c > 3 ? pn[3] : -1);
            Check(fOk, "Button TRANSITIONDURATIONS (INTLIST)", szGot);
        }
    }

    printf("\n[FILENAME - image records]\n");
    {
        const CThemeProperty* p = Prop(L"Button", 1, 0, TMT_IMAGEFILE);
        if (!p)
        {
            Check(false, "Button IMAGEFILE present", "<not found>");
        }
        else
        {
            sprintf(szGot, "kind=%d cbData=%d", (int)p->value.Kind(),
                    p->value.GetDataSize());
            Check(p->value.Kind() == TPID_BITMAPIMAGE &&
                  p->value.GetDataSize() == 16,
                  "Button IMAGEFILE is a 16-byte BITMAPIMAGE descriptor", szGot);
        }
    }
    {
        const CThemeProperty* p = Prop(L"AB::AddressBand", 1, 0, TMT_IMAGEFILE);
        if (!p)
        {
            Check(false, "AB::AddressBand IMAGEFILE present", "<not found>");
        }
        else
        {
            const void* pv = ((const void* const*)p->value.GetData())[0];
            sprintf(szGot, "bits=%p", pv);
            Check(pv != NULL, "AB::AddressBand IMAGEFILE resolved to bits", szGot);
        }
    }

    printf("\n[FONT - the .mui string path]\n");
    {
        const CThemeProperty* p = Prop(L"AeroWizardStyle", 1, 0, TMT_FONT);
        if (!p)
        {
            Check(false, "AeroWizardStyle FONT present", "<not found>");
        }
        else
        {
            const LOGFONTW* plf = p->value.GetFont();
            if (!plf)
            {
                sprintf(szGot, "cbData=%d (want %d)", p->value.GetDataSize(),
                        (int)sizeof(LOGFONTW));
                Check(false, "AeroWizardStyle FONT is a LOGFONTW", szGot);
            }
            else
            {
                sprintf(szGot, "face=\"%ls\" height=%ld weight=%ld quality=%d",
                        plf->lfFaceName, plf->lfHeight, plf->lfWeight,
                        (int)plf->lfQuality);
                Check(wcscmp(plf->lfFaceName, L"Segoe UI") == 0 &&
                      plf->lfHeight == -12 && plf->lfWeight == 400,
                      "AeroWizardStyle FONT = Segoe UI 9pt", szGot);
            }
        }
    }

    printf("\n[BCMAP - base-class inheritance]\n");
    {
        int iCls = g_pFile->FindClass(L"CompositedWindow::Window");
        if (iCls < 0)
        {
            Check(false, "CompositedWindow::Window exists", "<not found>");
        }
        else
        {
            int iBase = g_pFile->classes[iCls].iBaseClass;
            const WCHAR* pszBase = (iBase >= 0)
                ? g_pFile->classes[iBase].strFullName.Get() : L"<none>";
            sprintf(szGot, "base = \"%ls\" (want \"Window\")", pszBase);
            Check(iBase >= 0 && g_pFile->classes[iBase].strFullName.EqualsI(L"Window"),
                  "CompositedWindow::Window : Window", szGot);
        }
    }
    {
        int iCls = g_pFile->FindClass(L"Button");
        if (iCls < 0)
        {
            Check(false, "Button exists", "<not found>");
        }
        else
        {
            int iBase = g_pFile->classes[iCls].iBaseClass;
            const WCHAR* pszBase = (iBase >= 0)
                ? g_pFile->classes[iBase].strFullName.Get() : L"<none>";
            sprintf(szGot, "base = \"%ls\" (want \"ButtonStyle\")", pszBase);
            Check(iBase >= 0 && g_pFile->classes[iBase].strFullName.EqualsI(L"ButtonStyle"),
                  "Button : ButtonStyle", szGot);
        }
    }

    printf("\n[v3 rejection]\n");
    {
        static const WCHAR* const rgszV3[] = {
            L"Lautus\\lautus", L"Lunar\\lunar", L"Mizu\\mizu", L"Modern\\modern",
        };
        const WCHAR* pszBuildDir =
            (argc > 2) ? argv[2]
                       : L"C:\\Users\\darkfire\\Desktop\\ReactOSDev"
                         L"\\DwmReversingEnvironment\\build-msvc-amd64";
        WCHAR szPath[MAX_PATH];
        char szWhat[160];

        for (int i = 0; i < ARRAYSIZE(rgszV3); i++)
        {
            CThemeFile* pV3 = NULL;
            HRESULT hrV3;

            StringCchPrintfW(szPath, ARRAYSIZE(szPath),
                             L"%s\\media\\themes\\%s.msstyles\\%s.msstyles",
                             pszBuildDir, rgszV3[i],
                             wcsrchr(rgszV3[i], L'\\') + 1);

            sprintf(szWhat, "%ls rejected as ERROR_BAD_FORMAT", rgszV3[i]);

            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                printf("  SKIP  %-58s not built\n", szWhat);
                continue;
            }

            hrV3 = CThemeLoader::LoadTheme(szPath, NULL, NULL, &pV3);

            sprintf(szGot, "hr=%08lx file=%p", (unsigned long)hrV3, (void*)pV3);
            Check(hrV3 == HRESULT_FROM_WIN32(ERROR_BAD_FORMAT) && pV3 == NULL,
                  szWhat, szGot);

            if (pV3)
            {
                pV3->Release();
            }
        }
    }

    printf("\n%d passed, %d failed\n", g_cPass, g_cFail);

    g_pFile->Release();

    return g_cFail ? 1 : 0;
}
