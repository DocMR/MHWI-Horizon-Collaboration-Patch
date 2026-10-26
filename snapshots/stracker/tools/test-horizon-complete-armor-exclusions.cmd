@echo off
chcp 65001 >nul
call "%~dp0horizon-other-msvc.cmd"
if errorlevel 1 exit /b 1
pushd "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 /WX tools\test_horizon_complete_armor_exclusions.cpp staging\Horizon-complete-integration-20261021\build\horizon_armor_exclusions.obj /Fe:staging\Horizon-complete-integration-20261021\build\armor_exclusions_test.exe /Fo:staging\Horizon-complete-integration-20261021\build\armor_exclusions_test.obj
if errorlevel 1 goto failure
staging\Horizon-complete-integration-20261021\build\armor_exclusions_test.exe
if errorlevel 1 goto failure
popd
exit /b 0
:failure
popd
exit /b 1
