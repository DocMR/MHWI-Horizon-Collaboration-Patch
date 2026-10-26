"""Freeze the separate full-content Stracker candidate; never install it."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

P = Path(__file__).resolve().parents[1]
NAME = 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch - Stracker Edition'
OUT = P / 'staging/Horizon-complete-stracker-stage2-20261023'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    assert not OUT.exists()
    base = P / 'staging/Horizon-complete-installer-safety-20261022'
    package = OUT / 'package'
    shutil.copytree(base / 'package', package)
    (package / 'payload/dinput8.dll').unlink()
    plugin = package / 'payload/nativePC/plugins/HorizonCollaboration.dll'
    plugin.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(P / 'runtime/ninja-horizon-complete-stracker-stage2-20261023/HorizonCollaboration.dll', plugin)
    for source, target in [('Uninstall-HorizonStrackerPatch.ps1', 'Uninstall-Patch.ps1'),
                           ('Export-HorizonStrackerFeedback.ps1', 'Export-Feedback.ps1')]:
        shutil.copy2(P / 'tools' / source, package / 'support-root/StormslingerSupport' / target)
    manifest = json.loads((package / 'release-manifest.json').read_text())
    manifest.update(variant='stracker', status='offline_candidate_live_pending',
        delivery_revision='stracker-stage2-20261023', standalone_dll_sha256=None,
        plugin_relative_path='nativePC/plugins/HorizonCollaboration.dll', plugin_sha256=sha(plugin))
    manifest['files'] = [r for r in manifest['files'] if r['relative_path'] != 'dinput8.dll']
    manifest['files'].append(dict(relative_path='nativePC/plugins/HorizonCollaboration.dll', size=plugin.stat().st_size, sha256=sha(plugin)))
    manifest['diagnostics'].update(version='1.1.0-stracker-candidate', build='horizon-complete-stracker-20261023-stage2')
    for row in manifest['auxiliary_files']:
        file = package / 'support-root' / row['relative_path']
        row.update(size=file.stat().st_size, sha256=sha(file))
    raw = (json.dumps(manifest, indent=2) + '\n').encode()
    (package / 'release-manifest.json').write_bytes(raw)
    (package / 'support-root/StormslingerSupport/release-manifest.json').write_bytes(raw)
    public = P / NAME
    public.mkdir(exist_ok=False)
    archive = public / (NAME + '.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for root in [package / 'payload', package / 'support-root']:
            for file in sorted(root.rglob('*')):
                if file.is_file(): z.write(file, file.relative_to(root).as_posix())
    with zipfile.ZipFile(archive) as z:
        assert not set(z.namelist()) & {'dinput8.dll', 'loader.dll', 'loader-config.json', 'amd_ags_x64.dll'}
        assert all(hashlib.sha256(z.read(r['relative_path'])).hexdigest() == r['sha256']
                   for r in manifest['files'] + manifest['auxiliary_files'])
    shutil.copy2(P / 'tools/Install-HorizonStrackerPatch.cmd', public / 'Install-Patch.cmd')
    source = (P / 'Monster Hunter World Iceborne - Horizon Zero Dawn Collaboration Patch/安装说明 - Installation Instructions.txt').read_text(encoding='utf-8-sig')
    source = source.replace('（原生完整版）', '（保留原DLL版候选，尚未实机验收）').replace('(Complete Native Edition)', '(Stracker Edition Candidate — Live Testing Pending)')
    source = source.replace('无需Python、Stracker或额外MOD加载器；不保证与其它MOD兼容。', '无需Python；需要已安装且启用插件加载的受支持Stracker版本。保留原dinput8.dll、loader.dll及其配置；不保证任意其它MOD均兼容。')
    source = source.replace('Python, Stracker and additional mod loaders are not needed. Compatibility with other mods is not guaranteed.', 'Python is not needed. A supported Stracker loader with plugin loading enabled is required. The original dinput8.dll, loader.dll and configuration are retained; compatibility with arbitrary other mods is not guaranteed.')
    source = source.replace('nativePC、dinput8.dll、Export-Feedback.cmd', 'nativePC（含plugins/HorizonCollaboration.dll）、Export-Feedback.cmd')
    source = source.replace('nativePC, dinput8.dll, Export-Feedback.cmd', 'nativePC (including plugins/HorizonCollaboration.dll), Export-Feedback.cmd')
    source = '候选包：尚未完成本版实机验收，请勿作为正式版公开。原生版与本版不可同时启用，请先卸载原生版并恢复原加载器。\nCandidate: not live-accepted. Do not publish as a finished release. Uninstall the native edition and restore the original loader before using this edition.\n\n' + source
    (public / '安装说明 - Installation Instructions.txt').write_text(source, encoding='utf-8-sig', newline='\r\n')
    with zipfile.ZipFile(base / 'source-complete-native.zip') as z:
        entries = {name: z.read(name) for name in z.namelist()}
    for name in list(entries):
        if name not in ['source-manifest.json', 'BUILD.txt'] and (P / name).is_file():
            entries[name] = (P / name).read_bytes()
    for name in ['runtime/src/quest_loader_owner_copy.h', 'runtime/src/quest_loader_owner_copy_selftest.cpp',
                 'tools/build-horizon-complete-stracker-stage2-20261023.cmd',
                 'tools/Install-HorizonStrackerPatch.ps1', 'tools/Install-HorizonStrackerPatch.cmd',
                 'tools/Uninstall-HorizonStrackerPatch.ps1', 'tools/Export-HorizonStrackerFeedback.ps1',
                 'tools/prepare_horizon_stracker_candidate.py']:
        entries[name] = (P / name).read_bytes()
    entries['source-manifest.json'] = json.dumps([dict(path=n, sha256=hashlib.sha256(b).hexdigest())
        for n, b in sorted(entries.items()) if n not in ['source-manifest.json', 'BUILD.txt']], indent=2).encode()
    entries['BUILD.txt'] = b'Full Stracker candidate. Run tools/build-horizon-complete-stracker-stage2-20261023.cmd. Not live accepted.\n'
    with zipfile.ZipFile(OUT / 'source-stracker-stage2.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(entries.items()): z.writestr(name, data)
    report = dict(status='candidate_built_offline_tests_pending', public_directory=str(public),
        archive_sha256=sha(archive), plugin_sha256=sha(plugin), game_modified=False,
        source_sha256=sha(OUT / 'source-stracker-stage2.zip'), protected_original_dlls_not_shipped=True)
    (OUT / 'candidate-index.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report))


if __name__ == '__main__': main()
