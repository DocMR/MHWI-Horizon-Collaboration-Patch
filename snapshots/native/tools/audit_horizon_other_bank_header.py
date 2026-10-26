"""Certify the PC bank-header checks relevant to the converted candidates."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from convert_horizon_other_atrac9_media import bank_chunks


def main():
    im = CodeImage()
    fragments, instructions = decode_group(im, 0x28DDFB0)
    by = {x.address: x for x in instructions}
    pins = {0x28DE1B6: 'edi, [rax + 7]', 0x28DE1C2: 'r8d, [rax + 8]',
            0x28DE1DA: 'dword ptr [rbp - 0x51], 0x44484b42',
            0x28DE1E7: 'r8d, [rdi + 0xd]', 0x28DE1EE: 'rdx, [rbp - 0x39]',
            0x28DE236: 'edi, -0x14', 0x28DE2A5: 'dword ptr [rbp - 0x39], 0x78',
            0x28DE2B4: 'word ptr [rbp - 0x2d], 0',
            0x28DE2BC: 'byte ptr [r13 + 0xc0], al'}
    for address, operand in pins.items():
        assert by[address].op_str == operand
    assert by[0x28DE1A9].op_str == 'eax, eax'
    assert by[0x28DE1F2].op_str == '0x28db5f0'
    assert by[0x28DE2A9].op_str == '0x28de2b4'
    # The20-byte header payload starts at rbp-39. The flag check reads
    # only the low WORD of payload+12, not the high WORD set by PS banks.
    assert -0x2d - (-0x39) == 12
    rows = []
    candidates = json.loads((P/'research/horizon-other-pcm-bank-candidates.json').read_text(encoding='utf-8'))
    for row in candidates['banks']:
        data = (P/row['path']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == row['sha256']
        header = bank_chunks(data)[b'BKHD']
        assert len(header) >= 20
        version, identity, language, flags, project = struct.unpack_from('<5I', header)
        assert version == 120 and flags == 0x10000 and project == 1114
        assert flags & 0xffff == 0
        rows.append({'path': row['path'], 'header_bytes': len(header), 'version': version,
                     'bank_id': identity, 'language': language, 'flags': hex(flags),
                     'native_low_word_nonzero': bool(flags & 0xffff), 'project': project})
    window = [im.format(x) for x in instructions if 0x28DE1A9 <= x.address <= 0x28DE2BC]
    report = {'exe_sha256': im.meta['exe_sha256'], 'banks': rows,
              'header_reader': '0x28ddfb0', 'fragments': [list(f) for f in fragments],
              'native_header_consumption_bytes': 20,
              'finding': 'Version120 accepted by the header branch; PS flag10000 has lowWORD0 and does not enable the checked feedback flag.',
              'policy': 'Preserve original BKHD; no unsupported platform-field normalization.',
              'instructions': window,
              'limits': ['This certifies these header checks only, not complete bank loading, PCM parser or playback.',
                         'Optional native header XOR globals and downstream consumers remain separate conditions.',
                         'No target code executed.']}
    (P/'research/horizon-other-native-bank-header.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'candidate_headers': len(rows), 'version': 120, 'checked_flag_low_word': 0}))


if __name__ == '__main__':
    main()
