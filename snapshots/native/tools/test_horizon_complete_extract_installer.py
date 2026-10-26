"""Run the real extraction flow in isolated directories with UI/process mocks."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import zipfile

P = Path(__file__).resolve().parents[1]
ROOT = P/'research/horizon-complete-extract-fixture-20261021-r2'
SCRIPT = P/'tools/Install-HorizonPatch.ps1'


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def inventory(root):
    return {f.relative_to(root).as_posix(): digest(f.read_bytes())
            for f in root.rglob('*') if f.is_file()}


def main():
    if ROOT.exists():
        raise ValueError('Preserve previous fixture results')
    ROOT.mkdir()
    source = SCRIPT.read_text(encoding='utf-8-sig')
    # Only modal UI is redirected. ZIP parsing/checks, destination validation,
    # backup-confirmation branch and all extraction code run unchanged.
    fixture_script = ROOT/'installer-under-test.ps1'
    fixture_script.write_text('\ufeff'+source.replace('[Windows.Forms.MessageBox]', '[HorizonInstallerFixtureDialog]'), encoding='utf-8')
    wrapper = ROOT/'invoke-fixture.ps1'
    wrapper.write_text('''\ufeffparam([string]$FixtureScript,[string]$ArchivePath,[string]$GameRoot)
Add-Type -AssemblyName System.Windows.Forms
Add-Type -ReferencedAssemblies System.Windows.Forms -TypeDefinition @'
using System;
using System.IO;
using System.Windows.Forms;
public static class HorizonInstallerFixtureDialog {
 public static DialogResult Show(params object[] args) {
  string message=Convert.ToString(args[0]);
  File.AppendAllText(Environment.GetEnvironmentVariable("HZ_FIXTURE_MESSAGES"), message+"\\n---\\n");
  if(message.Contains("Have you backed up"))
   return (DialogResult)Enum.Parse(typeof(DialogResult), Environment.GetEnvironmentVariable("HZ_FIXTURE_ANSWER"));
  return DialogResult.OK;
 }
}
'@
function Get-CimInstance { if($env:HZ_FIXTURE_RUNNING -eq 'yes'){[pscustomobject]@{Name='MonsterHunterWorld.exe'}} }
& $FixtureScript -ArchivePath $ArchivePath -GameRoot $GameRoot
exit $LASTEXITCODE
''', encoding='utf-8')
    results = []
    for name, answer, fault in [('decline','No',None), ('close','Cancel',None),
                                ('accept','Yes',None), ('bad-hash','Yes','hash'),
                                ('incomplete','Yes','missing'), ('wrong-exe','Yes','exe'),
                                ('running','Yes','running')]:
        case = ROOT/name
        game = case/'game 中文 path'
        (game/'nativePC').mkdir(parents=True)
        (game/'savedata_backup').mkdir()
        (game/'MonsterHunterWorld.exe').write_bytes(b'fixture executable; never run')
        (game/'dinput8.dll').write_bytes(b'old original dll')
        (game/'nativePC/external-mod.bin').write_bytes(b'keep external mod')
        (game/'savedata_backup/save.bin').write_bytes(b'latest save')
        baseline = inventory(game)
        files = {'dinput8.dll': b'new patch dll', 'nativePC/fixture.bin': b'patch resource'}
        rows = [{'relative_path': rel, 'size': len(raw), 'sha256': digest(raw)} for rel, raw in files.items()]
        manifest = {'variant': 'standalone', 'distribution_layout': 'extract_archive_contents_directly_into_game_root',
                    'game_exe_sha256': digest((game/'MonsterHunterWorld.exe').read_bytes()),
                    'files': rows, 'auxiliary_files': []}
        if fault == 'exe':
            manifest['game_exe_sha256'] = '0'*64
        archive = case/'patch fixture.zip'
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
            z.writestr('StormslingerSupport/release-manifest.json', json.dumps(manifest))
            for rel, raw in files.items():
                if fault == 'missing' and rel == 'nativePC/fixture.bin':
                    continue
                z.writestr(rel, b'bad content' if fault == 'hash' and rel == 'dinput8.dll' else raw)
        messages = case/'messages.txt'
        env = dict(os.environ, HZ_FIXTURE_MESSAGES=str(messages), HZ_FIXTURE_ANSWER=answer,
                   HZ_FIXTURE_RUNNING='yes' if fault == 'running' else 'no')
        command = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(wrapper),
                   '-FixtureScript', str(fixture_script), '-ArchivePath', str(archive), '-GameRoot', str(game)]
        run = subprocess.run(command, cwd=P, env=env, capture_output=True, timeout=60)
        (case/'process-output.txt').write_bytes(run.stdout+run.stderr)
        assert messages.exists(), (name, run.returncode, run.stderr.decode(errors='replace'))
        text = messages.read_text(encoding='utf-8-sig')
        if name == 'accept':
            assert run.returncode == 0, (name, text)
            for rel, raw in files.items():
                assert (game/rel).read_bytes() == raw
            assert (game/'nativePC/external-mod.bin').read_bytes() == b'keep external mod'
            assert (game/'savedata_backup/save.bin').read_bytes() == b'latest save'
            assert 'Installation complete' in text
        else:
            assert inventory(game) == baseline, (name, text)
            assert run.returncode != 0, (name, text)
        if name in ('decline','close','accept'):
            assert '是否已将游戏根目录' in text and 'Have you backed up' in text
        else:
            assert 'Have you backed up' not in text
        results.append({'case': name, 'exit_code': run.returncode, 'expected_files_verified': True,
                        'no_writes_on_cancel_or_preflight_error': name != 'accept',
                        'dialog_messages': messages.relative_to(P).as_posix()})
    # Run the original SteamGames function with mocked registry entries but
    # real VDF/appmanifest files, including a secondary Unicode library path.
    steam = ROOT/'Steam root'
    library = ROOT/'第二 Steam Library'
    (steam/'steamapps').mkdir(parents=True)
    (library/'steamapps/common/Monster Hunter World').mkdir(parents=True)
    (library/'steamapps/common/Monster Hunter World/MonsterHunterWorld.exe').write_bytes(b'discovery fixture')
    escaped = str(library.resolve()).replace('\\','\\\\')
    (steam/'steamapps/libraryfolders.vdf').write_text('"libraryfolders" { "1" { "path" "'+escaped+'" } }', encoding='utf-8')
    (library/'steamapps/appmanifest_582010.acf').write_text('"AppState" { "appid" "582010" "installdir" "Monster Hunter World" }', encoding='utf-8')
    function = source[source.index('function SteamGames {'):source.index('function RequireClosed {')]
    discovery = ROOT/'test-discovery.ps1'
    discovery.write_text('''\ufeff$ErrorActionPreference='Stop'
function Test-Path { param([string]$LiteralPath,[string]$PathType)
 if($LiteralPath.StartsWith('HK')) {return $LiteralPath -eq 'HKCU:\\Software\\Valve\\Steam'}
 if($PathType){return Microsoft.PowerShell.Management\\Test-Path -LiteralPath $LiteralPath -PathType $PathType}
 return Microsoft.PowerShell.Management\\Test-Path -LiteralPath $LiteralPath
}
function Get-ItemProperty { [pscustomobject]@{SteamPath=$env:HZ_FIXTURE_STEAM} }
${env:ProgramFiles(x86)}=$null
'''+function+'\n@(SteamGames)|ConvertTo-Json -Compress\n', encoding='utf-8')
    run = subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(discovery)],
                         cwd=P, env=dict(os.environ, HZ_FIXTURE_STEAM=str(steam.resolve())), capture_output=True, timeout=60)
    assert run.returncode == 0, run.stderr.decode(errors='replace')
    # Windows PowerShell console encoding can lose Unicode; write results as
    # UTF8 instead of interpreting the console codepage as authoritative paths.
    output = ROOT/'discovered.json'
    with discovery.open('a',encoding='utf-8') as f:
        f.write("@(SteamGames)|ConvertTo-Json -Compress|Set-Content -LiteralPath $env:HZ_FIXTURE_DISCOVERY -Encoding UTF8\n")
    run = subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',str(discovery)],
                         cwd=P, env=dict(os.environ, HZ_FIXTURE_STEAM=str(steam.resolve()), HZ_FIXTURE_DISCOVERY=str(output)),
                         capture_output=True, timeout=60)
    assert run.returncode == 0, run.stderr.decode(errors='replace')
    found = json.loads(output.read_text(encoding='utf-8-sig'))
    if isinstance(found,str):
        found = [found]
    assert found == [str((library/'steamapps/common/Monster Hunter World').resolve())], found
    report = {'status': 'extract_installer_isolated_flow_and_secondary_steam_library_checks_passed',
              'source': SCRIPT.relative_to(P).as_posix(), 'source_sha256': digest(SCRIPT.read_bytes()),
              'cases': results, 'steam_discovery': {'registry': 'mocked', 'vdf_and_appmanifest': 'real fixture files',
                                                   'secondary_unicode_library_found': True},
              'limits': ['MessageBox results and CIM process inventory are mocked; no actual dialog clicks or real game process test',
                         'Small hash-listed ZIP fixture; final full package and real-machine installation still require acceptance'],
              'real_game_modified': False, 'real_saves_modified': False}
    (P/'research/horizon-complete-extract-installer-tests-20261021.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'status':report['status'], 'cases':len(results), 'secondary_unicode_library_found':True}))


if __name__ == '__main__':
    main()
