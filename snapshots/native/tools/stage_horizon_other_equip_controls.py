"""Stage source equipment controls with certified PC reader header versions."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC'
# extension, instance-vtable, reader, four-byte type, version, payload reader
TYPES = [('evam', 0x31DD1D0, 0xF65090, b'EVAM', 3, 0xB1A680),
         ('evbd', 0x31DD0C8, 0xF65190, b'EVBD', 2, 0x2DEB10),
         ('evhl', 0x31DCFC0, 0xF65290, b'EVHL', 37, 0xB1A730),
         ('evwp', 0x31DCEB8, 0xF65460, b'EVWP', 13, 0xB1AA90)]


def main():
    image = CodeImage()
    readers, files = [], []
    for extension, vtable, address, tag, version, payload in TYPES:
        assert image.pointer(vtable+0x50) == address
        getter = image.pointer(vtable+0x30)
        getter_code = image.instructions(getter)
        assert len(getter_code) == 2 and getter_code[-1].mnemonic == 'ret'
        name = image.target(getter_code[0])
        assert image.exe[image.raw(name):image.raw(name)+len(extension)+1] == extension.encode('ascii')+b'\0'
        code = image.instructions(address)
        assert code
        checks = [i for i in code if i.mnemonic == 'cmp']
        assert any(i.op_str == 'eax, 0x18091001' for i in checks)
        assert any(i.op_str == f'ebx, {hex(int.from_bytes(tag, "little"))}' for i in checks)
        assert any(i.op_str == f'eax, {version if version < 10 else hex(version)}' for i in checks)
        assert any(i.mnemonic == 'call' and i.op_str == hex(payload) for i in code)
        helper = image.instructions(payload)
        readers.append({'extension': extension, 'vtable': hex(vtable), 'reader': hex(address),
                        'payload_reader': hex(payload), 'payload_direct_extent_decoded': bool(helper),
                        'version': version, 'header_instructions': [image.format(i) for i in code],
                        'payload_instructions': [image.format(i) for i in helper]})
        for source in sorted(SOURCE.rglob('*.'+extension)):
            data = source.read_bytes()
            assert len(data) >= 12
            assert struct.unpack_from('<I', data)[0] == 0x18091001
            assert data[4:8] == tag and struct.unpack_from('<I', data, 8)[0] == version
            relative = source.relative_to(SOURCE)
            target = OUTPUT/relative
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                assert target.read_bytes() == data
            else:
                target.write_bytes(data)
            files.append({'path': relative.as_posix(), 'size': len(data),
                          'sha256': hashlib.sha256(data).hexdigest(), 'type': extension,
                          'version': version, 'operation': 'preserve original serialization'})
    report = {'status': 'Source control candidates; PC reader headers matched, complete payload semantics pending',
              'exe_sha256': image.meta['exe_sha256'], 'readers': readers, 'files': files,
              'limits': ['PC supports the same four header tags and versions; no complete payload-consumer equivalence claim.',
                         'No native reader, game code, simulation or dynamic validation executed.']}
    (P/'research/horizon-other-equip-control-readers.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/equip-controls.json').write_text(json.dumps({
        'schema_version': 1, 'module': 'horizon-other', 'files': files,
        'state': 'header-version certified candidates; payload and complete references pending'}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'control_files': len(files), 'types': [row['extension'] for row in readers],
                      'payload_extent_decoded': {row['extension']: row['payload_direct_extent_decoded'] for row in readers}}))


if __name__ == '__main__':
    main()
