@echo off
chcp 65001 >nul
set VSLANG=1033
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
"D:\编译器\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "E:\ChatGPT-MHWI\runtime\ninja-horizon-complete-c2-notice-20261021" --target StormboltPowerCellSessionSelftest StormboltEquipmentStartupSelftest HorizonCompleteShieldCallbacksSelftest
if errorlevel 1 exit /b 1
for %%T in (StormboltPowerCellSessionSelftest QuestMappingPatchSelftest StormboltHssReticleSelftest HorizonCompleteShieldCallbacksSelftest) do (
 "E:\ChatGPT-MHWI\runtime\ninja-horizon-complete-c2-notice-20261021\%%T.exe"
 if errorlevel 1 exit /b 1
)
exit /b 0
