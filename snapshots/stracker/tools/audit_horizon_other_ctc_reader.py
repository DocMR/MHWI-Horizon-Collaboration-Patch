"""Certify the native CTC extended-tail layout from saved PC instructions."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    registration = image.instructions(0x212A71)
    constructor = image.instructions(0x2382010)
    reader = image.instructions(0x2382330)
    assert registration and constructor and reader
    assert {0x3519E90, 0x51C6B80, 0x3519E70} <= {image.target(i) for i in registration}
    assert image.pointer(0x3519F20+0x50) == 0x2382330
    assert image.pointer(0x3519F20+0x30) == 0x2382320
    assert image.target(image.instructions(0x2382320)[0]) == 0x3519FA0
    assert image.exe[image.raw(0x3519FA0):image.raw(0x3519FA0)+4] == b'ctc\0'
    assert 0x3519F20 in {image.target(i) for i in constructor}
    by_address = {i.address: i for i in reader}
    pins = {0x2382378: 'cmp dword ptr [rbx], 0x435443',
            0x2382384: 'cmp dword ptr [rdi + 0xb4], 0x1c',
            0x23823BF: 'mov r8d, 0x40',
            0x23823C5: 'lea rdx, [rdi + 0xb8]',
            0x23823D6: 'mov ebx, dword ptr [rdi + 0xb8]',
            0x23823EC: 'lea ecx, [rbx + rbx*4]',
            0x23823F5: 'shl ecx, 4',
            0x238244A: 'mov ebx, dword ptr [rdi + 0xbc]',
            0x2382460: 'imul ecx, ebx, 0x70',
            0x2382493: 'cmp byte ptr [rdi + 0xf6], 0',
            0x238249C: 'mov ebx, dword ptr [rdi + 0xb8]',
            0x23824A9: 'shl ebx, 2',
            0x23824E7: 'mov dword ptr [rax + rdx*4], 0x3f800000',
            0x238250A: 'call 0x21757e0'}
    for address, expected in pins.items():
        ins = by_address[address]
        assert ins.mnemonic+' '+ins.op_str == expected, hex(address)
    rows = []
    for source in sorted((P/'research/horizon-other-equipment-source').rglob('*.ctc')):
        raw = source.read_bytes()
        chains, nodes = struct.unpack_from('<II', raw, 16)
        extended = bool(raw[0x4E])
        ordinary_end = 80+chains*80+nodes*112
        expected = ordinary_end+(chains*4 if extended else 0)
        assert expected == len(raw)
        rows.append({'path': source.relative_to(P/'research/horizon-other-equipment-source').as_posix(),
                     'sha256': hashlib.sha256(raw).hexdigest(), 'chains': chains, 'nodes': nodes,
                     'extension_flag': raw[0x4E], 'ordinary_end': ordinary_end, 'expected_bytes': expected,
                     'extension_values': list(struct.unpack_from('<'+str(chains)+'f', raw, ordinary_end)) if extended else []})
    report = {'exe_sha256': image.meta['exe_sha256'], 'resource_type': 'rCnsTinyChain',
              'registration': '0x212a60', 'constructor': '0x2382010',
              'instance_vtable': '0x3519f20', 'reader': '0x2382330',
              'file_extension_flag_offset': '0x4e', 'object_extension_flag_offset': '0xf6',
              'header_mapping': 'file+16..79 -> object+B8..F7; 80-byte header total',
              'extension_rule': 'When file+4E is nonzero, read chains*4 additional bytes into object+108 after ordinary nodes.',
              'files': rows, 'reader_instructions': [image.format(i) for i in reader],
              'limits': ['Static instruction/data audit only; no reader execution or physics simulation.',
                         'The five extra floats are preserved; their exact downstream physics meaning is not guessed.']}
    (P/'research/horizon-other-ctc-native-reader.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'ctc_files': len(rows), 'extended_files': [r for r in rows if r['extension_flag']],
                      'native_reader_pins': len(pins)}, ensure_ascii=False))


if __name__ == '__main__':
    main()
