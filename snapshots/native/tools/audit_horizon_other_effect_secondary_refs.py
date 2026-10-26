"""Resolve EFX->UVS->texture dependencies using bounded native UVS fields."""
from pathlib import Path
import hashlib
import json
from parse_uv_sequence import parse_bytes
from convert_horizon_other_assets import validate_linear

P = Path(__file__).resolve().parents[1]
BASE = P/'MHWI解包/chunk'


def main():
    report = json.loads((P/'research/horizon-other-effects.json').read_text(encoding='utf-8'))
    sequences, textures = [], {}
    for dependency in report['dependencies']:
        path = dependency['path']
        if path.endswith('.tex'):
            textures.setdefault(path, []).append({'owner': 'direct EFX field'})
        elif path.endswith('.uvs'):
            source = BASE/path
            data = source.read_bytes()
            parsed = parse_bytes(data)
            sequences.append({'path': path, 'sha256': hashlib.sha256(data).hexdigest(), **parsed})
            for row in parsed['textures']:
                textures.setdefault(row['path'], []).append({'owner': path, 'selector': row['selector']})
    texture_rows = []
    for path, origins in sorted(textures.items()):
        data = (BASE/path).read_bytes()
        validate_linear(data)
        texture_rows.append({'path': path, 'origins': origins, 'bytes': len(data),
                             'sha256': hashlib.sha256(data).hexdigest(),
                             'state': 'existing PC texture; linear mip extents checked'})
    result = {'module': 'horizon-other', 'uv_sequences': sequences, 'textures': texture_rows,
              'missing': [],
              'limits': ['UVS frame/index ranges and texture indices checked with native-access schema.',
                         'Texture existence and mip bounds do not prove visual equivalence or timing.',
                         'EFX fixed indices, triggers, materials and opaque fields remain separate work.',
                         'No target code, game loading or dynamic validation.']}
    for output in (P/'research/horizon-other-effect-secondary-refs.json',
                   P/'staging/Horizon-other-module/merge/effect-secondary-refs.json'):
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'uv_sequences': len(sequences), 'texture_dependencies': len(texture_rows),
                      'missing': 0, 'uv_records': sum(len(r['records']) for r in sequences)}))


if __name__ == '__main__':
    main()
