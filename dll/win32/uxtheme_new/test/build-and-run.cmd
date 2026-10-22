@echo off
rem Usage: test\build-and-run.cmd [<build-dir>]
setlocal

set SELFDIR=%~dp0
set SRCDIR=%SELFDIR%..
if "%~1"=="" (
    set BUILDDIR=%SELFDIR%..\..\..\..\build-msvc-amd64
) else (
    set BUILDDIR=%~1
)

if not defined VCINSTALLDIR (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
)

set OUT=%TEMP%\uxtheme_new_test
if not exist "%OUT%\obj" mkdir "%OUT%\obj"

echo === building AnchorTest ===
cl /nologo /EHs-c- /GR- /W3 /D_CRT_SECURE_NO_WARNINGS /Zi ^
   /Fo"%OUT%\obj\\" /Fd"%OUT%\obj\anchor.pdb" /Fe"%OUT%\anchortest.exe" ^
   "%SELFDIR%AnchorTest.cpp" ^
   "%SRCDIR%\src\UxThemeP.cpp" "%SRCDIR%\src\model\ThemeFile.cpp" ^
   "%SRCDIR%\src\load\PeModule.cpp" "%SRCDIR%\src\load\Primitives.cpp" ^
   "%SRCDIR%\src\load\VSUnpack.cpp" "%SRCDIR%\src\load\ThemeLoader.cpp" ^
   /link user32.lib gdi32.lib kernel32.lib || exit /b 1

echo === building RenderTest ===
cl /nologo /EHsc /GR- /W3 /D_CRT_SECURE_NO_WARNINGS /Zi ^
   /Fo"%OUT%\obj\\" /Fd"%OUT%\obj\render.pdb" /Fe"%OUT%\rendertest.exe" ^
   "%SELFDIR%RenderTest.cpp" /link user32.lib gdi32.lib || exit /b 1

echo === building AbiTest ===
cl /nologo /EHs-c- /GR- /W3 /D_CRT_SECURE_NO_WARNINGS /Zi ^
   /Fo"%OUT%\obj\\" /Fd"%OUT%\obj\abi.pdb" /Fe"%OUT%\abitest.exe" ^
   "%SELFDIR%AbiTest.cpp" /link user32.lib || exit /b 1

echo.
echo === AnchorTest (against the tree-built Aero) ===
"%OUT%\anchortest.exe" "%BUILDDIR%\media\themes\Aero\aero.msstyles" "%BUILDDIR%"
set RC1=%ERRORLEVEL%

echo.
echo === AbiTest ===
"%OUT%\abitest.exe" "%BUILDDIR%\DarkFiresReactOSModules\uxtheme_new\uxtheme.dll" ^
                    "%BUILDDIR%\media\themes\Aero\aero.msstyles"
set RC2=%ERRORLEVEL%

echo.
echo === RenderTest ===
"%OUT%\rendertest.exe" "%BUILDDIR%\DarkFiresReactOSModules\uxtheme_new\uxtheme.dll" ^
                       "%BUILDDIR%\media\themes\Aero\aero.msstyles"
set RC3=%ERRORLEVEL%

echo.
if %RC1%==0 if %RC2%==0 if %RC3%==0 (
    echo ALL TESTS PASSED
    exit /b 0
)
echo TESTS FAILED  ^(anchor=%RC1% abi=%RC2% render=%RC3%^)
exit /b 1
