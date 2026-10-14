@echo off
rem Builds Proofline_Simulator_v15.exe (put at the top of the repo): the v15 calculator on
rem the PC, driven by the REAL firmware code: core\ (the calculator, apps, exam mode) and the
rem v15 LCD drawing code (firmware-v15-lcd\src\ui*.cpp, fbtext.cpp, vf_lcd.cpp) drawing
rem into LovyanGFX's own sprite, shown in a plain Win32 window (tools\sim_v15).
rem
rem Needs: the MSVC Build Tools ("Desktop development with C++"), and LovyanGFX from the
rem firmware's PlatformIO libdeps (run  pio run -e v15lcd  once in firmware-v15-lcd).
rem Double-click it, or run it from a terminal (add "nopause" to skip the pause at the end).
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "FW=%ROOT%\firmware-v15-lcd\src"
set "HARNESS=%ROOT%\tools\ui_sim_v15"
set "LGFX=%ROOT%\firmware-v15-lcd\.pio\libdeps\v15lcd\LovyanGFX\src"
set "OUT=%HERE%build"
set "EXE=%ROOT%\Proofline_Simulator_v15.exe"
set "RC=0"
if not exist "%LGFX%\LovyanGFX.hpp" ( echo LovyanGFX not found at %LGFX% - run  pio run -e v15lcd  in firmware-v15-lcd first & set "RC=1" & goto done )
set "VS="
for /d %%d in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*") do if exist "%%d\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\BuildTools"
for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do if exist "%%d\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\Community"
if not defined VS ( echo MSVC Build Tools not found - install the "Desktop development with C++" Build Tools & set "RC=1" & goto done )
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
if not exist "%OUT%\lgfx" mkdir "%OUT%\lgfx"
if not exist "%OUT%\app" mkdir "%OUT%\app"
set "COMMON=/nologo /std:c++17 /EHsc /O2 /utf-8 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS /FI "%HARNESS%\shim\msvc_compat.h" /external:W0 /external:I "%HARNESS%\shim\lgfx_host" /external:I "%LGFX%""
rem 1. LovyanGFX for the host (third-party: warnings off; built once, delete build\lgfx to rebuild).
rem    The host panel / platform stand-ins are the screenshot harness's (tools\ui_sim_v15).
if not exist "%OUT%\lgfx\LGFXBase.obj" (
  echo Building LovyanGFX for the PC ^(once, about a minute^)...
  cl %COMMON% /W0 /c /Fo:"%OUT%\lgfx\\" "%LGFX%\lgfx\v1\LGFXBase.cpp" "%LGFX%\lgfx\v1\lgfx_v1.cpp" "%LGFX%\lgfx\v1\lgfx_fonts.cpp" "%LGFX%\lgfx\utility\*.c" "%HARNESS%\lgfx_host_panel.cpp" "%HARNESS%\lgfx_host_platform.cpp" "%HARNESS%\lgfx_host_font_stubs.c" >"%OUT%\lgfx_build.log" 2>&1 || ( type "%OUT%\lgfx_build.log" & set "RC=1" & goto done )
)
rem 2. The simulator + the firmware's drawing code + core.
echo Building the simulator...
cl %COMMON% /W3 /I "%HARNESS%\shim\fw" /I "%HERE%." /I "%FW%" /I "%ROOT%\core" /I "%ROOT%\tests\msvc\shim" /FI gccbuiltins.h /DPREVIEW_SWAP_BYTES=0 /DBOARD_V15_LCD=1 /Fo:"%OUT%\app\\" /Fe:"%EXE%" "%HERE%win_main.cpp" "%HERE%board.cpp" "%HERE%host_hw.cpp" "%HERE%wic_image.cpp" "%FW%\ui*.cpp" "%FW%\fbtext.cpp" "%FW%\vf_lcd.cpp" "%ROOT%\core\*.cpp" "%OUT%\lgfx\*.obj" /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comdlg32.lib comctl32.lib ole32.lib windowscodecs.lib shell32.lib || ( echo. & echo BUILD FAILED. If Proofline_Simulator_v15.exe is still open, close it and try again. & set "RC=1" & goto done )
echo.
echo Built %EXE%
echo Double-click Proofline_Simulator_v15.exe to start it.
:done
if /i not "%~1"=="nopause" pause
exit /b %RC%
