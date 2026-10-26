"""Compose retained shared audio edits onto the frozen equipment candidate."""
from pathlib import Path
import hashlib
import json
import shutil
from merge_horizon_other_audio_objects import merge

P = Path(__file__).resolve().parents[1]
MODULE = P / 'staging/Horizon-complete-integration-20261021'
BASE = P / 'staging/Horizon-complete-equipment-payload-20261021-c2'
OUT = P / 'staging/Horizon-complete-mechanism-payload-20261021-c3'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if OUT.exists():
        raise ValueError('Preserve frozen mechanism candidate; do not overwrite')
    delta = json.loads((MODULE / 'merge/audio-object-deltas.json').read_text(encoding='utf-8-sig'))
    assert delta['schema_version'] == 1 and delta['module'] == 'horizon-other'
    outputs = {}
    rows = []
    for row in delta['files']:
        relative = row['target']
        current = BASE / 'nativePC' / relative
        source = current if current.exists() else P / 'MHWI解包/chunk' / relative
        before = source.read_bytes()
        after = merge(before, row['changes'])
        assert merge(after, row['changes']) == after
        outputs[relative] = after
        rows.append({'path': 'nativePC/' + relative,
                     'source': source.relative_to(P).as_posix(),
                     'sound_objects': len(row['changes']),
                     'before_sha256': sha(before), 'after_sha256': sha(after),
                     'idempotence_checked': True})
    assert len(rows) == 4 and sum(r['sound_objects'] for r in rows) == 253
    # Validate all edits before creating the frozen output.
    shutil.copytree(BASE, OUT)
    for relative, data in outputs.items():
        target = OUT / 'nativePC' / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    preserved = 0
    for source in (BASE / 'nativePC').rglob('*'):
        if source.is_file():
            relative = source.relative_to(BASE / 'nativePC').as_posix()
            if relative not in outputs:
                assert source.read_bytes() == (OUT / 'nativePC' / relative).read_bytes()
                preserved += 1
    files = [{'path': f.relative_to(OUT).as_posix(), 'bytes': f.stat().st_size,
              'sha256': sha(f.read_bytes())}
             for f in sorted((OUT / 'nativePC').rglob('*')) if f.is_file()]
    report = {'status': 'shared_sound_objects_composed_offline_not_installed',
              'candidate': OUT.relative_to(P).as_posix(), 'resource_files': len(files),
              'banks': rows, 'unrelated_base_files_preserved': preserved,
              'unedited_sound_objects_and_non_hirc_chunks_preserved': True,
              'limits': ['No native playback or event-trigger acceptance',
                         'First live test waits for all first four implementation items'],
              'files': files}
    (P / 'research/horizon-complete-mechanism-payload-20261021.json').write_text(
        json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: report[k] for k in ['status', 'resource_files', 'unrelated_base_files_preserved']}))


if __name__ == '__main__':
    main()
