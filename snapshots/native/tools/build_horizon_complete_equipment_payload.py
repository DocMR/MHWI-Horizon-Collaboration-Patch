"""Compose equipment text onto the frozen task payload, preserving accepted files."""
from pathlib import Path
import json,hashlib,shutil
from merge_horizon_other_gmd import merge
P=Path(__file__).resolve().parents[1]
M=P/'staging/Horizon-complete-integration-20261021'
BASE=P/'staging/Horizon-complete-payload-20261021-c1'
OUT=P/'staging/Horizon-complete-equipment-payload-20261021-c2'
TABLES={'armor','ot_armor','bow','ot_weapon','armor_series','ou_bow','ot_series','wep_series','l_delivery'}
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 if OUT.exists():raise ValueError('Preserve existing frozen equipment candidate')
 source=json.loads((M/'merge/text-deltas.json').read_text(encoding='utf-8-sig'))
 changes=[];outputs={}
 for row in source['files']:
  table=row['target'].rsplit('/',1)[-1].rsplit('_',1)[0]
  if table not in TABLES:continue
  baseline=P/'MHWI解包/chunk'/row['target'];raw=baseline.read_bytes()
  assert sha(raw)==row['base_sha256'],row['target']
  path=BASE/'nativePC'/row['target'];before=path.read_bytes() if path.exists() else raw
  after=merge(before,row['changes']);outputs[row['target']]=after
  changes.append({'path':'nativePC/'+row['target'],'keys':len(row['changes']),'input_sha256':sha(before),'output_sha256':sha(after)})
 assert len(changes)==126
 parameters=json.loads((M/'merge/equipment-table-baselines.json').read_text(encoding='utf-8-sig'))
 for row in parameters['files']:
  assert row['operation']=='preserve_existing_pc_table' and row['ps_pc_byte_identical']
  assert sha((P/'MHWI解包/chunk'/row['path']).read_bytes())==row['sha256']
 shutil.copytree(BASE,OUT)
 for relative,data in outputs.items():
  path=OUT/'nativePC'/relative;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
 owned=set(outputs)
 for path in (BASE/'nativePC').rglob('*'):
  if path.is_file() and path.relative_to(BASE/'nativePC').as_posix() not in owned:
   assert path.read_bytes()==(OUT/'nativePC'/path.relative_to(BASE/'nativePC')).read_bytes()
 files=[{'path':f.relative_to(OUT).as_posix(),'size':f.stat().st_size,'sha256':sha(f.read_bytes())} for f in sorted((OUT/'nativePC').rglob('*')) if f.is_file()]
 report={'status':'equipment_text_composed_no_game_install_or_live_acceptance','candidate':OUT.relative_to(P).as_posix(),'resource_files':len(files),'text_files_merged':changes,
  'native_parameter_tables_preserved':len(parameters['files']),'unrelated_task_stormslinger_bytes_preserved':True,
  'remaining':['Original craft/select/upgrade/economic/save paths still require grouped live acceptance','Layered mapper ownership semantics and eligibility reviewed separately','154 guildcard reward-description updates wired in generator input but not in old delta; apply in interaction item later'],
  'files':files}
 (P/'research/horizon-complete-equipment-payload-20261021.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({k:report[k] for k in ['status','resource_files','native_parameter_tables_preserved','unrelated_task_stormslinger_bytes_preserved']}))
if __name__=='__main__':main()
