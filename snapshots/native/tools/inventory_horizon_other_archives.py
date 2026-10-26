"""Index PS equipment/text resources for the non-Stormslinger workstream."""
from pathlib import Path
import json
import re

P = Path(__file__).resolve().parents[1]
loader = P / 'tools/extract_ps_stormslinger.py'
namespace = {'__file__': str(loader)}
exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], namespace)


def main():
    selected, archives = {}, []
    dependency_file = P/'research/horizon-other-effect-audio-references.json'
    dependencies = set()
    if dependency_file.is_file():
        for row in json.loads(dependency_file.read_text(encoding='utf-8'))['dependencies']:
            dependencies.add(row['target'].replace('/Windows/', '/PS4/'))
    sound_objects = P/'research/horizon-other-external-sound-object-candidates.json'
    if sound_objects.is_file():
        for row in json.loads(sound_objects.read_text(encoding='utf-8'))['matches']:
            dependencies.add(row['path'].replace('/Windows/', '/PS4/'))
    source = namespace['ROOT'] / 'chunk'
    def order(path):
        match = re.fullmatch(r'chunk(?:(G)(\d+)|(\d+))-ps4\.bin', path.name)
        if not match:
            raise ValueError(f'Unrecognized archive ordering: {path.name}')
        return (1, int(match[2])) if match[1] else (0, int(match[3]))
    for path in sorted(source.glob('chunk*-ps4.bin'), key=order):
        archive = namespace['Archive'](path)
        try:
            entries = archive.entries()
            kept = 0
            for entry in entries:
                name = entry['path']
                if not (name in dependencies or name.startswith(('common/equip/', 'common/text/', 'common/facility/',
                                                               'common/guildcard/', 'common/item/', 'quest/'))
                        or any(word in name.lower() for word in ('horizon', 'hzd', 'aloy',
                               'body068', 'pl068_', 'bow010', 'ot036', 'ot_036', 'ot081', 'ot_081',
                               'ot_we036', 'ot_we081', 'otm036', 'otm081'))):
                    continue
                selected[name] = {**entry, 'archive': str(path)}
                kept += 1
            archives.append({'archive': str(path), 'entries': len(entries), 'selected': kept})
        finally:
            archive.f.close()
        print(path.name, len(entries), kept, flush=True)
    rows = []
    for name, entry in sorted(selected.items()):
        local = P / 'PS雷暴资源/nativePS4' / name
        pc = P / 'MHWI解包/chunk' / name
        rows.append({**entry, 'local_ps_exists': local.is_file(), 'pc_same_path_exists': pc.is_file()})
    out = P / 'research/horizon-other-archive-directory.json'
    out.write_text(json.dumps({'scope': 'Directory only; no asset payload extraction',
                              'precedence': 'Base numeric chunks followed by numeric G chunks; candidate precedence, runtime order unverified',
                              'archives': archives, 'files': rows}, ensure_ascii=False, indent=2), encoding='utf-8')
    print('Effective selected files:', len(rows), flush=True)


if __name__ == '__main__':
    main()
