"""Enumerate bounded MRL texture references for the scoped equipment models."""
from pathlib import Path
import hashlib
import json
import struct
from audit_stormbolt_effect_material_texture_table import parse

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
MODULE = P/'staging/Horizon-other-module/resources/nativePC'


def main():
    files, references, errors = [], {}, []
    for source in sorted(SOURCE.rglob('*.mrl3')):
        raw = source.read_bytes()
        try:
            parsed = parse(raw)
            for row in parsed['textures']:
                offset = int(row['record_offset'], 16)
                assert struct.unpack_from('<I', raw, offset)[0] == 0x241F5DEB
                path = row['path']
                assert not Path(path).is_absolute() and '..' not in Path(path).parts
                evidence = references.setdefault(path, {'path': path, 'materials': [],
                    'module_texture_exists': (MODULE/path).is_file(),
                    'pc_texture_exists': (P/'MHWI解包/chunk'/path).is_file(),
                    'ps_reference_exists': (SOURCE/path).is_file()})
                evidence['materials'].append(source.relative_to(SOURCE).as_posix())
            files.append({'path': source.relative_to(SOURCE).as_posix(),
                          'sha256': hashlib.sha256(raw).hexdigest(), **parsed})
        except (AssertionError, ValueError, struct.error) as error:
            errors.append({'path': str(source.relative_to(SOURCE)), 'error': type(error).__name__+': '+str(error)})
    unresolved = [row for row in references.values() if not row['module_texture_exists'] and not row['pc_texture_exists']]
    report = {'status': 'Static material texture-table references only', 'materials': files,
              'references': list(references.values()), 'unresolved': unresolved, 'errors': errors,
              'limits': ['Resolved path does not prove texture semantic equality or shader compatibility.',
                         'Only texture paths are resolved here; material/shader parameters, skeletons, meshes, audio and effects remain separate.',
                         'No rendering or other dynamic test performed.']}
    (P/'research/horizon-other-material-dependencies.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'materials_parsed': len(files), 'unique_texture_references': len(references),
                      'unresolved': [r['path'] for r in unresolved], 'parse_errors': errors}, ensure_ascii=False))


if __name__ == '__main__':
    main()
