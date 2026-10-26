@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (echo Run from an x64 Native Tools Command Prompt for Visual Studio. & exit /b 1)
cmake -S "%~dp0snapshots/stracker/runtime" -B "%~dp0build/stracker" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSTORMSLINGER_ORIGINAL_QUEST_IDS=ON -DSTORMSLINGER_NATIVE_QUEST_SAVE=ON -DSTORMSLINGER_WEAPON_TREE_FILTER=ON -DSTORMSLINGER_HORIZON_COMPLETE=ON -DSTORMSLINGER_HORIZON_STRACKER=ON
if errorlevel 1 exit /b 1
cmake --build "%~dp0build/stracker" --target HorizonCollaboration
exit /b %errorlevel%
