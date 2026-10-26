"""Isolated Stracker package install/restore; actual loader files never executed."""
from pathlib import Path
import hashlib, json, os, shutil, subprocess, zipfile

P=Path(__file__).resolve().parents[1]
R=P/'research/horizon-stracker-stage2-delivery-tests-20261023-r3'
D=P/'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch - Stracker Edition'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def files(root):return {p.relative_to(root).as_posix():sha(p) for p in root.rglob('*') if p.is_file()}
def run(cmd,env,log):
    r=subprocess.run(cmd,capture_output=True,env=env,timeout=120);log.write_bytes(r.stdout+r.stderr);return r.returncode
def main():
    R.mkdir(exist_ok=False)
    source=(D/'Install-Patch.cmd').read_text(encoding='utf-8').rsplit('# HORIZON_POWERSHELL_PAYLOAD',1)[1].lstrip()
    assert source==(P/'tools/Install-HorizonStrackerPatch.ps1').read_text(encoding='utf-8-sig')
    source=source.replace('[Windows.Forms.MessageBox]','[HorizonInstallerFixtureDialog]')
    source=source.replace('if((FileHash $exe) -ne $manifest.game_exe_sha256)','if($false)')
    installer=R/'install.ps1';installer.write_text(source,encoding='utf-8-sig')
    wrapper=P/'research/horizon-complete-extract-fixture-20261021-r2/invoke-fixture.ps1'
    uw=R/'uninstall.ps1';uw.write_text("param([string]$Script,[string]$GameRoot,[string]$BackupRoot)\nfunction Get-CimInstance {}\n& $Script -GameRoot $GameRoot -BackupRoot $BackupRoot -NonInteractive -ViaCommand\nexit $LASTEXITCODE\n",encoding='utf-8-sig')
    results=[]
    for name in ['normal','restore-originals','wrong-loader','wrong-questloader']:
        case=R/name;game=case/'中文 Game';game.mkdir(parents=True)
        (game/'MonsterHunterWorld.exe').write_bytes(b'fixture EXE, version gate skipped')
        for file in ['dinput8.dll','loader.dll','loader-config.json']:
            shutil.copy2(P/'Monster Hunter World'/file,game/file)
        plugins=game/'nativePC/plugins';plugins.mkdir(parents=True)
        shutil.copy2(P/'Monster Hunter World/nativePC/plugins/QuestLoader.dll',plugins/'QuestLoader.dll')
        (plugins/'unrelated.bin').write_bytes(b'unrelated plugin');(game/'savedata_backup').mkdir()
        (game/'savedata_backup/latest.bin').write_bytes(b'current save')
        if name=='restore-originals':
            target=game/'nativePC/common/guildcard/gc_data_pose.gcod';target.parent.mkdir(parents=True);target.write_bytes(b'original modded table')
            (game/'Uninstall-Patch.cmd').write_bytes(b'original uninstall command')
        if name=='wrong-loader':(game/'dinput8.dll').write_bytes(b'unknown proxy')
        if name=='wrong-questloader':(plugins/'QuestLoader.dll').write_bytes(b'unknown quest plugin')
        before=files(game);backup=case/'player-backup';shutil.copytree(game,backup)
        profile=case/'profile/Stormslinger';profile.mkdir(parents=True);ack=profile/'horizon-content.version';ack.write_bytes(b'confirmed')
        env=dict(os.environ,LOCALAPPDATA=str(profile.parent),HZ_FIXTURE_MESSAGES=str(case/'messages.txt'),HZ_FIXTURE_ANSWER='No',HZ_FIXTURE_RUNNING='no')
        cmd=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(wrapper),'-FixtureScript',str(installer),'-ArchivePath',str(D/(D.name+'.zip')),'-GameRoot',str(game)]
        if name.startswith('wrong-'):
            assert run(cmd,env,case/'reject.txt')==1 and files(game)==before and ack.exists()
            results.append(dict(case=name,rejected_without_writes=True));continue
        assert run(cmd,env,case/'cancel.txt')==2 and files(game)==before and ack.exists()
        env['HZ_FIXTURE_ANSWER']='Yes';assert run(cmd,env,case/'install.txt')==0
        m=json.loads((game/'StormslingerSupport/release-manifest.json').read_text())
        assert m['variant']=='stracker' and all(sha(game/r['relative_path'])==r['sha256'] for r in m['files']+m['auxiliary_files'])
        assert not ack.exists()
        for rel in ['dinput8.dll','loader.dll','loader-config.json','nativePC/plugins/QuestLoader.dll','nativePC/plugins/unrelated.bin','savedata_backup/latest.bin']:
            assert sha(game/rel)==before[rel],rel
        feedback=case/'feedback'
        export=['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(game/'StormslingerSupport/Export-Feedback.ps1'),'-GameRoot',str(game),'-PackageRoot',str(game/'StormslingerSupport'),'-OutputDirectory',str(feedback),'-NonInteractive']
        assert run(export,env,case/'feedback.txt')==0
        with zipfile.ZipFile(next(feedback.glob('*.zip'))) as z:
            version=json.loads(z.read('version.json').decode('utf-8-sig'));assert version['variant']=='stracker'
        code=run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(uw),'-Script',str(game/'StormslingerSupport/Uninstall-Patch.ps1'),'-GameRoot',str(game),'-BackupRoot',str(backup)],env,case/'uninstall.txt')
        assert code==(3 if name=='restore-originals' else 0),(name,code)
        # Mirror the CMD wrapper self-delete after exit 0; exit 3 preserves restored CMD.
        if code==0:(game/'Uninstall-Patch.cmd').unlink()
        assert files(game)==before,name
        results.append(dict(case=name,passed=True,original_loaders_plugins_saves_unchanged=True,feedback_variant='stracker'))
    report=dict(status='passed',cases=results,limits='Fake EXE; build gate skipped. UI/process inventory mocked. Real final candidate ZIP, loader hashes, file writes, feedback and restoration executed; no real game modified or launched.')
    (R/'results.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
if __name__=='__main__':main()
