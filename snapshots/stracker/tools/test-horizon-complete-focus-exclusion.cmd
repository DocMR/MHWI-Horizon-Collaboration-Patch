@echo off
chcp 65001 >nul
set VSLANG=1033
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT /Fe:research\horizon-complete-focus-exclusion.exe /Fo:research\horizon-complete-focus-exclusion.obj tools\test_horizon_complete_focus_exclusion.cpp
if errorlevel 1 exit /b 1
research\horizon-complete-focus-exclusion.exe
exit /b %errorlevel%
