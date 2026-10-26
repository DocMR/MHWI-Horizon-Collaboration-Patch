"""Bound MOD3 skeleton/mesh data and stage original model/material candidates."""
from pathlib import Path
import hashlib
import json
import math
import struct
import zlib
from audit_stormbolt_effect_material_texture_table import parse as parse_material
from analyze_hzd_code import CodeImage
from pc_unwind_groups import decode_group

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC'
SCHEMAS = ['https://raw.githubusercontent.com/AsteriskAmpersand/Mod3-MHW-Importer/master/mod3/Mod3Skeleton.py',
           'https://raw.githubusercontent.com/AsteriskAmpersand/Mod3-MHW-Importer/master/mod3/Mod3Mesh.py']


def pc_material_parameters():
    """Reuse the native loader's payload/count and masked descriptor lookup."""
    image = CodeImage()
    _, loader = decode_group(image, 0x2383400)
    by = {row.address: row for row in loader}
    for address, operand in {
        0x23836B2: 'r9d, dword ptr [rsi + rbp + 0x10]',
        0x23836F8: 'r8d, dword ptr [rsi + rbp + 0x14]',
        0x2383700: 'rdx, qword ptr [rsi + rbp + 0x30]',
        0x2383705: 'r8d, 0x11', 0x238370E: '0x235f750',
    }.items():
        assert by[address].op_str == operand
    lookup = {row.address: row for row in image.instructions(0x23D7D90)}
    assert lookup[0x23D7D9A].op_str == 'r10d, 0xfffff000'
    raw = (P/'MHWI解包/chunk/system/shader/ShaderPackage.sdf').read_bytes()
    count = struct.unpack_from('<I', raw, 8)[0]
    table, strings = struct.unpack_from('<Q', raw, 56)[0], struct.unpack_from('<Q', raw, 120)[0]
    assert table+8*count <= len(raw)
    descriptors = {}
    for index in range(count):
        at = struct.unpack_from('<Q', raw, table+8*index)[0]
        assert at+16 <= len(raw)
        name, kind, key = struct.unpack_from('<QII', raw, at)
        start = strings+name
        assert start < len(raw)
        end = raw.index(b'\0', start, min(start+256, len(raw)))
        row = {'index': index, 'name': raw[start:end].decode('ascii'), 'kind': kind & 63}
        if row['kind'] == 3:
            assert at+24 <= len(raw)
            row['constant_bytes'] = struct.unpack_from('<I', raw, at+16)[0]*4
        descriptors.setdefault((key << 12) & 0xFFFFFFFF, []).append(row)
    return descriptors, {'exe_sha256': image.meta['exe_sha256'],
                         'shader_package_sha256': hashlib.sha256(raw).hexdigest(),
                         'descriptor_count': count}


def inspect_material_payload(raw, material, descriptors):
    table = int(material['material_table_offset'], 16)
    assert table+56*material['material_count'] <= len(raw)
    rows = []
    for index in range(material['material_count']):
        at = table+56*index
        size, packed = struct.unpack_from('<II', raw, at+16)
        pointer = struct.unpack_from('<Q', raw, at+48)[0]
        count = packed >> 17
        assert pointer+size <= len(raw) and count*16 <= size
        parameters = []
        for number in range(count):
            kind, key, value = struct.unpack_from('<IIQ', raw, pointer+16*number)
            role = kind & 15
            candidates = descriptors.get(key & 0xFFFFF000, [])
            assert candidates, (index, number, hex(key))
            descriptor = candidates[0]  # Native lookup uses the first masked match.
            assert role in (0, 1, 2) and descriptor['kind'] == {0: 3, 1: 6, 2: 5}[role]
            if role == 2:
                # Zero is an original null binding, not a missing texture.
                assert 0 <= value <= material['texture_count']
            elif role == 1:
                assert value < 12
            elif value:
                assert count*16 <= value and value+descriptor['constant_bytes'] <= size
            parameters.append({'record': number, 'role': role, 'serialized_value': value,
                               'descriptor': descriptor, 'masked_matches': len(candidates)})
        rows.append({'material': index, 'payload_bytes': size, 'parameters': parameters})
    return rows


