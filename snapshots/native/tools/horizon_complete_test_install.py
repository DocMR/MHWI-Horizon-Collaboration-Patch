"""Developer-managed local test install/restore; never ships in the release ZIP."""
from pathlib import Path
import argparse,json,shutil
from patch_uninstall_plan import checked_path,digest
from stormslinger_release_manager import verify,require_game_closed
from vanilla_package_files import install,uninstall,check_game_version

def save_json(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
def snapshot_saves(roots,destination):
 rows=[]
 for index,root in enumerate(roots):
  root=Path(root)
  if not root.exists():continue
  for source in sorted(root.rglob('*')):
   if not source.is_file():continue
   relative=source.relative_to(root).as_posix();target=destination/str(index)/relative
   checked_path(root,relative);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
   value=digest(source)
   if digest(target)!=value:raise ValueError('Save backup mismatch')
   rows.append({'root':str(root),'relative_path':relative,'sha256':value})
 save_json(destination.parent/(destination.name+'.json'),rows)
 return rows

def check_saves(rows):
 if any(digest(checked_path(Path(r['root']),r['relative_path']))!=r['sha256'] for r in rows):
  raise ValueError('Latest saves changed during file operation')

def support_files(package,manifest):
 rows=[dict(r) for r in manifest['auxiliary_files']]
 rows.append({'relative_path':'StormslingerSupport/release-manifest.json',
  'sha256':digest(package/'release-manifest.json')})
 allowed={'Export-Feedback.cmd','Uninstall-Patch.cmd','StormslingerSupport/Export-Feedback.ps1',
  'StormslingerSupport/Uninstall-Patch.ps1','StormslingerSupport/LICENSE-minhook.txt','StormslingerSupport/release-manifest.json'}
 if {r['relative_path'] for r in rows}!=allowed:raise ValueError('Unexpected support inventory')
 for row in rows:
  if digest(checked_path(package/'support-root',row['relative_path']))!=row['sha256']:
   raise ValueError('Support source mismatch: '+row['relative_path'])
 return rows

def install_test(package,game,session,save_roots):
 manifest,payload,files=verify(package);aux=support_files(package,manifest)
 if session.exists():raise ValueError('Use a new test session')
 for root in [package,game,session]:
  for other in [package,game,session]:
   if root!=other and (root.is_relative_to(other) or other.is_relative_to(root)):
    raise ValueError('Package/game/session roots must be disjoint')
 session.mkdir(parents=True)
 saves=snapshot_saves(save_roots,session/'save-before')
 state={'status':'preparing','package':str(package),'game':str(game),'save_roots':[str(r) for r in save_roots],
  'package_manifest_sha256':digest(package/'release-manifest.json'),'support':[],'save_rollback':False}
 for i,row in enumerate(aux):
  name=row['relative_path'];target=checked_path(game,name);original=digest(target)
  backup=f'support-before/{i}.bin' if original else None
  if backup:
   out=session/backup;out.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,out)
   if digest(out)!=original:raise ValueError('Support backup mismatch')
  state['support'].append({'relative_path':name,'installed_sha256':row['sha256'],
   'original_sha256':original,'backup':backup})
 save_json(session/'session.json',state)
 install_state=session/'payload-state';install_state.mkdir()
 install(payload,game,install_state,files)
 state['status']='payload_installed';save_json(session/'session.json',state)
 for row in state['support']:
  target=checked_path(game,row['relative_path']);target.parent.mkdir(parents=True,exist_ok=True)
  shutil.copy2(package/'support-root'/row['relative_path'],target)
  if digest(target)!=row['installed_sha256']:raise ValueError('Support installation mismatch')
 check_saves(saves);state['status']='installed_waiting_for_user_start';save_json(session/'session.json',state)
 return state

def restore_test(session):
 state=json.loads((session/'session.json').read_text(encoding='utf-8-sig'))
 if state['status']=='restored':raise ValueError('Already restored')
 game=Path(state['game']);payload_state=session/'payload-state'
 native=json.loads((payload_state/'vanilla-install.json').read_text(encoding='utf-8-sig'))
 # Preserve changed files rather than allow the low-level restoration to overwrite them.
 for row in native['files']+state['support']:
  if digest(checked_path(game,row['relative_path'])) not in (row['installed_sha256'],row['original_sha256']):
   raise ValueError('Changed after test installation; preserve before restoring: '+row['relative_path'])
 for row in state['support']:
  if row['backup'] and digest(checked_path(session,row['backup']))!=row['original_sha256']:
   raise ValueError('Support original backup mismatch')
 # Keep latest post-test saves; never copy the pre-test saves back.
 if (session/'save-after.json').exists():raise ValueError('Post-test backup exists; inspect previous restoration before retry')
 saves=snapshot_saves([Path(r) for r in state['save_roots']],session/'save-after')
 uninstall(game,payload_state)
 for row in reversed(state['support']):
  target=checked_path(game,row['relative_path'])
  if row['backup']:shutil.copy2(session/row['backup'],target)
  else:target.unlink(missing_ok=True)
  if digest(target)!=row['original_sha256']:raise ValueError('Support restoration mismatch')
 support=game/'StormslingerSupport'
 if support.is_dir() and not any(support.iterdir()):support.rmdir()
 check_saves(saves);state['status']='restored';save_json(session/'session.json',state)
 save_json(session/'restore-verification.json',{'status':'restored','latest_saves_preserved':True,
  'payload_files':len(native['files']),'support_files':len(state['support']),'save_rollback':False})
 return state

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('action',choices=['check','install','restore'])
 parser.add_argument('--package-root',type=Path)
 parser.add_argument('--game-root',type=Path)
 parser.add_argument('--session-root',type=Path)
 parser.add_argument('--save-root',type=Path,action='append',default=[])
 a=parser.parse_args()
 if a.action=='check':
  if not a.package_root:parser.error('check requires package root')
  m,_,files=verify(a.package_root.resolve());support_files(a.package_root.resolve(),m)
  print(json.dumps({'payload':len(files),'support':6,'status':'checked_not_installed'}));return
 require_game_closed()
 if not a.session_root:parser.error('install/restore requires session root')
 if a.action=='install':
  if not a.package_root or not a.game_root:parser.error('install requires package and game roots')
  check_game_version(a.game_root.resolve())
  result=install_test(a.package_root.resolve(),a.game_root.resolve(),a.session_root.resolve(),[p.resolve() for p in a.save_root])
 else:result=restore_test(a.session_root.resolve())
 print(json.dumps({'status':result['status']}))
if __name__=='__main__':main()
