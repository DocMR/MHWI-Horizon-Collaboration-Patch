"""Freeze the first-four cumulative test package, without installing it."""
from pathlib import Path
import hashlib
import json
import shutil
from stormslinger_release_manager import verify
from horizon_complete_test_install import support_files

P = Path(__file__).resolve().parents[1]
OLD = P/'staging/Horizon-complete-task-test-c1/package'
BASE = P/'staging/Horizon-complete-mechanism-payload-20261021-c3'
OUT = P/'staging/Horizon-complete-first-four-test-c3/package'
DLL = P/'runtime/ninja-horizon-complete-c1-20261021/dinput8.dll'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if OUT.parent.exists():
        raise ValueError('Preserve existing frozen cumulative test package')
    old, _, _ = verify(OLD)
    assert sha(DLL) == old['standalone_dll_sha256']
    resources = json.loads((P/'research/horizon-complete-mechanism-payload-20261021.json').read_text(encoding='utf-8-sig'))
    for row in resources['files']:
        assert sha(BASE/row['path']) == row['sha256'], row['path']
    assert len(resources['files']) == 629
    OUT.mkdir(parents=True)
    shutil.copytree(BASE/'nativePC', OUT/'payload/nativePC')
    shutil.copy2(DLL, OUT/'payload/dinput8.dll')
    manifest = dict(old)
    manifest.update(status='first_four_cumulative_candidate_uninstalled_unaccepted',
                    baseline='accepted_stormslinger_plus_six_quests_equipment_and_shared_mechanisms',
                    live_acceptance='Pending grouped user test of first four implementation items; not final release',
                    files=[{'relative_path': f.relative_to(OUT/'payload').as_posix(),
                            'size': f.stat().st_size, 'sha256': sha(f)}
                           for f in sorted((OUT/'payload').rglob('*')) if f.is_file()])
    manifest['candidate_resources'] = 'c3'
    manifest['full_project_complete'] = False
    manifest['remaining_project_items'] = ['Grouped live acceptance of first four items',
                                           'Guildcard/tutorial text, presentation and language finishing',
                                           'Final delivery packaging and install/uninstall acceptance']
    (OUT/'release-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    shutil.copytree(OLD/'support-root', OUT/'support-root')
    shutil.copy2(OUT/'release-manifest.json', OUT/'support-root/StormslingerSupport/release-manifest.json')
    verified, _, files = verify(OUT)
    support = support_files(OUT, verified)
    result = {'status': 'first_four_cumulative_test_candidate_frozen_not_installed',
              'package': OUT.relative_to(P).as_posix(), 'payload_files': len(files),
              'resource_files': len(resources['files']), 'support_files': len(support),
              'manifest_sha256': sha(OUT/'release-manifest.json'), 'dll_sha256': sha(DLL),
              'support_scripts_unchanged_from_previous_candidate': True,
              'game_modified': False, 'save_modified': False, 'live_accepted': False}
    (P/'research/horizon-complete-first-four-candidate-20261021.json').write_text(
        json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
