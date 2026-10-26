@echo off
chcp 65001 >nul
set VSLANG=1033
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT /Iruntime\vendor\minhook-1.3.4\include /Fe:research\horizon-aloy-voice-call.exe /Fo:research\ tools\test_horizon_aloy_voice_call.cpp staging\Horizon-complete-integration-20261021\src\horizon_owner_verification.cpp runtime\ninja-horizon-complete-c3-repairs-20261021\vendor\minhook-1.3.4\minhook.x64.lib bcrypt.lib
if errorlevel 1 exit /b 1
research\horizon-aloy-voice-call.exe
exit /b %errorlevel%
