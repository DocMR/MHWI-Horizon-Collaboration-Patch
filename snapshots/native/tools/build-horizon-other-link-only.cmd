@echo off
chcp 65001 >nul
call "%~dp0horizon-other-msvc.cmd"
if not "%errorlevel%"=="0" exit /b %errorlevel%
pushd "%~dp0..\staging\Horizon-other-module"
if not exist build\link-only mkdir build\link-only
cl /nologo /TC /W3 /WX /c ..\..\runtime\vendor\minhook-1.3.4\src\hook.c /Fobuild\link-only\minhook_hook.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /TC /W3 /WX /c ..\..\runtime\vendor\minhook-1.3.4\src\buffer.c /Fobuild\link-only\minhook_buffer.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /TC /W3 /WX /c ..\..\runtime\vendor\minhook-1.3.4\src\trampoline.c /Fobuild\link-only\minhook_trampoline.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /TC /W3 /WX /c ..\..\runtime\vendor\minhook-1.3.4\src\hde\hde64.c /Fobuild\link-only\minhook_hde64.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c ..\..\runtime\src\stormbolt_status_update_entry.cpp /Fobuild\link-only\stormbolt_status_update_entry_globals.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_status_update_entry_asm.obj ..\..\runtime\src\stormbolt_status_update_entry.asm
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c ..\..\runtime\src\stormbolt_timer_update_entry.cpp /Fobuild\link-only\stormbolt_timer_update_entry_globals.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_timer_update_entry_asm.obj ..\..\runtime\src\stormbolt_timer_update_entry.asm
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_growth_entry_asm.obj ..\..\runtime\src\stormbolt_growth_entry.asm
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c ..\..\runtime\src\stormbolt_loop_update_entry.cpp /Fobuild\link-only\stormbolt_loop_update_entry_globals.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_loop_update_entry_asm.obj ..\..\runtime\src\stormbolt_loop_update_entry.asm
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c ..\..\runtime\src\stormbolt_sync_entries.cpp /Fobuild\link-only\stormbolt_sync_entries_globals.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_sync_pack_entry_asm.obj ..\..\runtime\src\stormbolt_sync_pack_entry.asm
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_sync_complete_entry_asm.obj ..\..\runtime\src\stormbolt_sync_complete_entry.asm
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c ..\..\runtime\src\stormbolt_remote_update_entry.cpp /Fobuild\link-only\stormbolt_remote_update_entry_globals.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\stormbolt_remote_update_entry_asm.obj ..\..\runtime\src\stormbolt_remote_update_entry.asm
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\horizon_shield_multiplier_entry_asm.obj src\horizon_shield_multiplier_entry.asm
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\link-only\horizon_shield_reaction_entry_asm.obj src\horizon_shield_reaction_entry.asm
if not "%errorlevel%"=="0" goto failure
link /nologo /DLL /NOENTRY /MACHINE:X64 /OPT:NOREF /OPT:NOICF /OUT:build\link-only\horizon_other_linkcheck.dll /WHOLEARCHIVE:build\horizon_other_catalog.lib build\link-only\minhook_hook.obj build\link-only\minhook_buffer.obj build\link-only\minhook_trampoline.obj build\link-only\minhook_hde64.obj build\link-only\stormbolt_status_update_entry_globals.obj build\link-only\stormbolt_status_update_entry_asm.obj build\link-only\stormbolt_timer_update_entry_globals.obj build\link-only\stormbolt_timer_update_entry_asm.obj build\link-only\stormbolt_growth_entry_asm.obj build\link-only\stormbolt_loop_update_entry_globals.obj build\link-only\stormbolt_loop_update_entry_asm.obj build\link-only\stormbolt_sync_entries_globals.obj build\link-only\stormbolt_sync_pack_entry_asm.obj build\link-only\stormbolt_sync_complete_entry_asm.obj build\link-only\stormbolt_remote_update_entry_globals.obj build\link-only\stormbolt_remote_update_entry_asm.obj build\link-only\horizon_shield_multiplier_entry_asm.obj build\link-only\horizon_shield_reaction_entry_asm.obj kernel32.lib bcrypt.lib libvcruntime.lib libucrt.lib
if not "%errorlevel%"=="0" goto failure
popd
exit /b 0
:failure
popd
exit /b 1
