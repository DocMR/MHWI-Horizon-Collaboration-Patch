"""Certify native craft material sufficiency and combined inventory query."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    methods={}
    bodies={}
    for entry in (0x16B7D90,0x1896DF0,0x1DD25B0):
        fs,code=decode_group(im,entry)
        bodies[entry]={i.address:i for i in code}
        methods[hex(entry)]=dict(fragments=[list(f) for f in fs],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest(),
            instructions=[im.format(i) for i in code])
    checks={0x16B7D90:{0x16B7DBC:'rsi, [rcx + 0x20]',
        0x16B7DCC:'edi, byte ptr [r14 + rbp + 0x28]',0x16B7DD2:'edx, 2',
        0x16B7DD7:'0x1896df0',0x16B7DDC:'edi, eax',0x16B7DE1:'0x16b7df1'},
        0x1896DF0:{0x1896DF8:'edx, 2',0x1896E85:'ebp, dword ptr [rax + 0xc]',
            0x1896E91:'0x1896ec0',0x1896E9B:'eax, ebp'},
        0x1DD25B0:{0x1DD28AF:'0x16b7d90',0x1DD299E:'0x16b7d90'}}
    for entry,expected in checks.items():
        for address,operand in expected.items():
            assert bodies[entry][address].op_str==operand
    report=dict(exe_sha256=im.meta['exe_sha256'],methods=methods,
        facts=['16B7D90 compares each nonzero recipe+20 item required BYTE+28 against1896DF0(item,2), exits on first shortage.',
            '1896DF0 selector2 sums matched storage record+C with1896EC0 query result, preserving native special-item137 branch.',
            'Palico menu1DD25B0 calls this material predicate in both item branches.'],
        limits=['This predicate is material sufficiency only, not all unlock/progression/funds/capacity conditions.',
            '1896EC0 query implementation and full menu enable-state result still need certification.',
            'Do not force empty recipes to pass; native predicate returns false for no nonzero material entries.',
            'No native mechanism execution or dynamic test.'])
    (P/'research/horizon-other-palico-material-admission.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(material_slots=4,combined_inventory_selector=2,menu_predicate_calls=2)))


if __name__=='__main__':
    main()
