"""Certify EPVSP8 reader and stage bounded serialized sound parameters."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage
from pc_unwind_groups import decode_group

P = Path(__file__).resolve().parents[1]


def parse(data):
    assert struct.unpack_from('<3I', data) == (0x18091001, 0x505345, 8)
    kind = struct.unpack_from('<I', data, 12)[0]
    assert kind == 0x45185BED
    end = data.index(b'\0', 16)
    path = data[16:end].decode('ascii').replace('\\', '/')
    assert 0 < end-16 < 80
    cursor, arrays = end+1, []
    for fmt in ('<3i2I', '<2i4I'):
        count = struct.unpack_from('<I', data, cursor)[0]
        cursor += 4
        width = struct.calcsize(fmt)
        assert count <= (len(data)-cursor)//width
        rows = [list(struct.unpack_from(fmt, data, cursor+i*width)) for i in range(count)]
        cursor += count*width
        arrays.append(rows)
    assert cursor == len(data)
    return {'resource_type_hash': hex(kind), 'container': path+'.wwct',
            'array_a': arrays[0], 'array_b': arrays[1], 'complete_byte_consumption': True}


def main():
    im = CodeImage()
    assert im.pointer(0x33E3BC8+8) == 0x19DCD90
    init = im.instructions(0x19DCD90)
    assert any(im.target(x) == 0x33E3BF8 for x in init)
    assert im.pointer(0x33E3BF8+0x50) == 0x19DD080
    header = {x.address: x for x in im.instructions(0x19DD080)}
    for address, operand in {0x19DD0A3: 'eax, 0x18091001', 0x19DD0B4: 'eax, 0x505345',
                             0x19DD0C5: 'eax, 8', 0x19DD0D6: '0xb0ddb0'}.items():
        assert header[address].op_str == operand
    _, body = decode_group(im, 0xB0DDB0)
    by = {x.address: x for x in body}
    assert by[0xB0DDD7].op_str == 'qword ptr [rax + 0x10]'
    assert by[0xB0DE2B].op_str == '0x2175a10'
    assert by[0xB0DEA6].op_str == by[0xB0E019].op_str == 'qword ptr [rax + 0x10]'
    for address in (0xB0DFA3, 0xB0DFAF, 0xB0DFBB, 0xB0E10A, 0xB0E116):
        assert by[address].op_str == 'qword ptr [rax + 0x30]'
    for address in (0xB0DFC7, 0xB0DFD3, 0xB0E122, 0xB0E12E, 0xB0E13A, 0xB0E146):
        assert by[address].op_str == 'qword ptr [rax + 0x10]'
    files = []
    root = P/'research/horizon-other-equipment-source'
    output = P/'staging/Horizon-other-module/resources/nativePC'
    for source in sorted(root.rglob('*.epvsp')):
        raw = source.read_bytes()
        parsed = parse(raw)
        relative = source.relative_to(root)
        target = output/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert target.read_bytes() == raw
        else:
            target.write_bytes(raw)
        files.append({'path': relative.as_posix(), 'size': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                      **parsed, 'container_pc_present': (P/'MHWI解包/chunk'/parsed['container']).is_file()})
    report = {'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': im.meta['exe_sha256'],
              'header_reader': '0x19dd080', 'payload_reader': '0xb0ddb0', 'files': files,
              'reader_instructions': [im.format(x) for x in body],
              'limits': ['Array scalar layout certified; event meanings and playback still pending.',
                         'Native stream signed/unsigned DWORD reads have equal serialized width.',
                         'No target code executed or dynamic validation.']}
    for destination in (P/'research/horizon-other-sound-params.json',
                        P/'staging/Horizon-other-module/merge/sound-params.json'):
        destination.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'staged': len(files), 'missing_pc_containers': [r['container'] for r in files if not r['container_pc_present']]}))


if __name__ == '__main__':
    main()
