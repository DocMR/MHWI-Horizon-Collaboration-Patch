"""Stage bounded EFX candidates and resolve their serialized resource paths."""
from pathlib import Path
import hashlib
import json
from parse_efx_asset import parse_bytes
from convert_horizon_other_assets import convert_texture, validate_linear

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-effect-audio-source'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC'
BASE = P/'MHWI解包/chunk'
EXT = {'0241DBCD': '.tex', '2BB523F2': '.tex', '43C3C8DE': '.tex',
       '5CDDE63B': '.tex', '65443A49': '.uvs', '107DA68D': '.mod3'}


def put(relative, data):
    target = (OUTPUT/relative).resolve()
    assert target.is_relative_to(OUTPUT.resolve())
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        assert target.read_bytes() == data
    else:
        target.write_bytes(data)
    return {'path': relative, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    files, effects, refs = [], [], {}
    for source in sorted(SOURCE.rglob('*.efx')):
        raw = source.read_bytes()
        parsed = parse_bytes(raw)
        assert parsed['consumed_bytes'] == len(raw)
        relative = source.relative_to(SOURCE).as_posix()
        effects.append({'path': relative, **parsed})
        files.append(put(relative, raw))
        for node in parsed['parsed_nodes']:
            for param in node['parameters']:
                extra = bytes.fromhex(param.get('extra_hex', ''))
                for string in extra.split(b'\0'):
                    if not string or not all(32 <= x < 127 for x in string):
                        continue
                    assert param['id'] in EXT
                    path = string.decode('ascii').replace('\\', '/')+EXT[param['id']]
                    refs.setdefault(path, []).append({'effect': relative, 'parameter': param['id']})
    directory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))
    index = {r['path']: r for r in directory['files']}
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    dependencies = []
    for path, origins in sorted(refs.items()):
        row = {'path': path, 'origins': origins, 'pc_present': (BASE/path).is_file()}
        if not row['pc_present']:
            entry = index.get(path)
            if not entry:
                row['state'] = 'source archive lookup required'
            else:
                archive = ns['Archive'](Path(entry['archive']))
                try:
                    data = archive.read(entry['offset'], entry['size'])
                finally:
                    archive.f.close()
                assert len(data) == entry['size']
                target = SOURCE/path
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                row['source_sha256'] = hashlib.sha256(data).hexdigest()
                if path.endswith('.tex'):
                    converted = convert_texture(data)
                    validate_linear(converted)
                    files.append(put(path, converted))
                    row['state'] = 'linear texture candidate staged'
                else:
                    row['state'] = 'source extracted; native schema audit pending'
        else:
            row['state'] = 'existing PC dependency'
        dependencies.append(row)
    report = {'schema_version': 1, 'module': 'horizon-other', 'files': files,
              'effects': effects, 'dependencies': dependencies,
              'limits': ['Complete framing parsing is not rendering verification.',
                         'Resource paths cover bounded variable fields; fixed indices and secondary resources remain to audit.',
                         'No game or native code execution.']}
    for target in (P/'research/horizon-other-effects.json', P/'staging/Horizon-other-module/merge/effects.json'):
        target.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'effects': len(effects), 'nodes': sum(r['node_count'] for r in effects),
                      'staged_files': len(files), 'dependencies': dependencies}, ensure_ascii=False))


if __name__ == '__main__':
    main()
