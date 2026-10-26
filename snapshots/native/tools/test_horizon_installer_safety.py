"""Real ZIP I/O in isolated targets; fake EXE, process inventory and UI only."""
from pathlib import Path
import hashlib
import json
import os
import subprocess

P = Path(__file__).resolve().parents[1]
R = P / 'research/horizon-installer-safety-fixed-20261022'
D = P / 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch'


def inventory(root):
    return {f.relative_to(root).as_posix(): hashlib.sha256(f.read_bytes()).hexdigest()
            for f in root.rglob('*') if f.is_file()}


def run(command, env, output):
    result = subprocess.run(command, capture_output=True, env=env, timeout=120)
    output.write_bytes(result.stdout + result.stderr)
    return result.returncode


def main():
    R.mkdir(exist_ok=False)
    archive = D / (D.name + '.zip')
    source = (P / 'tools/Install-HorizonPatch.ps1').read_text(encoding='utf-8-sig')
    source = source.replace('[Windows.Forms.MessageBox]', '[HorizonInstallerFixtureDialog]')
    # These tests cover filesystem failures, not game build authentication.
    source = source.replace('if((FileHash $exe) -ne $manifest.game_exe_sha256)', 'if($false)')
    script = R / 'install-under-test.ps1'
    script.write_text(source, encoding='utf-8-sig')
    wrapper_source = (P / 'research/horizon-complete-extract-fixture-20261021-r2/invoke-fixture.ps1').read_text(encoding='utf-8-sig')
    wrapper_source = wrapper_source.replace('& $FixtureScript',
        "if($env:HZ_FIXTURE_LOCK){$held=[IO.File]::Open($env:HZ_FIXTURE_LOCK,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::None)}\n& $FixtureScript")
    wrapper = R / 'invoke-install.ps1'
    wrapper.write_text(wrapper_source, encoding='utf-8-sig')
    results = []
    for name in ['readonly', 'target-directory', 'parent-file', 'locked-file']:
        case = R / name
        game = case / 'Game'; game.mkdir(parents=True)
        (game / 'MonsterHunterWorld.exe').write_bytes(b'non-executable fixture')
        (game / 'dinput8.dll').write_bytes(b'original loader')
        blocked = game / 'nativePC/common/guildcard/gc_data_pose.gcod'
        if name == 'parent-file':
            (game / 'nativePC').write_bytes(b'file occupying parent directory')
        else:
            blocked.parent.mkdir(parents=True)
            if name == 'target-directory': blocked.mkdir()
            else: blocked.write_bytes(b'original table')
        profile = case / 'profile/Stormslinger'; profile.mkdir(parents=True)
        ack = profile / 'horizon-content.version'; ack.write_bytes(b'confirmed')
        before = inventory(game)
        if name == 'readonly': blocked.chmod(0o444)
        env = dict(os.environ, LOCALAPPDATA=str(profile.parent),
            HZ_FIXTURE_MESSAGES=str(case / 'messages.txt'), HZ_FIXTURE_ANSWER='Yes',
            HZ_FIXTURE_RUNNING='no', HZ_FIXTURE_LOCK=str(blocked) if name == 'locked-file' else '')
        command = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(wrapper),
            '-FixtureScript', str(script), '-ArchivePath', str(archive), '-GameRoot', str(game)]
        code = run(command, env, case / 'output.txt')
        assert code == 1, (name, code)
        assert inventory(game) == before, name
        assert ack.read_bytes() == b'confirmed', name
        assert not (game / 'StormslingerSupport').exists(), name
        results.append(dict(case=name, passed=True, exit_code=code, no_patch_writes=True))

    # Exercise the original uninstall UI-race scenario against the fixed logic.
    game = R / 'uninstall-race/Game'; (game / 'StormslingerSupport').mkdir(parents=True)
    (game / 'MonsterHunterWorld.exe').write_bytes(b'non-executable fixture')
    payload = b'installed patch'; (game / 'dinput8.dll').write_bytes(payload)
    manifest = dict(variant='standalone', distribution_layout='extract_archive_contents_directly_into_game_root',
        files=[dict(relative_path='dinput8.dll', sha256=hashlib.sha256(payload).hexdigest())], auxiliary_files=[])
    (game / 'StormslingerSupport/release-manifest.json').write_text(json.dumps(manifest))
    uninstall = R / 'uninstall-under-test.ps1'
    uninstall.write_text((P / 'tools/Uninstall-HorizonPatch.ps1').read_text(encoding='utf-8-sig')
        .replace('[Windows.Forms.MessageBox]', '[AuditUninstallDialog]'), encoding='utf-8-sig')
    checks = R / 'process-checks.txt'
    env = dict(os.environ, HZ_AUDIT_RUNNING='no', HZ_AUDIT_CHECKS=str(checks))
    before = inventory(game)
    code = run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        str(P / 'research/global-review-20261022/invoke-uninstall-race.ps1'),
        '-Script', str(uninstall), '-GameRoot', str(game)], env, R / 'uninstall-race-output.txt')
    assert code == 1 and len(checks.read_text().splitlines()) == 2
    assert inventory(game) == before
    results.append(dict(case='game_started_during_uninstall_dialog', passed=True,
        process_checks=2, exit_code=code, no_deletions_or_restores=True))
    report = dict(status='passed', cases=results,
        limits='Real final ZIP and production file operations; EXE validation skipped for fake EXE; UI/process inventory mocked. No real game modified or launched.')
    (R / 'results.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == '__main__':
    main()
