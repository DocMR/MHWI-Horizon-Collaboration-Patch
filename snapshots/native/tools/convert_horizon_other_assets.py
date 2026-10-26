"""Extract scoped PS equipment references and produce isolated linear TEX candidates."""
from pathlib import Path
import collections
import hashlib
import json
import re
import struct

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC'
FORMATS = {7: (1, 4), 22: (4, 8), 23: (4, 8), 26: (4, 16), 30: (4, 16), 31: (4, 16)}


def validate_linear(data):
    mip_count, width, height = struct.unpack_from('<3I', data, 20)
    dimension, depth = struct.unpack_from('<I', data, 16)[0], struct.unpack_from('<I', data, 32)[0]
    assert dimension in (2, 5)
    layers = depth if dimension == 5 else 1
    assert layers > 0 and mip_count > 0
    fmt = struct.unpack_from('<I', data, 36)[0]
    unit, size = FORMATS[fmt]
    offsets = struct.unpack_from('<'+str(mip_count*layers)+'Q', data, 184)
    assert offsets[0] == 184+8*mip_count*layers
    for index, offset in enumerate(offsets):
        level = index % mip_count
        end = offsets[index+1] if index+1 < len(offsets) else len(data)
        expected = max(1, (max(1, width >> level)+unit-1)//unit)*max(1, (max(1, height >> level)+unit-1)//unit)*size
        assert end-offset == expected
    assert struct.unpack_from('<I', data, 56)[0] == 0xFFFFFFFF


def convert_texture(data):
    assert data[:4] == b'TEX\0'
    if struct.unpack_from('<I', data, 56)[0] == 0xFFFFFFFF:
        validate_linear(data)
        return data
    assert struct.unpack_from('<I', data, 16)[0] == 2
    mip_count, width, height = struct.unpack_from('<3I', data, 20)
    fmt = struct.unpack_from('<I', data, 36)[0]
    unit, block_size = FORMATS[fmt]
    offsets = list(struct.unpack_from('<'+str(mip_count)+'Q', data, 184))
    assert mip_count > 0 and offsets[0] == 184+8*mip_count
    output = bytearray(data[:offsets[0]])
    struct.pack_into('<I', output, 56, 0xFFFFFFFF)
    for level, offset in enumerate(offsets):
        block_width = max(1, (max(1, width >> level)+unit-1)//unit)
        block_height = max(1, (max(1, height >> level)+unit-1)//unit)
        tile_width = (block_width+7)//8
        end = offsets[level+1] if level+1 < mip_count else len(data)
        assert end-offset == tile_width*((block_height+7)//8)*64*block_size
        struct.pack_into('<Q', output, 184+8*level, len(output))
        for y in range(block_height):
            for x in range(block_width):
                morton = sum((((x >> b) & 1) << (2*b)) | (((y >> b) & 1) << (2*b+1)) for b in range(3))
                index = ((y//8)*tile_width+x//8)*64+morton
                output.extend(data[offset+index*block_size:offset+(index+1)*block_size])
    return bytes(output)


def main():
    # Re-establish byte-level anchors for this isolated converter, without running game code.
    anchors = []
    for name in ('Assets/default_tex/noise_LIN.tex', 'Assets/default_tex/snow_Col_CMM.tex'):
        source = (P/'PS雷暴资源/nativePS4'/name).read_bytes()
        base = (P/'MHWI解包/chunk'/name).read_bytes()
        result = convert_texture(source)
        source_offset = struct.unpack_from('<Q', result, 184)[0]
        base_offset = struct.unpack_from('<Q', base, 184)[0]
        assert result[source_offset:] == base[base_offset:]
        anchors.append({'path': name, 'linear_payload_equals_pc': True})
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    inventory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))
    selected = [row for row in inventory['files'] if re.match(
        r'(?:pl/[fm]_equip/pl068_\d+/|otomo/equip/ot(?:036|081)/|otomo/wp/ot_we(?:036|081)/|wp/bow/bow010/)', row['path'])]
    receipts = []
    for archive_name in sorted({row['archive'] for row in selected}):
        archive = ns['Archive'](Path(archive_name))
        try:
            for row in [r for r in selected if r['archive'] == archive_name]:
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                target = (SOURCE/row['path']).resolve()
                assert target.is_relative_to(SOURCE.resolve())
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                base = P/'MHWI解包/chunk'/row['path']
                receipt = {**row, 'source_sha256': hashlib.sha256(data).hexdigest(),
                           'pc_same_path_exists': base.is_file(),
                           'pc_byte_identical': base.is_file() and base.read_bytes() == data}
                receipts.append(receipt)
        finally:
            archive.f.close()
        print(Path(archive_name).name, 'references extracted', flush=True)
    converted, errors = [], []
    for row in receipts:
        if not row['path'].endswith('.tex'):
            continue
        data = (SOURCE/row['path']).read_bytes()
        try:
            result = convert_texture(data)
            mip_count, width, height = struct.unpack_from('<3I', result, 20)
            fmt = struct.unpack_from('<I', result, 36)[0]
            validate_linear(result)
            target = OUTPUT/row['path']
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(result)
            base = P/'MHWI解包/chunk'/row['path']
            converted.append({'path': row['path'], 'source_sha256': row['source_sha256'],
                              'output_sha256': hashlib.sha256(result).hexdigest(), 'output_bytes': len(result),
                              'format': fmt, 'mip_count': mip_count,
                              'operation': 'preserved_linear' if result == data else 'morton_to_linear',
                              'pc_file_identical': base.is_file() and base.read_bytes() == result})
        except (AssertionError, KeyError, struct.error) as error:
            errors.append({'path': row['path'], 'error': type(error).__name__+': '+str(error)})
    report = {'status': 'Static texture candidates only; equipment dependencies and runtime mechanisms incomplete',
              'algorithm': '8x8 Morton tiled to linear; remove padded blocks; rebuild mip offsets; PC platform tag FFFFFFFF',
              'anchors': anchors, 'source_files': receipts, 'textures': converted, 'errors': errors,
              'unconverted_other_files': [row['path'] for row in receipts if not row['path'].endswith('.tex')],
              'limits': ['No game execution, injection or dynamic validation.',
                         'Model/material/collision/animation/effect/audio references extracted but not implicitly considered PC-compatible.',
                         'Source platform layout and complete references require separate certification; textures alone do not make equipment usable.']}
    (P/'research/horizon-other-equipment-assets.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/equipment-textures.json').write_text(json.dumps({
        'schema_version': 1, 'module': 'horizon-other', 'files': converted,
        'policy': 'Exclusive resource paths; verify hash for identical overlap. No generic global texture replacement.'}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'references': len(receipts), 'reference_bytes': sum(r['size'] for r in receipts),
                      'texture_outputs': len(converted), 'conversion_errors': len(errors),
                      'operations': dict(collections.Counter(r['operation'] for r in converted)),
                      'formats': dict(collections.Counter(r['format'] for r in converted))}), flush=True)


if __name__ == '__main__':
    main()
