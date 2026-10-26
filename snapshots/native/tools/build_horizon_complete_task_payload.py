"""Compose retained native payloads and task material text; no extraction/install."""
from pathlib import Path
import hashlib,json,zipfile
from merge_horizon_other_gmd import merge
P=Path(__file__).resolve().parents[1]
M=P/'staging/Horizon-complete-integration-20261021'
OUT=P/'staging/Horizon-complete-payload-20261021-c1'
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 if OUT.exists():raise ValueError('Candidate already exists; preserve it')
 archive=P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch/Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch.zip'
 assert sha(archive.read_bytes())=='e05a011dccca4fe6a3f3de19a2faf3592906cbfa2060956102b9036ef4b60eb4'
 with zipfile.ZipFile(archive) as z:
  data={n:z.read(n) for n in z.namelist() if n.startswith('nativePC/') and not n.endswith('/')}
 original=dict(data);overlap=[]
 module=json.loads((M/'module-manifest.json').read_text(encoding='utf-8-sig'))
 for row in module['resources']:
  name='nativePC/'+row['path'];blob=(M/'resources'/name).read_bytes()
  assert len(blob)==row['size'] and sha(blob)==row['sha256'],name
  if name in data:
   assert data[name]==blob,('resource collision',name)
   overlap.append(name)
  data[name]=blob
 text=json.loads((M/'merge/text-deltas.json').read_text(encoding='utf-8-sig'))
 applied=[]
 for row in text['files']:
  if not row['target'].startswith('common/text/steam/item_'):continue
  name='nativePC/'+row['target'];base=P/'MHWI解包/chunk'/row['target']
  assert sha(base.read_bytes())==row['base_sha256'],name
  before=data.get(name,base.read_bytes());after=merge(before,row['changes'])
  applied.append({'path':name,'input_sha256':sha(before),'output_sha256':sha(after),'keys':len(row['changes'])})
  data[name]=after
 assert len(applied)==14
 # All original weapon/HUD/audio files survive exactly, except material strings
 # explicitly merged by stable key above.
 edited={r['path'] for r in applied}
 assert all(data[n]==blob for n,blob in original.items() if n not in edited)
 for q in [67101,67102,67607,67802,67803,67804]:
  assert f'nativePC/quest/questData_{q}.mib' in data
  for lang in ['ara','chS','chT','eng','fre','ger','ita','jpn','kor','pol','ptB','rus','spL','spa']:
   assert f'nativePC/common/text/quest/q{q}_{lang}.gmd' in data
 for name,blob in sorted(data.items()):
  out=OUT/name;out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes(blob)
 report={'status':'resource_union_and_quest_material_text_merged_not_installed',
  'candidate':OUT.relative_to(P).as_posix(),'resources':len(data),'deduplicated_identical':overlap,
  'material_text':applied,'standalone_other_files_preserved':True,'shared_equipment_tables':'Not regenerated; preserve native PC baselines until explicit edits are required',
  'remaining':['Task resource dependency/caller/save verification and grouped live acceptance','Remaining equipment/interaction text deltas are staged inputs, not yet merged'],
  'files':[{'path':n,'size':len(b),'sha256':sha(b)} for n,b in sorted(data.items())]}
 (P/'research/horizon-complete-task-payload-20261021.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({k:report[k] for k in ['status','resources','standalone_other_files_preserved']}))
if __name__=='__main__':main()
