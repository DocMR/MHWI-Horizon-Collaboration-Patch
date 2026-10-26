from pathlib import Path
import hashlib,json,os,shutil,subprocess,zipfile
P=Path(__file__).resolve().parents[1]
R=P/'research/horizon-mapping-link-fix-install-test-20261023'
BASE='Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
D=P/(BASE+' - Release Packages')/'Startup Fix'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def files(p):return {f.relative_to(p).as_posix():sha(f) for f in p.rglob('*') if f.is_file()}
def run(cmd,env,log):
 r=subprocess.run(cmd,env=env,capture_output=True,timeout=180);log.write_bytes(r.stdout+r.stderr);return r.returncode
def main():
 R.mkdir(exist_ok=False)
 source=(P/'tools/Install-HorizonUnifiedPatch.ps1').read_text(encoding='utf-8-sig')
 with zipfile.ZipFile(D/(BASE+' - Unified Edition.zip')) as z:entries={n:z.read(n) for n in z.namelist()}
 assert len(entries)==4
 # UI and game-process mocking only; fake EXE skips the inner game build gate.
 for n in ['tools/native.ps1','tools/stracker.ps1']:
  old=hashlib.sha256(entries[n]).hexdigest()
  s=entries[n].decode('utf-8-sig').replace('[Windows.Forms.MessageBox]','[HorizonInstallerFixtureDialog]').replace('if((FileHash $exe) -ne $manifest.game_exe_sha256)','if($false)')
  entries[n]=s.encode('utf-8-sig');source=source.replace(old,hashlib.sha256(entries[n]).hexdigest())
 source=source.replace('[Windows.Forms.MessageBox]','[HorizonInstallerFixtureDialog]')
 fixture=R/'install.ps1';fixture.write_text(source,encoding='utf-8-sig')
 archive=R/'fixture.zip'
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_STORED) as z:
  for n,b in entries.items():z.writestr(n,b)
 wrapper=P/'research/horizon-complete-extract-fixture-20261021-r2/invoke-fixture.ps1'
 uw=R/'uninstall.ps1';uw.write_text('param([string]$Script,[string]$GameRoot,[string]$BackupRoot)\nfunction Get-CimInstance {}\n& $Script -GameRoot $GameRoot -BackupRoot $BackupRoot -NonInteractive\nexit $LASTEXITCODE\n',encoding='utf-8-sig')
 results=[]
 for name in ['stracker']:
  case=R/name;game=case/'Game';game.mkdir(parents=True)
  (game/'MonsterHunterWorld.exe').write_bytes(b'fixture EXE')
  (game/'save.bin').write_bytes(b'latest save untouched')
  if name in ['stracker','disabled','unknown-quest']:
   for f in ['dinput8.dll','loader.dll','loader-config.json']:shutil.copy2(P/'Monster Hunter World'/f,game/f)
   plugins=game/'nativePC/plugins';plugins.mkdir(parents=True)
   shutil.copy2(P/'Monster Hunter World/nativePC/plugins/QuestLoader.dll',plugins/'QuestLoader.dll')
  if name=='unknown-proxy':(game/'dinput8.dll').write_bytes(b'unknown')
  if name=='alternate-loader':(game/'winmm.dll').write_bytes(b'unknown alternate')
  if name=='incomplete':shutil.copy2(P/'Monster Hunter World/dinput8.dll',game/'dinput8.dll')
  if name=='disabled':(game/'loader-config.json').write_text('{"enablePluginLoader":false}')
  if name=='unknown-quest':(game/'nativePC/plugins/QuestLoader.dll').write_bytes(b'unknown')
  if name=='orphan-plugin':
   path=game/'nativePC/plugins/x.dll';path.parent.mkdir(parents=True);path.write_bytes(b'orphan')
  if name in ['clean','stracker']:
   path=game/'nativePC/common/guildcard/gc_data_pose.gcod';path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'original shared file')
  before=files(game);backup=case/'backup';shutil.copytree(game,backup)
  profile=case/'profile';profile.mkdir()
  env=dict(os.environ,LOCALAPPDATA=str(profile),HZ_FIXTURE_MESSAGES=str(case/'messages.txt'),HZ_FIXTURE_RUNNING='no',HZ_FIXTURE_ANSWER='No')
  cmd=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(wrapper),'-FixtureScript',str(fixture),'-ArchivePath',str(archive),'-GameRoot',str(game)]
  if name not in ['clean','stracker']:
   assert run(cmd,env,case/'reject.txt')==1 and files(game)==before,name
   assert '1982?tab=files' in (case/'messages.txt').read_text(),name
   results.append(dict(case=name,rejected_without_writes=True));continue
  assert run(cmd,env,case/'cancel.txt')==2 and files(game)==before,name
  env['HZ_FIXTURE_ANSWER']='Yes'
  for iteration in [1]:
   assert run(cmd,env,case/f'install-{iteration}.txt')==0,name
   m=json.loads((game/'StormslingerSupport/release-manifest.json').read_text(encoding='utf-8-sig'))
   assert m['variant']==('standalone' if name=='clean' else 'stracker')
   for row in m['files']+m['auxiliary_files']:assert sha(game/row['relative_path'])==row['sha256']
   if name=='stracker':assert all(sha(game/f)==before[f] for f in ['dinput8.dll','loader.dll','loader-config.json','nativePC/plugins/QuestLoader.dll'])
   log=case/'safe-logs';log.mkdir(exist_ok=True)
   (log/'runtime.jsonl').write_text(json.dumps(dict(event='startup_component',component=22,detail=2,native_status=0,private_unlisted='drop-me'))+'\n')
   feedback=case/'feedback'
   export=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(game/'StormslingerSupport/Export-Feedback.ps1'),'-PackageRoot',str(game/'StormslingerSupport'),'-GameRoot',str(game),'-LogRoot',str(log),'-OutputDirectory',str(feedback),'-NonInteractive']
   assert run(export,env,case/'feedback.txt')==0
   with zipfile.ZipFile(next(feedback.glob('*.zip'))) as z:
    event=json.loads(z.read('logs/runtime.jsonl').decode('utf-8-sig').strip())
    assert event['component']==22 and event['detail']==2 and event['native_status']==0 and 'private_unlisted' not in event
   # Reinstall in place must select the same mode and remain usable by uninstaller.
   assert run(cmd,env,case/f'reinstall-{iteration}.txt')==0,name
   assert run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(uw),'-Script',str(game/'StormslingerSupport/Uninstall-Patch.ps1'),'-GameRoot',str(game),'-BackupRoot',str(backup)],env,case/f'uninstall-{iteration}.txt')==0,name
   assert files(game)==before,name
  results.append(dict(case=name,automatic_selection=True,cancel_unchanged=True,reinstall_and_restore_once=True))
 report=dict(status='passed',cases=results,limits='Real inner payloads, dispatcher and file I/O; fake EXE build check skipped; UI/process mocked. No game directory modified or game launched.')
 (R/'results.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
if __name__=='__main__':main()
