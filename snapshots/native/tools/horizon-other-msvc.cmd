@echo off
rem Use an x64 developer prompt, an explicit toolchain, or Visual Studio discovery.
if /I "%VSCMD_ARG_TGT_ARCH%"=="x64" exit /b 0
if defined HORIZON_VCVARS64 goto explicit
set "horizon_vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%horizon_vswhere%" goto missing
set "horizon_vsroot="
for /f "usebackq tokens=*" %%i in (`"%horizon_vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "horizon_vsroot=%%i"
if not defined horizon_vsroot goto missing
call "%horizon_vsroot%\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b %errorlevel%
:explicit
call "%HORIZON_VCVARS64%" >nul
exit /b %errorlevel%
:missing
echo Open an x64 Visual Studio developer prompt or set HORIZON_VCVARS64 to vcvars64.bat. 1>&2
exit /b 1
