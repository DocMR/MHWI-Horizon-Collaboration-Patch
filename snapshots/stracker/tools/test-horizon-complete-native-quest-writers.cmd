@echo off
chcp 65001 >nul
call "%~dp0horizon-other-msvc.cmd"
if errorlevel 1 exit /b 1
pushd "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 /WX tools\test_horizon_complete_native_quest_writers.cpp staging\Horizon-complete-integration-20261021\build\horizon_quest_mapping.obj /Fe:staging\Horizon-complete-integration-20261021\build\native_quest_writers_test.exe /Fo:staging\Horizon-complete-integration-20261021\build\native_quest_writers_test.obj
if errorlevel 1 goto failure
staging\Horizon-complete-integration-20261021\build\native_quest_writers_test.exe
if errorlevel 1 goto failure
popd
exit /b 0
:failure
popd
exit /b 1
