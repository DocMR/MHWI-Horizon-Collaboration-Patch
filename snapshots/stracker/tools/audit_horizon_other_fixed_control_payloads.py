"""Certify fixed EV payload read widths and compare source file structure."""
from pathlib import Path
import json
import struct
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    stream = {}
    for slot, width in [(0x30, 4), (0x48, 4), (0x60, 12)]:
        address = image.pointer(0x34DF3C8+slot)
        xs = image.instructions(address)
        assert xs
        if width == 4:
            assert any(i.mnemonic in ('add', 'lea') and (
                i.op_str in ('ecx, 4', 'eax, [rdx + 4]')) for i in xs)
        else:
            by_address = {i.address: i for i in xs}
            for at, operands in [(0x2175B95, 'eax, [rcx + 4]'),
                                  (0x2175BA0, 'ecx, 4'), (0x2175BB3, 'eax, [rcx + 4]')]:
                assert by_address[at].op_str == operands
        stream[hex(slot)] = {'address': hex(address), 'width': width,
                             'instructions': [image.format(i) for i in xs]}
    definitions = [('evam', 0xB1A680, 26), ('evbd', 0x2DEB10, 16), ('evwp', 0xB1AA90, 145)]
    helpers = {}
    for extension, address, expected_bytes in definitions:
        xs = image.instructions(address)
        assert xs
        calls = [i.op_str for i in xs if i.mnemonic == 'call']
        if extension == 'evam':
            assert calls.count('qword ptr [rax + 0x30]') == 3
            assert sum(i.mnemonic == 'setne' for i in xs) == 2
        elif extension == 'evbd':
            assert calls == ['qword ptr [rax + 0x48]']
        else:
            assert calls.count('qword ptr [rax + 0x60]') == 6
            assert calls.count('qword ptr [rax + 0x48]') == 3
            assert calls.count('qword ptr [rax + 0x10]') == 4
            assert calls.count('0x19a0e40') == 1
            assert sum(i.mnemonic == 'setne' for i in xs) == 5
        helpers[extension] = {'address': hex(address), 'expected_file_bytes': expected_bytes,
                              'instructions': [image.format(i) for i in xs]}
    nested = image.instructions(0x19A0E40)
    calls = [i.op_str for i in nested if i.mnemonic == 'call']
    assert calls == ['qword ptr [rax + 0x60]', 'qword ptr [rax + 0x60]', 'qword ptr [rax + 0x48]']
    schemas = {'evam': [('dword', 4)]*3+[('bool', 1)]*2,
               'evbd': [('float', 4)],
               'evwp': [('vec3', 12), ('vec3', 12), ('float', 4),
                        ('vec3', 12), ('vec3', 12), ('float', 4),
                        ('vec3', 12), ('vec3', 12), ('float', 4)] +
                       [('dword', 4)]*4 + [('vec3', 12), ('vec3', 12), ('float', 4)] + [('bool', 1)]*5}
    rows = []
    for extension, _, expected in definitions:
        for source in sorted((P/'research/horizon-other-equipment-source').rglob('*.'+extension)):
            data = source.read_bytes()
            offset, fields = 12, []
            for kind, width in schemas[extension]:
                raw = data[offset:offset+width]
                assert len(raw) == width
                value = (struct.unpack('<3f', raw) if kind == 'vec3' else
                         struct.unpack('<f', raw)[0] if kind == 'float' else
                         struct.unpack('<I', raw)[0] if kind == 'dword' else raw[0])
                fields.append({'offset': offset, 'kind': kind, 'bytes': width, 'value': value})
                offset += width
            assert offset == len(data) == expected
            rows.append({'path': source.relative_to(P/'research/horizon-other-equipment-source').as_posix(),
                         'fields': fields, 'complete_byte_consumption': True})
    report = {'exe_sha256': image.meta['exe_sha256'], 'stream_widths': stream,
              'helpers': helpers, 'nested_evwp_reader': [image.format(i) for i in nested], 'files': rows,
              'limits': ['Serialized widths/order and exact file consumption only; field gameplay meanings and downstream use remain to certify.',
                         'EVHL is variable-length and not covered by these fixed schemas.',
                         'Static instructions plus file parsing only; no target instructions or gameplay mechanisms executed.']}
    (P/'research/horizon-other-fixed-control-payloads.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'fixed_payload_files': len(rows), 'exact_file_consumption': True, 'variable_evhl_pending': True}))


if __name__ == '__main__':
    main()
