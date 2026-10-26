@echo off
chcp 65001 >nul
set VSLANG=1033
call "D:\编译器\VS2026\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
"D:\编译器\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S "E:\ChatGPT-MHWI\research\horizon-complete-c4-source-rebuild-20261022\runtime" -B "E:\ChatGPT-MHWI\research\horizon-complete-c4-source-rebuild-20261022\build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TRY_COMPILE_CONFIGURATION=Release -DCMAKE_C_FLAGS_DEBUG="/Z7 /Ob0 /Od /RTC1" -DCMAKE_CXX_FLAGS_DEBUG="/Z7 /Ob0 /Od /RTC1" -DSTORMSLINGER_ORIGINAL_QUEST_IDS=ON -DSTORMSLINGER_NATIVE_QUEST_SAVE=ON -DSTORMSLINGER_WEAPON_TREE_FILTER=ON -DSTORMSLINGER_HORIZON_COMPLETE=ON
if errorlevel 1 exit /b 1
"D:\编译器\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "E:\ChatGPT-MHWI\research\horizon-complete-c4-source-rebuild-20261022\build" --target StormslingerStandalone
if errorlevel 1 exit /b 1
exit /b 0
