"""Exercise the real delivery ZIP and uninstall script in isolated directories."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
P=Path(__file__).resolve().parents[1]
R=P/'research/Horizon-complete-delivery-safety-regression-20261022'
D=P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def run(cmd,env,path):
    r=subprocess.run(cmd,env=env,cwd=P,capture_output=True,timeout=120)
    path.write_bytes(r.stdout+r.stderr);return r.returncode
def main():
    assert not R.exists();R.mkdir()
    wrapper=P/'research/horizon-complete-extract-fixture-20261021-r2/invoke-fixture.ps1'
    archive=D/(D.name+'.zip')
    command_text=(D/'Install-Patch.cmd').read_text(encoding='utf-8')
    source=command_text.rsplit('# HORIZON_POWERSHELL_PAYLOAD',1)[1].lstrip()
    assert source==(P/'tools/Install-HorizonPatch.ps1').read_text(encoding='utf-8-sig')
    assert not (D/'StormslingerInstaller').exists()
    script=R/'install.ps1';script.write_text('\ufeff'+source.replace('[Windows.Forms.MessageBox]','[HorizonInstallerFixtureDialog]'),encoding='utf-8')
    uninstall_wrapper=R/'uninstall.ps1'
    uninstall_wrapper.write_text('\ufeffparam([string]$Script,[string]$GameRoot,[string]$BackupRoot,[switch]$ViaCommand)\nfunction Get-CimInstance {}\n& $Script -GameRoot $GameRoot -BackupRoot $BackupRoot -NonInteractive -ViaCommand:$ViaCommand\nexit $LASTEXITCODE\n',encoding='utf-8')
    cases=[]
    for name in ['no-originals','restore-originals','preserve-modified']:
        case=R/name;game=case/'中文 Game';game.mkdir(parents=True)
        shutil.copy2(P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch/../Monster Hunter World/steam_api64_o.dll',case/'unrelated-dll-sample.bin')
        shutil.copy2('H:/Steam/steamapps/common/Monster Hunter World/MonsterHunterWorld.exe',game/'MonsterHunterWorld.exe')
        (game/'nativePC').mkdir();(game/'savedata_backup').mkdir()
        (game/'nativePC/external.bin').write_bytes(b'external mod');(game/'savedata_backup/latest.bin').write_bytes(b'latest save')
        (game/'DLSS').mkdir()
        if name=='preserve-modified':(game/'DLSS/external.bin').write_bytes(b'keep DLSS')
        originals={}
        if name!='no-originals':
            originals={'dinput8.dll':b'original loader','nativePC/common/guildcard/gc_data_pose.gcod':b'original table',
                'Export-Feedback.cmd':b'original export','Uninstall-Patch.cmd':b'original command',
                'StormslingerSupport/Export-Feedback.ps1':b'original export script',
                'StormslingerSupport/Uninstall-Patch.ps1':b'original uninstall script',
                'StormslingerSupport/release-manifest.json':b'original manifest'}
            for rel,data in originals.items():
                p=game/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        backup=case/'player-backup';shutil.copytree(game,backup)
        profile=case/'profile';profile.mkdir()
        (profile/'Stormslinger').mkdir()
        ack=profile/'Stormslinger/horizon-content.version';ack.write_text('horizon-complete-20261022-c10')
        retained_log=profile/'Stormslinger/runtime.jsonl';retained_log.write_bytes(b'keep log')
        env=dict(os.environ,HZ_FIXTURE_MESSAGES=str(case/'messages.txt'),HZ_FIXTURE_ANSWER='Yes',HZ_FIXTURE_RUNNING='no',LOCALAPPDATA=str(profile))
        command=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(wrapper),'-FixtureScript',str(script),'-ArchivePath',str(archive),'-GameRoot',str(game)]
        # The actual No branch must leave originals and inventory unchanged.
        env['HZ_FIXTURE_ANSWER']='No'
        before={p.relative_to(game).as_posix():sha(p) for p in game.rglob('*') if p.is_file()}
        assert run(command,env,case/'install-no.txt')==2
        assert ack.exists() and (game/'DLSS').is_dir()
        assert before=={p.relative_to(game).as_posix():sha(p) for p in game.rglob('*') if p.is_file()}
        env['HZ_FIXTURE_ANSWER']='Yes';assert run(command,env,case/'install-yes.txt')==0
        assert not ack.exists() and retained_log.read_bytes()==b'keep log'
        assert (game/'DLSS').exists()==(name=='preserve-modified')
        m=json.loads((game/'StormslingerSupport/release-manifest.json').read_text(encoding='utf-8-sig'))
        assert all(sha(game/r['relative_path'])==r['sha256'] for r in m['files']+m['auxiliary_files'])
        changed='nativePC/common/guildcard/gc_data_pose.gcod'
        if name=='preserve-modified':(game/changed).write_bytes(b'user modified')
        command=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(uninstall_wrapper),'-Script',str(game/'StormslingerSupport/Uninstall-Patch.ps1'),'-GameRoot',str(game)]
        if originals:command+=['-BackupRoot',str(backup),'-ViaCommand']
        code=run(command,env,case/'uninstall.txt')
        assert code==({'no-originals':0,'restore-originals':3,'preserve-modified':2}[name]),(name,code)
        assert (game/'nativePC/external.bin').read_bytes()==b'external mod'
        assert (game/'savedata_backup/latest.bin').read_bytes()==b'latest save'
        assert retained_log.read_bytes()==b'keep log' and not ack.exists()
        if name=='restore-originals':
            assert all((game/rel).read_bytes()==b for rel,b in originals.items())
            assert {p.relative_to(game).as_posix():sha(p) for p in game.rglob('*') if p.is_file()}==before
        elif name=='no-originals':
            assert {p.relative_to(game).as_posix():sha(p) for p in game.rglob('*') if p.is_file()}==before
        else:
            assert (game/changed).read_bytes()==b'user modified'
            assert json.loads((game/'StormslingerSupport/release-manifest.json').read_text())['variant']=='standalone'
            assert (game/'dinput8.dll').read_bytes()==originals['dinput8.dll']
        cases.append(dict(name=name,passed=True,exit_code=code))
    report=dict(status='passed',cases=cases,archive_sha256=sha(archive),
        limits=['Actual ZIP and scripts executed; UI answers and process inventory are fixtures.',
                'Original CMD restoration checks exit3 preservation contract, not an actual double-click.',
                'No real game installation or another-PC acceptance.'])
    (R/'isolated-delivery-evidence.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report))
if __name__=='__main__':main()
