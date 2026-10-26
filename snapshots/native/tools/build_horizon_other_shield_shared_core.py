"""Record existing shared core dependencies for single-owner Shield-Weaver integration."""
import hashlib,json,re
from pathlib import Path
P=Path(__file__).resolve().parents[1]
M=P/'staging/Horizon-other-module'
def main():
 seeds=['stormbolt_power_cell_query.h','stormbolt_power_cell_parameters.h','stormbolt_power_cell_multiplier.h',
        'stormbolt_power_cell_timer.h','stormbolt_power_cell_store.h','stormbolt_power_cell_visual_state.h']
 root=P/'runtime/src';pending=list(seeds);seen=set();files=[]
 while pending:
  name=pending.pop()
  if name in seen:continue
  seen.add(name);path=root/name;data=path.read_bytes();source=data.decode('utf-8-sig')
  deps=re.findall(r'^#include "([^"\n]+)"',source,re.M)
  for dep in deps:assert (root/dep).is_file(),dep
  pending.extend(deps)
  assert 'selftest' not in name
  files.append({'path':path.relative_to(P).as_posix(),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),
                'project_includes':deps,'operation':'reuse_single_shared_source_no_copy_or_execution'})
 evidence=[]
 for name in ['horizon-other-shield-effect-seeds.json','pc-power-cell-status-update-callsite.json',
              'pc-power-cell-multiplier-insertion-contract.json','ps-power-cell-loop-timer-contract.json',
              'horizon-other-shield-lifecycle-owners.json','horizon-other-shield-auxiliary-owners.json','horizon-other-shield-multiplier-owner.json','horizon-other-shield-reaction-gate.json','horizon-other-shield-consumer-inventory.json','horizon-other-shield-binding-guards.json']:
  path=P/'research'/name;data=path.read_bytes();json.loads(data)
  evidence.append({'path':path.relative_to(P).as_posix(),'sha256':hashlib.sha256(data).hexdigest()})
 adapters=[]
 for name in ['stormbolt_power_cell_event_eligibility_adapter.h','stormbolt_power_cell_timer_adapter.h',
              'stormbolt_loop_update_inputs.h','stormbolt_loop_skill_gate.h','stormbolt_power_cell_prompt.h',
              'stormbolt_status_update_dispatch.h','stormbolt_timer_update_dispatch.h',
              'stormbolt_loop_call_patch.h','stormbolt_loop_relay.h',
              'stormbolt_status_update_entry.cpp','stormbolt_status_update_entry.asm',
              'stormbolt_timer_update_entry.cpp','stormbolt_timer_update_entry.asm',
              'stormbolt_status_lifecycle_hooks.h','stormbolt_power_cell_lifecycle.h',
              'stormbolt_growth_session.h','stormbolt_growth_dispatch.h','stormbolt_growth_entry.asm',
              'stormbolt_power_cell_growth_complete_adapter.h','stormbolt_power_cell_growth_inputs.h',
              'stormbolt_power_cell_growth_gate.h','stormbolt_power_cell_growth_commit.h',
              'stormbolt_power_cell_event_growth.h','stormbolt_damage_exception_hooks.h','stormbolt_damage_exception_store.h','stormbolt_loop_audio_dispatch.h','stormbolt_loop_component_update.h',
              'stormbolt_power_cell_update.h','stormbolt_loop_update_entry.cpp','stormbolt_loop_update_entry.asm','stormbolt_sync_pack_dispatch.h','stormbolt_sync_complete_dispatch.h',
              'stormbolt_power_cell_sync_frames.h','stormbolt_sync_entries.cpp',
              'stormbolt_sync_pack_entry.asm','stormbolt_sync_complete_entry.asm','stormbolt_remote_update_dispatch.h',
              'stormbolt_power_cell_remote_native_adapter.h','stormbolt_power_cell_remote_inputs.h',
              'stormbolt_power_cell_sync_codec.h','stormbolt_remote_update_entry.cpp','stormbolt_remote_update_entry.asm']:
  path=root/name;data=path.read_bytes()
  adapters.append({'path':path.relative_to(P).as_posix(),'sha256':hashlib.sha256(data).hexdigest(),
                   'operation':'reuse existing selected helper functions; transitive shell_hook/types/MinHook headers required but no shooting hook is registered'})
 # The merge consumer needs the complete selected adapters' local include
 # closure, including publication and transaction helpers added by the gun chat.
 # Keep one copy in runtime; enumerate dependencies instead of copying headers.
 pending=[P/row['path'] for row in files+adapters];seen_paths=set();closure=[]
 while pending:
  path=pending.pop().resolve()
  if path in seen_paths:continue
  seen_paths.add(path)
  assert path.is_relative_to(P),path
  assert 'selftest' not in path.name.lower(),path
  data=path.read_bytes()
  dependencies=[]
  for dep in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',data.decode('utf-8-sig'),re.M):
   target=(path.parent/dep).resolve()
   if not target.is_file() and dep=='MinHook.h':
    target=P/'runtime/vendor/minhook-1.3.4/include/MinHook.h'
   assert target.is_file(),(path,dep)
   assert target.is_relative_to(P),target
   dependencies.append(target.relative_to(P).as_posix());pending.append(target)
  closure.append({'path':path.relative_to(P).as_posix(),'bytes':len(data),
                  'sha256':hashlib.sha256(data).hexdigest(),'project_includes':dependencies,
                  'operation':'reuse_single_shared_source_no_copy_or_execution'})
 report={'schema_version' :1,'module':'horizon-other','status':'shared_source_reuse_contract_integration_pending',
   'installation_allowed':False,'executed':False,'source_files':sorted(files,key=lambda r:r['path']),
   'evidence':evidence,'native_adapter_sources':adapters,
   'selected_adapter_dependency_closure':sorted(closure,key=lambda row:row['path']),
   'module_adapter':'src/horizon_shield_status.h/.cpp, horizon_shield_status_calls.h/.cpp and horizon_shield_status_session.h/.cpp, horizon_shield_growth_calls.h/.cpp, horizon_shield_aux_lifecycles.h/.cpp, horizon_shield_loop_call.h/.cpp, horizon_shield_sync_calls.h/.cpp, horizon_shield_remote_call.h/.cpp, horizon_shield_multiplier_call.h/.cpp + multiplier_entry.asm, horizon_shield_reaction_call.h/.cpp + reaction_entry.asm, horizon_shield_equipment_session.h/.cpp (equipment-only composition), horizon_shield_native_bindings.h/.cpp (12 endpoint byte guards and typed addresses), horizon_other_activation.h/.cpp (quest mapping + equipment rollback order); borrows shared host store; explicit native call owner implemented but not invoked or auto-bootstrapped',
   'native_calls':{'sites':[{'rva':'0x11c6ce3','original':'488b0db67fe403'},{'rva':'0x11c7f18','original':'e883a9d90084c0'}],
      'installed':False,'reaction_false_path_site':'0x1217b3e (6bytes), R15b local predicate only; own MASM once','multiplier_site':'0x11b9d44 (7bytes); XMM6.low before native skill14; new module MASM must link once','remote_apply_site':'0x11b034a','sync_producer_sites':['0x1199e99','0x1199f4b'],
      'sync_drain_rule':'prevent new producer frames, finish captured frames with completion callback still installed; only then restore CALLs/withdraw; in_flight rejects premature restoration',
      'loop_update_site':'0x12ee81d','growth_sites':['0x121b37f','0x121b39e','0x121b3a7'],
      'growth_requires':'shared status, damage, loop and sync stores with authenticated native lifecycles; complete growth bindings; no shooting prerequisite',
      'auxiliary_lifecycle_composer':'ShieldAuxiliaryLifecycles guards and queues damage1216EF0/1217340 plus update121B8D0/absorb118EFE0, loop12EA460/12EAA40, sync11927A0/1193610; no new stores',
      'requires_existing_lifecycle_hooks':True,
      'equipment_only_lifecycle_composer':'ShieldStatusSession borrows common store, guards original constructor/reset/destroy and queues those three hooks in shared transaction',
      'lifecycle_owners':{'construct':'0x11b6450','reset':'0x11c34a0','destroy':'0x11b68a0','vtable':'0x322c268'},
      'sequence':'prepare/publish while quiescent ->shared hook commit ->install twoCALLs ->leave quiescent region',
      'restore':'restore bothCALLs and drain frames ->withdraw callbacks ->shared hook stop; retain all state if restoration_pending',
      'link_rule':'status/timer entry globals CPP and status/timer/growth MASM exactly once in final unified binary; diagnostic copies stay in build/link-only'},
   'ownership':{'equipment_skill_and_other_content':'horizon-other',
      'storm_gun_shooting_class48_and_overload':'storm-gun module',
      'shared_native_lifecycle_hooks_and_sidecar_store':'one unified owner in final merge; both modules borrow same per-status state',
      'standalone_other_module':'must compose required equipment-only lifecycle/effect adapters without requiring Stormslinger equipped'},
   'reuse_contract':['Skill predicate must remain native query(actor,209,parameter0,default0)>0.',
     'Keep status-manager generation separate from weapon generation; weapon switch must not reset shared equipment state.',
     'Share one store/constructor/destructor/update hook for each native owner; never install one copy per module.',
     'Preserve loaded skill resource values and exact native arithmetic position; no blanket multiplier on final damage.',
     'Do not import StormboltPowerCellSession wholesale: it also owns class48 shooting integration and shared transaction mutations.',
     'Hashes identify reviewed source revisions; re-read changed shared files before composing final module.'],
   'pending':['Certify PS/PC effect pairing and required hooks for equipment independent of weapon.',
     'Resolve full parameter resource ownership, lifecycle, pending refresh scheduling, effect/audio triggers and synchronization.',
     'Compose equipment module adapters and shared activation/rollback with final unified owner.'],
   'limits':['Existing other-chat tests are not dynamic validation performed by this chat.',
      'This contract does not mark existing shared primitives or whole equipment mechanism complete.']}
 out=M/'merge/shield-shared-core.json';out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
 print(json.dumps({'reviewed_shared_headers':len(files),'evidence_files':len(evidence),'integration_complete':False,'executed':False}))
if __name__=='__main__':main()
