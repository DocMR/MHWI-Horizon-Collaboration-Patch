"""Certify native price comparison and two-slot pre-confirmation capacity gate."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    methods,bodies={},{}
    for entry in (0x191A9C0,0x134BEE0,0x16B6C60,0x1DC9910):
        fs,code=decode_group(im,entry)
        bodies[entry]={i.address:i for i in code}
        methods[hex(entry)]=dict(fragments=[list(f) for f in fs],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest(),
            instructions=[im.format(i) for i in code])
    checks={0x191A9C0:{0x191A9DD:'0xbcd950',0x191A9E7:'eax, dword ptr [rax + 0x20]',
        0x191AA07:'dword ptr [rax + 0x98], ebx',0x191AA0D:'al'},
        0x134BEE0:{0x134BF3B:'rax, [r14 + 0xb0bb0]',0x134BF42:'dword ptr [rax], -1',
        0x134BF49:'edi, ebp',0x134BF4F:'rax, 0x98'},
        0x1DC9910:{0x1DC9955:'edx, [r15 + 2]',0x1DC9959:'0x134bee0',
            0x1DC9960:'0x1dc9b3c'},
        0x16B6C60:{0x16B6CA6:'dword ptr [rcx + rax + 0xb0bb0], -1'}}
    for entry,expected in checks.items():
        for address,operand in expected.items():
            assert bodies[entry][address].op_str==operand
    report=dict(exe_sha256=im.meta['exe_sha256'],methods=methods,
        facts=['Funds predicate resolves equipment by category/ID, reads native price+20, compares unsigned character+98>=price.',
            'Capacity predicate counts type=-1 records at character+B0BB0 with stride98, preserving original progression-dependent scan bound.',
            'Confirmation producer1DC9910 requests capacity2 and branches out on failure before constructing craft callback346A4E8.',
            '16B6C60 empty-slot lookup tests the same record type marker used by output helper16B4420.'],
        limits=['Target row menu flag aggregation, recipe unlock/progression and save persistence remain separate.',
            'Do not replace native progression-dependent capacity bound with a constant.',
            'No native execution or dynamic testing.'])
    (P/'research/horizon-other-palico-funds-capacity.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(native_methods=4,capacity_before_confirmation=2)))


if __name__=='__main__':
    main()
