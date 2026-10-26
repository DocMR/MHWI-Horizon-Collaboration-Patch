"""Separate actual owner+A18 setter receivers from owner-wide field matches."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    rows = []
    for entry, call, receiver, expected in [
        (0xA52E80, 0xA53162, 0xA53158, 'rcx, qword ptr [rdi + 0x20a00]'),
        (0x1BFE9D0, 0x1BFF22C, 0x1BFF222, 'rcx, qword ptr [rsi + 0xa18]'),
        (0x1CBD166, 0x1CBD166, 0x1CBD15C, 'rcx, qword ptr [rdi + 0xa18]'),
        (0x1FF87A0, 0x1FF8ABE, 0x1FF8AB4, 'rcx, qword ptr [r14 + 0xef8]')]:
        fragments, body = decode_group(im, entry)
        by = {i.address: i for i in body}
        assert by[call].op_str == '0x130c860' and by[receiver].op_str == expected
        if call == 0x1BFF22C:
            assert by[0x1BFF207].op_str == 'r8, qword ptr [rsi + 0x5f68]'
            assert by[0x1BFF217].op_str == 'qword ptr [rax + 0x58]'
            assert im.target(by[0x1BFF200]) == 0x51C46F8
        if call == 0x1CBD166:
            assert by[0x1CBD13A].op_str == 'r8, [rdi + 0x87d9]'
            assert im.target(by[0x1CBD147]) == 0x51C46F8
        rows.append(dict(call=hex(call), receiver=expected,
            owner_a18_receiver='+ 0xa18]' in expected,
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            fragments=[list(f) for f in fragments],
            context=[im.format(i) for i in body if call-100 <= i.address <= call+20]))
    report = dict(exe_sha256=im.meta['exe_sha256'], candidates=rows,
        next_inputs=['1BFF22C: path pointer from receiver+5F68; WWCT fallback after another resource type attempt.',
                     '1CBD166: inline path receiver+87D9; concrete receiver type still needs proof.'],
        limits=['Two true A18 receivers remain candidates; neither identifies player common WWCT by itself.',
                'A53162 and1FF8ABE are not A18 receivers despite A18 occurring elsewhere in their owners.',
                'No runtime reads, game execution or game-directory writes.'])
    (P/'research/horizon-other-sound-loader-candidates.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(candidates=4, actual_a18_receivers=2)))


if __name__ == '__main__':
    main()
