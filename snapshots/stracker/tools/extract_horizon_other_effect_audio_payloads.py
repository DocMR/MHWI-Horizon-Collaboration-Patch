"""Extract only source payloads named by parsed reference dependencies."""
from pathlib import Path
import hashlib
import json
import re

P = Path(__file__).resolve().parents[1]


def main():
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    directory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))
    index = {r['path']: r for r in directory['files']}
    refs = json.loads((P/'research/horizon-other-effect-audio-references.json').read_text(encoding='utf-8'))
    wanted = sorted({r['target'].replace('/Windows/', '/PS4/') for r in refs['dependencies'] if not r['pc_present']})
    # The two collaboration Palico costumes also supply NPC voice/container
    # resources; equipment EPV/WWBK traversal alone does not name this family.
    npc_resources = {name for name in index if
        re.fullmatch(r'npc/npc\d+/sound/npc\d+_ot(?:036|081)\.wwctl', name) or
        re.fullmatch(r'sound/npc/npc_ot(?:036|081)_[^/]+\.(?:wwct|wwbk|wwev)', name) or
        re.fullmatch(r'sound/wwise/PS4/npc_ot(?:036|081)_(?:se|vo)_cat\.nbnk', name)}
    npc_resources |= {name for name in index if re.search(r'otm(?:036|081)', name)}
    wanted = sorted(set(wanted) | npc_resources)
    sound_objects = P/'research/horizon-other-external-sound-object-candidates.json'
    if sound_objects.is_file():
        wanted = sorted(set(wanted) | {r['path'].replace('/Windows/', '/PS4/')
                        for r in json.loads(sound_objects.read_text(encoding='utf-8'))['matches']})
    missing = [name for name in wanted if name not in index]
    rows = []
    for archive_path in sorted({index[name]['archive'] for name in wanted if name in index}):
        archive = ns['Archive'](Path(archive_path))
        try:
            for name in wanted:
                entry = index.get(name)
                if not entry or entry['archive'] != archive_path:
                    continue
                data = archive.read(entry['offset'], entry['size'])
                assert len(data) == entry['size']
                root = P/'research/horizon-other-effect-audio-source'
                target = (root/name).resolve()
                assert target.is_relative_to(root.resolve())
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                rows.append({'path': name, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                             'archive': archive_path, 'header_hex': data[:32].hex()})
        finally:
            archive.f.close()
    report = {'files': rows, 'source_missing': missing,
              'status': 'Extracted PS source payloads only, not PC-converted or staged',
              'dynamic_validation_performed': False}
    (P/'research/horizon-other-effect-audio-payloads.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'extracted': len(rows), 'bytes': sum(r['size'] for r in rows), 'missing': missing}))


if __name__ == '__main__':
    main()
