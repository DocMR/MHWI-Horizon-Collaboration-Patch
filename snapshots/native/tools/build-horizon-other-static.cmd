@echo off
chcp 65001 >nul
call "%~dp0horizon-other-msvc.cmd"
if not "%errorlevel%"=="0" exit /b %errorlevel%
pushd "%~dp0..\staging\Horizon-other-module"
if not exist build mkdir build
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_quest_catalog.cpp /Fobuild\horizon_quest_catalog.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_quest_resource_bridge.cpp /Fobuild\horizon_quest_resource_bridge.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_quest_mapping.cpp /Fobuild\horizon_quest_mapping.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_rank_requirements.cpp /Fobuild\horizon_rank_requirements.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_bow_tree_edits.cpp /Fobuild\horizon_bow_tree_edits.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_armor_exclusions.cpp /Fobuild\horizon_armor_exclusions.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_palico_recipe_edits.cpp /Fobuild\horizon_palico_recipe_edits.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_native_integration.cpp /Fobuild\horizon_native_integration.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_native_clone.cpp /Fobuild\horizon_native_clone.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_equipment_hooks.cpp /Fobuild\horizon_equipment_hooks.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_owner_verification.cpp /Fobuild\horizon_owner_verification.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_quest_resource_hooks.cpp /Fobuild\horizon_quest_resource_hooks.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_mapping_transaction.cpp /Fobuild\horizon_mapping_transaction.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_quest_completion_fact.cpp /Fobuild\horizon_quest_completion_fact.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_native_rank_reader.cpp /Fobuild\horizon_native_rank_reader.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_quest_open_maintenance.cpp /Fobuild\horizon_quest_open_maintenance.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_quest_open_hook.cpp /Fobuild\horizon_quest_open_hook.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /c src\horizon_executable_identity.cpp /Fobuild\horizon_executable_identity.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_module_prepare.cpp /Fobuild\horizon_module_prepare.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_module_activation.cpp /Fobuild\horizon_module_activation.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_status.cpp /Fobuild\horizon_shield_status.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_status_calls.cpp /Fobuild\horizon_shield_status_calls.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_status_session.cpp /Fobuild\horizon_shield_status_session.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_growth_calls.cpp /Fobuild\horizon_shield_growth_calls.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_aux_lifecycles.cpp /Fobuild\horizon_shield_aux_lifecycles.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_loop_call.cpp /Fobuild\horizon_shield_loop_call.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_sync_calls.cpp /Fobuild\horizon_shield_sync_calls.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_remote_call.cpp /Fobuild\horizon_shield_remote_call.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_multiplier_call.cpp /Fobuild\horizon_shield_multiplier_call.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_reaction_call.cpp /Fobuild\horizon_shield_reaction_call.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_equipment_session.cpp /Fobuild\horizon_shield_equipment_session.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_shield_native_bindings.cpp /Fobuild\horizon_shield_native_bindings.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_other_activation.cpp /Fobuild\horizon_other_activation.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_other_host.cpp /Fobuild\horizon_other_host.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_npc_sound_hook.cpp /Fobuild\horizon_npc_sound_hook.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_guildcard_admission.cpp /Fobuild\horizon_guildcard_admission.obj
if not "%errorlevel%"=="0" goto failure
cl /nologo /std:c++17 /EHsc /W4 /WX /I..\..\runtime\vendor\minhook-1.3.4\include /c src\horizon_tutorial_trigger.cpp /Fobuild\horizon_tutorial_trigger.obj
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\horizon_tutorial_trigger_entry.obj src\horizon_tutorial_trigger_entry.asm
if not "%errorlevel%"=="0" goto failure
ml64 /nologo /c /Fobuild\horizon_shield_status_span_entries.obj src\horizon_shield_status_span_entries.asm
if not "%errorlevel%"=="0" goto failure
lib /nologo /OUT:build\horizon_other_catalog.lib build\horizon_quest_catalog.obj build\horizon_quest_resource_bridge.obj build\horizon_quest_mapping.obj build\horizon_rank_requirements.obj build\horizon_bow_tree_edits.obj build\horizon_armor_exclusions.obj build\horizon_palico_recipe_edits.obj build\horizon_native_integration.obj build\horizon_native_clone.obj build\horizon_equipment_hooks.obj build\horizon_owner_verification.obj build\horizon_quest_resource_hooks.obj build\horizon_mapping_transaction.obj build\horizon_quest_completion_fact.obj build\horizon_native_rank_reader.obj build\horizon_quest_open_maintenance.obj build\horizon_quest_open_hook.obj build\horizon_executable_identity.obj build\horizon_module_prepare.obj build\horizon_module_activation.obj build\horizon_shield_status.obj build\horizon_shield_status_calls.obj build\horizon_shield_status_session.obj build\horizon_shield_growth_calls.obj build\horizon_shield_aux_lifecycles.obj build\horizon_shield_loop_call.obj build\horizon_shield_sync_calls.obj build\horizon_shield_remote_call.obj build\horizon_shield_multiplier_call.obj build\horizon_shield_reaction_call.obj build\horizon_shield_equipment_session.obj build\horizon_shield_native_bindings.obj build\horizon_other_activation.obj build\horizon_other_host.obj build\horizon_npc_sound_hook.obj build\horizon_guildcard_admission.obj build\horizon_shield_status_span_entries.obj build\horizon_tutorial_trigger.obj build\horizon_tutorial_trigger_entry.obj
if not "%errorlevel%"=="0" goto failure
popd
exit /b 0
:failure
popd
exit /b 1
