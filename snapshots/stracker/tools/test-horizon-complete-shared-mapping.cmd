@echo off
chcp 65001 >nul
call "%~dp0horizon-other-msvc.cmd"
if errorlevel 1 exit /b 1
pushd "%~dp0.."
cl /nologo /std:c++17 /EHsc /W4 /WX tools\test_horizon_complete_shared_mapping.cpp staging\Horizon-complete-integration-20261021\build\horizon_quest_mapping.obj staging\Horizon-complete-integration-20261021\build\horizon_mapping_transaction.obj staging\Horizon-complete-integration-20261021\build\horizon_owner_verification.obj staging\Horizon-complete-integration-20261021\build\horizon_native_integration.obj staging\Horizon-complete-integration-20261021\build\horizon_bow_tree_edits.obj staging\Horizon-complete-integration-20261021\build\horizon_palico_recipe_edits.obj staging\Horizon-complete-integration-20261021\build\horizon_armor_exclusions.obj staging\Horizon-complete-integration-20261021\build\horizon_quest_completion_fact.obj staging\Horizon-complete-integration-20261021\build\horizon_rank_requirements.obj staging\Horizon-complete-integration-20261021\build\horizon_quest_catalog.obj /Fe:staging\Horizon-complete-integration-20261021\build\shared_mapping_test.exe /Fo:staging\Horizon-complete-integration-20261021\build\shared_mapping_test.obj
if errorlevel 1 goto failure
staging\Horizon-complete-integration-20261021\build\shared_mapping_test.exe
if errorlevel 1 goto failure
popd
exit /b 0
:failure
popd
exit /b 1
