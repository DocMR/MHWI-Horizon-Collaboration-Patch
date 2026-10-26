"""Certify native armor output slot writes and capture by craft handler."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    methods, indexed={},{}
    for entry in (0x1DC2570,0x16B4420):
        fragments,body=decode_group(im,entry)
        indexed[entry]={i.address:i for i in body}
        methods[hex(entry)]=dict(fragments=[list(f) for f in fragments],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            instructions=[im.format(i) for i in body])
    checks={0x1DC2570:{0x1DC270E:'r8d, word ptr [rax + 0x1a]',
        0x1DC2713:'ecx, ecx',0x1DC2715:'edx, byte ptr [rax + 0xc]',
        0x1DC2719:'0x16b4420',0x1DC2721:'esi, eax',
        0x1DC2748:'qword ptr [rcx + 0xa608]',
        0x1DC2756:'dword ptr [rcx + rax*4 + 0xa5ec], esi'},
        0x16B4420:{0x16B4478:'dword ptr [rcx + rax + 0xb0bb0], -1',
        0x16B44AC:'ebx, dword ptr [rcx + rax + 0xb0ba0]',
        0x16B44CA:'rax, 0xb0b98',0x16B44D5:'dword ptr [rcx + 0x18], r15d',
        0x16B44D9:'dword ptr [rcx + 0x20], r14d',0x16B44DD:'dword ptr [rcx + 0x24], ebp'}}
    for entry,expected in checks.items():
        for address,operand in expected.items():
            assert indexed[entry][address].op_str==operand
    report=dict(exe_sha256=im.meta['exe_sha256'],methods=methods,
        facts=['16B4420 finds free equipment slot through native index list and empty type marker-1.',
            'Record base character+B0B98+slot*98 receives type+18=0, category+20, equipment ID+24.',
            '1DC2570 takes category from equipment+C and ID from equipment+1A, calls output helper and stores returned slot in shop+A5F0/countA608.',
            'Later callback1DC5090 consumes same saved slots for follow-up operation.'],
        limits=['Native slot initialization and failure handling/admission still need full upstream proof.',
            'Target Horizon recipe selection and single-piece/full-set routing remain separate.',
            'No claim about save persistence or completed full craft feature.',
            'No native execution or dynamic test.'])
    (P/'research/horizon-other-armor-output.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(native_output_fields=3,captured_output_slots=True)))


if __name__=='__main__':
    main()
