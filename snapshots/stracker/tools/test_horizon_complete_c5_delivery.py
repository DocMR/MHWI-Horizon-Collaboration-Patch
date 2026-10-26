"""Actual C5 ZIP install/uninstall in a workspace fixture, UI/process inventory mocked."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
P=Path(__file__).resolve().parents[1]
R=P/'research/horizon-complete-c5-delivery-fixture-20261022'
D=P/'staging/Horizon-complete-cumulative-c5-20261022/distribution/Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 assert not R.exists();R.mkdir();game=R/'中文 Game';game.mkdir()
 shutil.copy2('H:/Steam/steamapps/common/Monster Hunter World/MonsterHunterWorld.exe',game/'MonsterHunterWorld.exe')
 (game/'nativePC').mkdir();(game/'savedata_backup').mkdir()
 (game/'nativePC/external.bin').write_bytes(b'external');(game/'savedata_backup/save.bin').write_bytes(b'latest save')
 fixture=R/'installer.ps1';source=(D/'StormslingerInstaller/Install-Patch.ps1').read_text(encoding='utf-8-sig')
 fixture.write_text('\ufeff'+source.replace('[Windows.Forms.MessageBox]','[HorizonInstallerFixtureDialog]'),encoding='utf-8')
 wrapper=P/'research/horizon-complete-extract-fixture-20261021-r2/invoke-fixture.ps1';assert wrapper.exists()
 env=dict(os.environ,HZ_FIXTURE_MESSAGES=str(R/'messages.txt'),HZ_FIXTURE_ANSWER='Yes',HZ_FIXTURE_RUNNING='no',LOCALAPPDATA=str(R/'profile'))
 (R/'profile').mkdir()
 archive=D/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch.zip'
 cmd=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(wrapper),'-FixtureScript',str(fixture),'-ArchivePath',str(archive),'-GameRoot',str(game)]
 run=subprocess.run(cmd,env=env,cwd=P,capture_output=True,timeout=120);(R/'install-output.txt').write_bytes(run.stdout+run.stderr);assert run.returncode==0
 manifest=json.loads((game/'StormslingerSupport/release-manifest.json').read_text(encoding='utf-8-sig'))
 for row in manifest['files']+manifest['auxiliary_files']:assert sha(game/row['relative_path'])==row['sha256']
 for name in ['gc_data_title1.gcod','gc_data_pose.gcod']:assert (game/'nativePC/common/guildcard'/name).exists()
 uninstall=R/'uninstall-wrapper.ps1';uninstall.write_text('\ufeffparam([string]$Script,[string]$GameRoot)\nfunction Get-CimInstance {}\n& $Script -GameRoot $GameRoot -NonInteractive\nexit $LASTEXITCODE\n',encoding='utf-8')
 cmd=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(uninstall),'-Script',str(game/'StormslingerSupport/Uninstall-Patch.ps1'),'-GameRoot',str(game)]
 run=subprocess.run(cmd,env=env,cwd=P,capture_output=True,timeout=120);(R/'uninstall-output.txt').write_bytes(run.stdout+run.stderr);assert run.returncode==0
 assert (game/'nativePC/external.bin').read_bytes()==b'external' and (game/'savedata_backup/save.bin').read_bytes()==b'latest save'
 assert not (game/'dinput8.dll').exists() and not (game/'StormslingerSupport').exists()
 for row in manifest['files']+manifest['auxiliary_files']:assert not (game/row['relative_path']).exists()
 assert not list((R/'profile').rglob('*'))
 report={'status':'passed','archive_sha256':sha(archive),'installed_payload_files':len(manifest['files']),'new_gcod_tables_installed_and_removed':True,'external_file_and_latest_save_preserved':True,'pre_uninstall_backup_created':False,'limits':['UI answer and process inventory mocked; explicit workspace path used, not real Steam discovery or real game launch.','This is not another-computer acceptance.'],'real_game_modified':False}
 (P/'staging/Horizon-complete-cumulative-c5-20261022/isolated-delivery-evidence.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report))
if __name__=='__main__':main()