def inspect_model(raw):
    assert len(raw) >= 320 and struct.unpack_from('<IH', raw) == (0x444F4D, 237)
    bones, meshes, names = struct.unpack_from('<3H', raw, 6)
    vertices, indices = struct.unpack_from('<II', raw, 12)
    skeleton, groups, name_table, mesh_table, vertex_buffer, index_buffer = struct.unpack_from('<6Q', raw, 48)
    assert 320 <= skeleton <= groups <= name_table <= mesh_table <= vertex_buffer <= index_buffer <= len(raw)
    assert name_table+names*128 <= mesh_table and mesh_table+meshes*80 <= vertex_buffer
    assert vertex_buffer+struct.unpack_from('<I', raw, 24)[0] == index_buffer
    assert index_buffer+2*indices <= len(raw)
    if bones:
        matrix_offset = skeleton+bones*24
        map_offset = matrix_offset+bones*128
        assert map_offset+512 <= groups
        for i in range(bones):
            function, parent, child, *values = struct.unpack_from('<hBB5f', raw, skeleton+i*24)
            assert parent == 255 or parent < bones
            assert child == 255 or child < bones
            assert all(math.isfinite(value) for value in values)
        matrices = struct.unpack_from('<'+str(bones*32)+'f', raw, matrix_offset)
        assert all(math.isfinite(value) for value in matrices)
        assert all(value == 255 or value < bones for value in raw[map_offset:map_offset+512])
    material_names = []
    for index in range(names):
        field = raw[name_table+128*index:name_table+128*(index+1)]
        assert b'\0' in field
        material_names.append(field.split(b'\0')[0].decode('ascii'))
    rows = []
    for index in range(meshes):
        at = mesh_table+index*80
        count = struct.unpack_from('<H', raw, at+2)[0]
        material = struct.unpack_from('<H', raw, at+6)[0]
        stride = raw[at+14]
        sub, relative, fmt, face_offset, face_count, base = struct.unpack_from('<6I', raw, at+16)
        start = vertex_buffer+relative+stride*(sub+base)
        end = start+stride*count
        assert material < names and stride > 0 and start <= end <= index_buffer
        assert face_count % 3 == 0 and index_buffer+2*(face_offset+face_count) <= len(raw)
        rows.append({'mesh': index, 'vertices': count, 'stride': stride,
                     'format': hex(fmt), 'material_index': material, 'vertex_start': start,
                     'vertex_end': end, 'face_index_count': face_count})
    return {'bones': bones, 'vertices': vertices, 'indices': indices,
            'material_names': material_names, 'meshes': rows}


