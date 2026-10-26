"""Verify inherited current-source build and concrete shared-owner collisions."""
from pathlib import Path
import hashlib,json,struct
P=Path(__file__).resolve().parents[1]
M=P/'staging/Horizon-complete-integration-20261021'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 source=P/'staging/Horizon-other-module'
 changes=[]
 for p in (source/'src').iterdir():
  if p.is_file() and sha(p)!=sha(M/'src'/p.name):
   changes.append({'name':p.name,'handoff_sha256':sha(p),'current_sha256':sha(M/'src'/p.name)})
 expected={'horizon_module_prepare.h','horizon_module_prepare.cpp','horizon_mapping_transaction.h',
  'horizon_mapping_transaction.cpp','horizon_other_activation.h','horizon_other_host.h',
  'horizon_native_integration.h','horizon_native_integration.cpp'}
 assert {x['name'] for x in changes}==expected,changes
 test=(P/'research/horizon-complete-shared-owners-test-20261021.log').read_text(encoding='utf-8-sig')
 assert 'PASS borrowed tutorial-only ownership' in test and 'composed bow+Stormslinger clones' in test
 objects=list((M/'build').glob('horizon_*.obj'));assert len(objects)==39,len(objects)
 dll=M/'build/link-only/horizon_other_linkcheck.dll';b=dll.read_bytes();pe=struct.unpack_from('<I',b,60)[0];opt=pe+24
 assert struct.unpack_from('<H',b,pe+4)[0]==0x8664
 assert struct.unpack_from('<I',b,opt+16)[0]==0
 export_rva,export_size=struct.unpack_from('<II',b,opt+112)
 sections=[]
 for i in range(struct.unpack_from('<H',b,pe+6)[0]):
  off=opt+struct.unpack_from('<H',b,pe+20)[0]+40*i
  name=b[off:off+8].split(b'\0')[0].decode('ascii')
  virtual_size,rva,raw_size,raw=struct.unpack_from('<IIII',b,off+8)
  flags=struct.unpack_from('<I',b,off+36)[0]
  sections.append((name,rva,max(virtual_size,raw_size),raw,flags))
 def at(rva):
  for _,start,size,raw,_ in sections:
   if start<=rva<start+size:return raw+rva-start
  raise AssertionError(hex(rva))
 def name_at(rva):
  off=at(rva);return b[off:b.index(b'\0',off)].decode('ascii')
 directory=at(export_rva)
 functions,names,addresses,name_table,ordinals=struct.unpack_from('<IIIII',b,directory+20)
 assert functions==names==1
 assert name_at(struct.unpack_from('<I',b,at(name_table))[0])=='stormbolt_live_probe'
 ordinal=struct.unpack_from('<H',b,at(ordinals))[0]
 target=struct.unpack_from('<I',b,at(addresses)+ordinal*4)[0]
 section=next(x for x in sections if x[1]<=target<x[1]+x[2])
 assert not section[4]&0x20000000,section
 exports=[{'name':'stormbolt_live_probe','kind':'data','section':section[0],'rva':hex(target)}]
 mapping=json.loads((M/'merge/quest-save-map-candidate.json').read_text(encoding='utf-8-sig'))
 ids=mapping['candidate_entries'];assert ids[-1]==-1 and len(ids)==359 and len(set(ids[:-1]))==358
 assert ids[352:358]==[67101,67102,67607,67802,67803,67804]
 runtime=(P/'runtime/src/quest_bridge.cpp').read_text(encoding='utf-8')
 assert '#if !defined(STORMSLINGER_NATIVE_QUEST_SAVE)\nint countHook' in runtime
 assert '#if !defined(STORMSLINGER_NATIVE_QUEST_SAVE)\n   {10,' in runtime
 collisions=[
  {'area':'Quest list/data/classification','addresses':['0x478A20','0x1A10170','0x19751B0'],
   'current':'Both runtime two-quest bridge and other module resource hooks own these entries',
   'required':'One six-quest owner; retain original grades/prerequisites; no second hook registration'},
  {'area':'Native quest-save mapping','addresses':['0x324FAC0 and21 RIP references'],
   'current':'Both modules prepare their own shared six-ID mapping objects; native-save flag already omits old+2 wrappers',
   'required':'One prepared table/publication and restoration owner; preserve original352 prefix and six appended indices'},
  {'area':'Bow and Stormslinger weapon tree','addresses':['0x13732F0','0x13738B0'],
   'current':'Both modules own these entries; other module has CloneComposer for shared edits',
   'required':'Compose edits in a single authenticated original clone/predicate, then install once'},
  {'area':'Equipment status/sync lifecycle','addresses':['Existing shared status/timer/loop/sync consumers'],
   'current':'OtherModuleHost already accepts borrowed transaction and4 stores; runtime currently has quest/shooting transactions',
   'required':'Single ownership for each shared consumer/store; use staged prepare/commit/activate/restore rather than a second autonomous host'},
 ]
 report={'status':'shared_mapping_and_tree_composition_fixtures_passed_root_integration_pending','static_objects':len(objects),
  'candidate':str(M.relative_to(P)),'link_only_dll_sha256':sha(dll),'link_only_entry_point':0,'link_only_exports':exports,
  'executed_or_installed':False,'generators_run':False,'fixture_log':'research/horizon-complete-shared-owners-test-20261021.log','source_matches_handoff':False,'intentional_source_changes':changes,
  'native_quest_count':352,'extended_count':358,'storm_quest_indices':{'67802':355,'67803':356},
  'duplicate_old_count_wrappers':'Not present when STORMSLINGER_NATIVE_QUEST_SAVE=ON; historical unconditional claim is not current',
  'shared_ownership':collisions,
  'unresolved':['Production entry actual timing and object lifetime before new status calls','Non-Stormslinger quest-save/ownership semantic gaps retained','Current source build is not activation or live acceptance'],
  'files':[{'path':f.relative_to(P).as_posix(),'sha256':sha(f)} for f in sorted((M/'src').iterdir()) if f.is_file()]}
 (P/'research/horizon-complete-ownership-20261021.json').write_text(json.dumps(report,indent=2)+'\n')
 (M/'merge/current-source-build-20261021.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items() if k not in ('files','shared_ownership')}))
if __name__=='__main__':main()
