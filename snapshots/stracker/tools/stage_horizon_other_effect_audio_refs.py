"""Stage parsed EPV51 and WWBK4 references; inventory unresolved payloads."""
from pathlib import Path
import hashlib
import json
import re
import struct
from parse_epv3_native import parse_bytes
from audit_horizon_other_body068_event_chain import cstring, parse_events

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
MODULE = P/'staging/Horizon-other-module'
BASE = P/'MHWI解包/chunk'


def main():
    evidence = json.loads((P/'research/pc-epv-reader-header.json').read_text(encoding='utf-8'))
    assert evidence['serialized_header'] == 'uint32 epv magic, uint32 version51, uint32 group count'
    files, dependencies = [], []
    supplemental = P/'research/horizon-other-effect-audio-source'
    sources = list(SOURCE.rglob('*'))
    sources += list((supplemental/'sound/npc').glob('npc_ot*_snd_bk.wwbk'))
    sources += [p for p in supplemental.rglob('*') if p.is_file() and
                p.suffix in ('.wwctl', '.wwct', '.wwev', '.wwpk') and
                re.search(r'(?:ot036|ot081|otm036|otm081)', p.name)]
    sources += list((supplemental/'sound/dialogue/npc_talk').glob('npc_otm*_talk.wwbk'))
    for source in sorted(sources):
        if source.suffix not in ('.epv3', '.wwbk', '.wwctl', '.wwct', '.wwev', '.wwpk'):
            continue
        data = source.read_bytes()
        relative = source.relative_to(SOURCE if source.is_relative_to(SOURCE) else supplemental).as_posix()
        if source.suffix == '.epv3':
            parsed = parse_bytes(data, relative)
            assert parsed['complete_byte_consumption']
            refs = sorted({path for group in parsed['groups'] for row in group['records']
                           for path in row['paths'] if path})
            refs += [parsed['fallback']] if parsed['fallback'] else []
            for path in refs:
                target = path.replace('\\', '/')+'.efx'
                dependencies.append({'owner': relative, 'target': target,
                                     'pc_present': (BASE/target).is_file(), 'kind': 'effect'})
        elif source.suffix in ('.wwbk', '.wwpk'):
            package = source.suffix == '.wwpk'
            assert len(data) >= 16 and data[:4] == (b'WWPK' if package else b'WWBK')
            version, count, reserved = struct.unpack_from('<3I', data, 4)
            assert version == (3 if package else 4) and reserved == 0
            # Names have NUL separators. Preserve serialization; do not infer
            # stream payload compatibility from this bank-name reference list.
            names = re.findall(rb'[^\x00]+', data[16:])
            assert len(names) == count
            for name in names:
                text = name.decode('ascii')
                assert re.fullmatch(r'[A-Za-z0-9_]+', text)
                target = 'sound/wwise/Windows/'+text+('.npck' if package else '.nbnk')
                dependencies.append({'owner': relative, 'target': target,
                                     'pc_present': (BASE/target).is_file(),
                                     'kind': 'audio_package' if package else 'audio_bank'})
        else:
            edges = []
            if source.suffix == '.wwctl':
                magic, version, count, reserved = struct.unpack_from('<4I', data)
                assert (magic, version, reserved) == (0x4c544357, 1, 0)
                cursor = 16
                for _ in range(count):
                    name, cursor = cstring(data, cursor)
                    assert name
                    edges.append((name+'.wwct', 'sound_container'))
                assert cursor == len(data)
            elif source.suffix == '.wwct':
                header = struct.unpack_from('<10I', data)
                # Scoped NPC containers: event and switch arrays only. Do not
                # infer other WWCT arrays from printable path scanning.
                assert header[:4] == (0x54435757, 6, 0, 0) and header[6:] == (0, 0, 0, 0)
                cursor = 40
                for count, extension, kind in ((header[4], '.wwev', 'sound_event'),
                                                (header[5], '.wwsw', 'sound_switch')):
                    for _ in range(count):
                        assert cursor+4 <= len(data)
                        _, = struct.unpack_from('<I', data, cursor)
                        name, cursor = cstring(data, cursor+4)
                        assert name
                        edges.append((name+extension, kind))
                assert cursor == len(data)
            else:
                paths, records = parse_events(data)
                assert paths[2:] == ['', ''] and not any(r['child'] for r in records)
                edges += [(name+extension, kind) for name, extension, kind in
                          zip(paths[:2], ('.wwbk', '.wwpk'), ('bank_reference', 'package_reference')) if name]
            for target, kind in edges:
                assert not Path(target).is_absolute() and '..' not in Path(target).parts
                dependencies.append({'owner': relative, 'target': target,
                                     'pc_present': (BASE/target).is_file(), 'kind': kind})
        output = MODULE/'resources/nativePC'/relative
        output.parent.mkdir(parents=True, exist_ok=True)
        if output.exists():
            assert output.read_bytes() == data
        else:
            output.write_bytes(data)
        files.append({'path': relative, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                      'operation': 'preserve parsed reference serialization'})
    report = {'schema_version': 1, 'module': 'horizon-other', 'files': files,
              'dependencies': dependencies,
              'limits': ['Reference resources only; missing payloads are not silently replaced.',
                         'EPV v51 parsed to exact end using certified native reader structure.',
                         'WWBK v4 names inventoried; complete native reader semantics remain to certify.',
                         'Effect/audio payload conversion and actual interaction remain incomplete.',
                         'No game or target code execution.']}
    for output in (P/'research/horizon-other-effect-audio-references.json', MODULE/'merge/effect-audio-references.json'):
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'staged_references': len(files), 'dependencies': len(dependencies),
                      'pc_missing': [r['target'] for r in dependencies if not r['pc_present']]}))


if __name__ == '__main__':
    main()
