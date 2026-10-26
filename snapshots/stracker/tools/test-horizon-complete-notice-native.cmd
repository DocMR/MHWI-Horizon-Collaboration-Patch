@echo off
chcp 65001 >nul
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT /DSTORMSLINGER_HORIZON_COMPLETE=1 /Iruntime\vendor\minhook-1.3.4\include /Fe:research\horizon-complete-notice-native.exe /Fo:research\ tools\test_horizon_complete_notice_native.cpp staging\Horizon-complete-integration-20261021\src\horizon_owner_verification.cpp runtime\src\runtime_log.cpp runtime\ninja-horizon-complete-c2-notice-20261021\vendor\minhook-1.3.4\minhook.x64.lib bcrypt.lib
if errorlevel 1 exit /b 1
research\horizon-complete-notice-native.exe %*
exit /b %errorlevel%