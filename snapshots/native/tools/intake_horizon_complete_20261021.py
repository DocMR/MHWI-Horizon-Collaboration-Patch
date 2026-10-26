"""Preserve accepted standalone and inventory the inherited Horizon candidate."""
import hashlib,json,shutil
from pathlib import Path
P=Path(__file__).resolve().parents[1]
M=P/'staging/Horizon-other-module'
OUT=P/'backups/Stormslinger-before-Horizon-complete-20261021'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def save(p,d):p.write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def main():
 if OUT.exists():raise ValueError('Do not overwrite the standalone backup')
 OUT.mkdir(parents=True)
 distribution=P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
 shutil.copytree(distribution,OUT/'standalone-release')
 for name in ('source.zip','release-index.json','final-archive-verification.json'):
  shutil.copy2(P/'dist/Stormslinger-native-1.0.0-final-20261021'/name,OUT/name)
 shutil.copytree(P/'runtime/src',OUT/'runtime/src')
 shutil.copy2(P/'runtime/CMakeLists.txt',OUT/'runtime/CMakeLists.txt')
 tools=OUT/'tools';tools.mkdir()
 for name in ('Uninstall-ExtractPatch.ps1','Export-Feedback.ps1','Export-Feedback.cmd','View-StormslingerFeedback.ps1','package_stormslinger_extract_release.py','test_stormslinger_extract_uninstall.py','test_stormslinger_extract_portability.py'):
  shutil.copy2(P/'tools'/name,tools/name)
 for name in ('项目目标.md','项目进度.md','新聊天交接.md','行家雷暴弩枪移植计划.md'):
  shutil.copy2(P/name,OUT/name)
 files=[{'path':f.relative_to(OUT).as_posix(),'size':f.stat().st_size,'sha256':sha(f)} for f in sorted(OUT.rglob('*')) if f.is_file()]
 for f in distribution.rglob('*'):
  if f.is_file():assert sha(f)==sha(OUT/'standalone-release'/f.relative_to(distribution))
 save(OUT/'backup-manifest.json',{'status':'verified_standalone_backup','files':files,'game_modified':False})
 manifest=read(M/'module-manifest.json');resources=[]
 for r in manifest['resources']:
  f=M/'resources/nativePC'/r['path'];assert f.stat().st_size==r['size'] and sha(f)==r['sha256'],str(f)
  resources.append(r)
 assert len(resources)==421 and sum(r['size'] for r in resources)==206663000
 for name in manifest['merge_inputs']:assert (M/name).is_file(),name
 for name,value in manifest['final_chat_handoff_2026_10_20']['key_artifact_sha256'].items():assert sha(P/name)==value,name
 receipt=read(M/'merge/catalog-build.json');stale=[];known=set()
 for row in receipt['files']:
  f=M/row['file'];known.add(f.resolve())
  if f.is_file() and sha(f)!=row['sha256']:stale.append(row['file'])
 unrecorded=[f.relative_to(M).as_posix() for f in (M/'src').iterdir() if f.is_file() and f.resolve() not in known]
 artifacts=[{'path':str(f.relative_to(P)),'size':f.stat().st_size,'sha256':sha(f)} for f in sorted(P.glob('research/horizon-other-*.json'))]
 report={'status':'handoff_intake_verified_not_installable','standalone_backup':str(OUT.relative_to(P)),
  'resources':len(resources),'resource_bytes':sum(r['size'] for r in resources),'merge_inputs':len(manifest['merge_inputs']),
  'resource_and_key_handoff_hashes_match':True,'stale_recorded_build_files':stale,'unrecorded_current_sources':unrecorded,
  'evidence_index':artifacts,'module_content_complete':False,'game_modified':False,
  'next':'Unified native host / quest mapping ownership, preserving existing package tooling'}
 save(P/'research/horizon-complete-intake-20261021.json',report)
 print(json.dumps({k:v for k,v in report.items() if k!='evidence_index'}))
if __name__=='__main__':main()
