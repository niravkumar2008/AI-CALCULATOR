@echo off
rem The full-size fuzz campaign (tools\fuzz\README.md). Builds tools\fuzz\build\fuzz_long_<mode>.exe
rem from tests\fuzz_tests.cpp + core\ with the MSVC Build Tools and runs it.
rem   run_long.bat                 debug build: /RTC1 /MDd /D_ITERATOR_DEBUG_LEVEL=2 (catches more, ~10x slower)
rem   run_long.bat release         /O2: the same campaign in a few minutes
rem   run_long.bat debug --seeds 2 --sessions 50     any further options go to fuzz_long.exe
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "MODE=debug"
if /i "%~1"=="release" ( set "MODE=release" & shift )
if /i "%~1"=="debug" shift
set "ARGS="
:args
if "%~1"=="" goto built_args
set "ARGS=%ARGS% %1"
shift
goto args
:built_args
set "VS="
for /d %%d in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*") do if exist "%%d\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\BuildTools"
for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do if exist "%%d\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\Community"
if not defined VS ( echo MSVC Build Tools not found - install the "Desktop development with C++" Build Tools & exit /b 1 )
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
set "OUT=%HERE%build\%MODE%"
if not exist "%OUT%" mkdir "%OUT%"
set "FLAGS=/O2"
if "%MODE%"=="debug" set "FLAGS=/Od /Zi /RTC1 /MDd /D_ITERATOR_DEBUG_LEVEL=2"
cl /nologo /std:c++17 /EHsc /W3 /utf-8 %FLAGS% /I "%ROOT%\tests\msvc\shim" /FI gccbuiltins.h /D_CRT_SECURE_NO_WARNINGS /Fe:"%OUT%\fuzz_long_%MODE%.exe" /Fo:"%OUT%\\" /Fd:"%OUT%\\" "%HERE%fuzz_long.cpp" "%ROOT%\tests\fuzz_tests.cpp" "%ROOT%\core\*.cpp" >"%OUT%\build.log" 2>&1 || ( type "%OUT%\build.log" & exit /b 1 )
cd /d "%ROOT%"
"%OUT%\fuzz_long_%MODE%.exe"%ARGS%
