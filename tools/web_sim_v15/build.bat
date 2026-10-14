@echo off
rem Builds the website's v15 calculator demo: tools\web_sim_v15\dist\index.html, one
rem self-contained page (JS + WebAssembly inlined) for the site's /try/ page.
rem The screen is drawn by the REAL firmware code: core\ (the calculator, apps, exam mode)
rem and the v15 LCD drawing code (firmware-v15-lcd\src\ui*.cpp, fbtext.cpp, vf_lcd.cpp)
rem drawing into LovyanGFX's own sprite, compiled to WebAssembly with Emscripten.
rem
rem Needs: the Emscripten SDK in %USERPROFILE%\emsdk (emsdk install latest + activate latest),
rem Python 3 with Pillow (make_page.py), and LovyanGFX from the firmware's PlatformIO libdeps
rem (run  pio run -e v15lcd  once in firmware-v15-lcd).
rem Run it from a terminal or double-click it (add "nopause" to skip the pause at the end).
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "FW=%ROOT%\firmware-v15-lcd\src"
set "HARNESS=%ROOT%\tools\ui_sim_v15"
set "LGFX=%ROOT%\firmware-v15-lcd\.pio\libdeps\v15lcd\LovyanGFX\src"
set "OUT=%HERE%build"
set "EMSDK_DIR=%USERPROFILE%\emsdk"
set "RC=0"
if not exist "%LGFX%\LovyanGFX.hpp" ( echo LovyanGFX not found at %LGFX% - run  pio run -e v15lcd  in firmware-v15-lcd first & set "RC=1" & goto done )
if not exist "%EMSDK_DIR%\emsdk_env.bat" ( echo Emscripten SDK not found at %EMSDK_DIR% & set "RC=1" & goto done )
call "%EMSDK_DIR%\emsdk_env.bat" >nul 2>nul
where em++ >nul 2>nul || ( echo em++ not found after emsdk_env.bat - run  emsdk activate latest  & set "RC=1" & goto done )
if not exist "%OUT%\lgfx" mkdir "%OUT%\lgfx"
if not exist "%OUT%\app" mkdir "%OUT%\app"

set "OPT=-Os -fwasm-exceptions"
set "INC=-isystem "%HARNESS%\shim\lgfx_host" -isystem "%LGFX%""
rem 1. LovyanGFX for the browser (third-party: warnings off; built once, delete build\lgfx to rebuild).
rem    The host panel and font stand-ins are the screenshot harness's (tools\ui_sim_v15).
if not exist "%OUT%\lgfx\LGFXBase.o" (
  echo Building LovyanGFX for WebAssembly ^(once^)...
  for %%f in ("%LGFX%\lgfx\v1\LGFXBase.cpp" "%LGFX%\lgfx\v1\lgfx_v1.cpp" "%LGFX%\lgfx\v1\lgfx_fonts.cpp" "%HARNESS%\lgfx_host_panel.cpp" "%HERE%lgfx_host_platform.cpp") do (
    call em++ -std=c++17 %OPT% -w %INC% -c "%%~f" -o "%OUT%\lgfx\%%~nf.o" || ( set "RC=1" & goto done )
  )
  for %%f in ("%LGFX%\lgfx\utility\*.c" "%HARNESS%\lgfx_host_font_stubs.c") do (
    call emcc %OPT% -w %INC% -c "%%~f" -o "%OUT%\lgfx\%%~nf.o" || ( set "RC=1" & goto done )
  )
)
rem 2. The board + the firmware's drawing code + core, linked into one JS file with the
rem    WebAssembly inlined (SINGLE_FILE). No threads, no file system, no network code.
echo Building the web simulator...
set "SRCS="%HERE%web_main.cpp" "%HERE%board.cpp" "%HERE%host_hw.cpp""
for %%f in ("%FW%\ui*.cpp" "%FW%\fbtext.cpp" "%FW%\vf_lcd.cpp" "%ROOT%\core\*.cpp" "%OUT%\lgfx\*.o") do call set "SRCS=%%SRCS%% "%%~f""
call em++ -std=c++17 %OPT% -Wno-unused-function %INC% -I"%HARNESS%\shim\fw" -I"%HERE%." -I"%FW%" -I"%ROOT%\core" ^
  -DPREVIEW_SWAP_BYTES=0 -DBOARD_V15_LCD=1 %SRCS% -o "%OUT%\sim.js" ^
  -sMODULARIZE=1 -sEXPORT_NAME=createSim -sSINGLE_FILE=1 -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1 ^
  -sFILESYSTEM=0 -sEXPORTED_RUNTIME_METHODS=HEAPU8,UTF8ToString -sEXPORTED_FUNCTIONS=_malloc,_free ^
  -sINCOMING_MODULE_JS_API=[] -sDYNAMIC_EXECUTION=0 --closure 1 || ( echo BUILD FAILED & set "RC=1" & goto done )
rem 3. The page: sim.js + the sample photo inlined into page.html; checks for vendor names.
python "%HERE%make_page.py" || ( set "RC=1" & goto done )
:done
if /i not "%~1"=="nopause" pause
exit /b %RC%