def pc_format_witnesses(needed):
    witnesses = {}
    examined = 0
    for source in (P/'MHWI解包/chunk').rglob('*.mod3'):
        with source.open('rb') as stream:
            header = stream.read(320)
            if len(header) < 320 or struct.unpack_from('<IH', header) != (0x444F4D, 237):
                continue
            meshes = struct.unpack_from('<H', header, 8)[0]
            table = struct.unpack_from('<Q', header, 72)[0]
            if table+80*meshes > source.stat().st_size:
                continue
            stream.seek(table)
            data = stream.read(80*meshes)
        examined += 1
        for i in range(meshes):
            fmt = hex(struct.unpack_from('<I', data, 80*i+24)[0])
            pair = (fmt, data[80*i+14])
            if pair in needed and pair not in witnesses:
                witnesses[pair] = {'format': pair[0], 'stride': pair[1],
                                   'pc_path': source.relative_to(P/'MHWI解包/chunk').as_posix(),
                                   'mesh': i, 'sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
        if needed <= witnesses.keys():
            break
    assert needed <= witnesses.keys(), 'Some source format/stride pairs lack a PC sample'
    return list(witnesses.values()), examined


def main():
    # Existing source-backed PS/PC model and material anchors remain byte-identical.
    for name in ('wp/lbg/lbg001/mod/lbg001.mod3', 'wp/lbg/lbg001/mod/lbg001.mrl3'):
        assert (P/'PS雷暴资源/nativePS4'/name).read_bytes() == (P/'MHWI解包/chunk'/name).read_bytes()
    descriptors, parameter_identity = pc_material_parameters()
    models, materials, needed = [], [], set()
    for source in sorted(SOURCE.rglob('*.mod3')):
        raw = source.read_bytes()
        parsed = inspect_model(raw)
        material_source = source.with_suffix('.mrl3')
        material_raw = material_source.read_bytes()
        material = parse_material(material_raw)
        payloads = inspect_material_payload(material_raw, material, descriptors)
        table = int(material['material_table_offset'], 16)
        assert table+56*material['material_count'] <= len(material_raw)
        hashes = [struct.unpack_from('<I', material_raw, table+56*i+4)[0]
                  for i in range(material['material_count'])]
        parsed['material_bindings'] = [
            {'name': name, 'hash': hex(zlib.crc32(name.encode('ascii')) ^ 0xFFFFFFFF),
             'matching_records': [i for i, value in enumerate(hashes)
                                  if value == zlib.crc32(name.encode('ascii')) ^ 0xFFFFFFFF]}
            for name in parsed['material_names']]
        needed.update((row['format'], row['stride']) for row in parsed['meshes'])
        models.append({'path': source.relative_to(SOURCE).as_posix(),
                       'sha256': hashlib.sha256(raw).hexdigest(), 'size': len(raw), **parsed})
        materials.append({'path': material_source.relative_to(SOURCE).as_posix(),
                          'sha256': hashlib.sha256(material_raw).hexdigest(), 'size': len(material_raw),
                          'material_count': material['material_count'], 'texture_count': material['texture_count'],
                          'pc_parameter_bindings': payloads})
    witnesses, examined = pc_format_witnesses(needed)
    for row in models+materials:
        data = (SOURCE/row['path']).read_bytes()
        target = OUTPUT/row['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert target.read_bytes() == data
        else:
            target.write_bytes(data)
    report = {'status': 'Original serialized MOD3/MRL candidates; statically bounded, not dynamically verified',
              'schemas': SCHEMAS, 'models': models, 'materials': materials,
              'pc_format_stride_witnesses': witnesses, 'pc_files_examined': examined,
              'pc_parameter_identity': parameter_identity,
              'limits': ['Matching PC format/stride witnesses and a shared byte-identical reference establish structural support, not complete rendering equivalence.',
                         'Material names are matched by native full32 CRC to MRL record+4, stride56; unmatched names remain explicit fallback candidates, not rendering proof.',
                         'All selected payload keys resolve to PC descriptors with matching roles and bounded nonnull constant data; shader permutation selection and GPU acceptance remain unverified.',
                         'Bone tables, matrices, map indices and mesh buffers are bounded; skinned animation/physics behavior remains unverified.',
                         'No game launch, target code execution or dynamic tests.']}
    (P/'research/horizon-other-model-material-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/model-materials.json').write_text(json.dumps({
        'schema_version': 1, 'module': 'horizon-other', 'operation': 'preserve_original_serialization',
        'files': [{k: row[k] for k in ('path', 'size', 'sha256')} for row in models+materials],
        'pc_parameter_identity': parameter_identity,
        'pc_parameter_bindings': [{'path': row['path'], 'materials': row['pc_parameter_bindings']}
                                  for row in materials],
        'state': 'PC parameter keys/roles/reference bounds statically checked; shader permutation, complete dependency and rendering acceptance pending'}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'models': len(models), 'materials': len(materials),
                      'meshes': sum(len(row['meshes']) for row in models),
                      'format_stride_pairs': sorted(needed), 'pc_samples': witnesses}, ensure_ascii=False))


if __name__ == '__main__':
    main()
