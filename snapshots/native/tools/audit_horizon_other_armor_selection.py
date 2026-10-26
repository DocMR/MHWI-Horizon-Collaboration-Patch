"""Certify menu index decomposition and selected equipment/recipe pointers."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    fragments,body=decode_group(im,0x1DC2570)
    by={i.address:i for i in body}
    checks={0x1DC25BC:'0xb788d0',0x1DC25DB:'r8d, dword ptr [rcx + rdx*8 + 0x3c8]',
        0x1DC25E5:'r8d',0x1DC25EA:'r12, edx',
        0x1DC25ED:'r13, qword ptr [r9 + rax*8 + 0xa3e8]',
        0x1DC25FC:'r10d, 1',0x1DC2602:'rsi, qword ptr [r13 + r12*8 + 0x18]',
        0x1DC2607:'r14, qword ptr [r13 + r12*8 + 0x30]',
        0x1DC260E:'rsi, qword ptr [r13 + 0x18]',0x1DC2612:'r14, qword ptr [r13 + 0x30]',
        0x1DC262B:'0x16b85b0',0x1DC2700:'r12d, 2',0x1DC2765:'0x1dc2706'}
    for address,operand in checks.items():
        assert by[address].op_str==operand
    report=dict(exe_sha256=im.meta['exe_sha256'],fragments=[list(f) for f in fragments],
        sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
        instructions=[im.format(i) for i in body],
        facts=['Native menu lookup B788D0 result is divided by current row column count; quotient chooses shop+A3E8 row, remainder chooses its entry.',
            'Selection mode1 uses row+18+remainder*8 equipment and row+30+remainder*8 recipe; other mode selects row+18 and row+30.',
            'The selected recipe pointer directly reaches16B85B0, preserving native material fields.',
            'Output loop iterates two adjacent equipment pointers. This alone is not five-part full-set crafting evidence.'],
        limits=['Row construction and target Horizon recipe membership remain unproven.',
            'Mode1 meaning is not assigned from integer value alone.',
            'No custom selection/payment/output path added; static inspection only.'])
    (P/'research/horizon-other-armor-selection.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(selection_modes=2,max_output_entries_in_this_loop=2)))


if __name__=='__main__':
    main()
