@echo off
rem Build and run the core tests on Windows with the MSVC Build Tools (no GCC needed).
rem The shim headers stand in for GCC's __builtin_*_overflow and <dirent.h>.
rem Output: tests\msvc\tests.exe, then it runs from the repo root so tests\golden is found.
setlocal
set "HERE=%~dp0"
set "ROOT=%HERE%..\.."
set "VS="
for /d %%d in ("%ProgramFiles(x86)%\Microsoft Visual Studio\*") do if exist "%%d\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\BuildTools"
for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do if exist "%%d\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS=%%d\Community"
if not defined VS ( echo MSVC Build Tools not found - install the "Desktop development with C++" Build Tools & exit /b 1 )
call "%VS%\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /std:c++17 /EHsc /O2 /W3 /utf-8 /I "%HERE%shim" /FI gccbuiltins.h /D_CRT_SECURE_NO_WARNINGS /Fe:"%HERE%tests.exe" /Fo:"%HERE%\" "%ROOT%\tests\tests.cpp" "%ROOT%\core\*.cpp" || exit /b 1
cd /d "%ROOT%"
"%HERE%tests.exe"
