@echo off
chcp 65001 >nul
set VSLANG=1033
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /MT /Fe:"E:\ChatGPT-MHWI\research\horizon-complete-notice-content.exe" /Fo:"E:\ChatGPT-MHWI\research\horizon-complete-notice-content.obj" "E:\ChatGPT-MHWI\tools\test_horizon_complete_notice_content.cpp"
if errorlevel 1 exit /b 1
"E:\ChatGPT-MHWI\research\horizon-complete-notice-content.exe"
exit /b %errorlevel%
