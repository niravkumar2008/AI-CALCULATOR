@echo off
rem Builds ui_sim_v15.exe: the v15 LCD firmware's drawing code (ui.cpp, fbtext.cpp,
rem vf_lcd.cpp, selftest.cpp, unchanged) + core/ + LovyanGFX's own LGFX_Sprite, on the PC,
rem with the MSVC Build Tools (same detection as tests\msvc\build_and_run.bat).
rem LovyanGFX comes from the firmware's PlatformIO libdeps: run  pio run -e v15lcd  once
rem in firmware-v15-lcd first. Output: tools\ui_sim_v15\build\ui_sim_v15.exe
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "FW=%ROOT%\firmware-v15-lcd\src"
set "LGFX=%ROOT%\firmware-v15-lcd\.pio\libdeps\v15lcd\LovyanGFX\src"
set "OUT=%HERE%build"
if not exist "%LGFX%\LovyanGFX.hpp" ( echo LovyanGFX not found at %LGFX% - run pio run -e v15lcd in firmware-v15-lcd first & exit /b 1 )
set "VS="
for /d %%d in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*") do if exist "%%d\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\BuildTools"
for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do if exist "%%d\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\Community"
if not defined VS ( echo MSVC Build Tools not found - install the "Desktop development with C++" Build Tools & exit /b 1 )
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
if not exist "%OUT%\lgfx" mkdir "%OUT%\lgfx"
if not exist "%OUT%\app" mkdir "%OUT%\app"
set "COMMON=/nologo /std:c++17 /EHsc /O2 /utf-8 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /FI "%HERE%shim\msvc_compat.h" /external:W0 /external:I "%HERE%shim\lgfx_host" /external:I "%LGFX%""
rem 1. LovyanGFX (third-party: warnings off; built once, delete build\lgfx to rebuild)
if not exist "%OUT%\lgfx\LGFXBase.obj" (
  echo Building LovyanGFX for the host...
  cl %COMMON% /W0 /c /Fo:"%OUT%\lgfx\\" "%LGFX%\lgfx\v1\LGFXBase.cpp" "%LGFX%\lgfx\v1\lgfx_v1.cpp" "%LGFX%\lgfx\v1\lgfx_fonts.cpp" "%LGFX%\lgfx\utility\*.c" "%HERE%lgfx_host_panel.cpp" "%HERE%lgfx_host_platform.cpp" "%HERE%lgfx_host_font_stubs.c" >"%OUT%\lgfx_build.log" 2>&1 || ( type "%OUT%\lgfx_build.log" & exit /b 1 )
)
rem 2. Firmware drawing code + core + harness
cl %COMMON% /W3 /I "%HERE%shim\fw" /I "%HERE%." /I "%FW%" /I "%ROOT%\core" /I "%ROOT%\tests\msvc\shim" /FI gccbuiltins.h /DPREVIEW_SWAP_BYTES=0 /DBOARD_V15_LCD=1 /Fo:"%OUT%\app\\" /Fe:"%OUT%\ui_sim_v15.exe" "%HERE%harness.cpp" "%HERE%host_hw.cpp" "%HERE%selftest_tu.cpp" "%FW%\ui.cpp" "%FW%\fbtext.cpp" "%FW%\vf_lcd.cpp" "%ROOT%\core\*.cpp" "%OUT%\lgfx\*.obj" || exit /b 1
echo Built %OUT%\ui_sim_v15.exe
