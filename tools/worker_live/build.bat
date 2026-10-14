@echo off
rem Builds worker_live.exe (a pretend calculator for the Worker API) with the MSVC Build Tools.
rem The .exe goes to %TEMP%\worker_live_obj (kept out of the repo). Then: worker_live register http://127.0.0.1:8787, then status / poll / solve.
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "VS="
for /d %%d in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*") do if exist "%%d\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\BuildTools"
for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do if exist "%%d\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\Community"
if not defined VS ( echo MSVC Build Tools not found & exit /b 1 )
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not exist "%TEMP%\worker_live_obj" mkdir "%TEMP%\worker_live_obj"
cl /nologo /std:c++17 /EHsc /O1 /W3 /utf-8 /I "%ROOT%\tests\msvc\shim" /FI gccbuiltins.h /D_CRT_SECURE_NO_WARNINGS /Fe:"%TEMP%\worker_live_obj\worker_live.exe" /Fo:"%TEMP%\worker_live_obj\\" "%HERE%worker_live.cpp" "%ROOT%\core\worker_api.cpp" "%ROOT%\core\claude_api.cpp" "%ROOT%\core\json.cpp" "%ROOT%\core\png.cpp" "%ROOT%\core\text.cpp" "%ROOT%\core\font.cpp" "%ROOT%\core\font_data.cpp" || exit /b 1
echo Built %TEMP%\worker_live_obj\worker_live.exe
