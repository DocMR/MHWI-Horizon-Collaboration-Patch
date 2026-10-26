"""Trace serialized quest flags into manager state without changing quest bytes."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P


def main():
    im=CodeImage()
    parser=im.instructions(0x1A10170)
    b={i.address:i for i in parser}
    cursor,reads=6,[]
    for i in parser:
        if not 0x1A1026D<=i.address<=0x1A105AE:continue
        width=0
        if i.mnemonic=='call' and i.op_str in ('qword ptr [rax + 0x10]','qword ptr [rax + 0x30]'):width=4
        elif i.mnemonic=='call' and i.op_str=='qword ptr [rax + 8]':width=2
        elif i.mnemonic=='movzx' and i.op_str=='eax, byte ptr [rcx + rax]':width=1
        if width:
            reads.append({'site':hex(i.address),'offset':hex(cursor),'width':width})
            cursor+=width
    assert reads[-1]=={'site':'0x1a105ae','offset':'0x84','width':4}
    widths=[]
    for slot,width in ((8,2),(16,4),(48,4)):
        target=im.pointer(0x34DF3C8+slot)
        body=im.instructions(target)
        assert any(i.mnemonic=='add' and i.op_str==f'ecx, {width}' for i in body)
        widths.append({'slot':hex(slot),'target':hex(target),'width':width})
    assert b[0x1A1017B].op_str=='rbp, [rsp - 0xc30]'
    assert b[0x1A10183].op_str=='rsp, 0xd30'
    assert b[0x1A105B1].op_str=='dword ptr [rbp - 0x18], eax'
    # RBP=RSP+100; temporary begins atRSP+60, hence RBP-18=temporary+88.
    assert b[0x1A11BD3].op_str=='rcx, [rdi + 0xb0]'
    assert b[0x1A11BE1].op_str=='rdx, [rsp + 0x60]'
    assert b[0x1A11BE6].op_str=='0x130d560'
    a=0x130D560;z=next(v for v in im.starts if v>a)
    raw=im.code[a-im.base:z-im.base];assert all(im.valid[a-im.base:z-im.base])
    copy=list(im.md.disasm(raw,a));assert sum(i.size for i in copy)==len(raw)
    cb={i.address:i for i in copy}
    assert cb[0x130D69B].op_str=='eax, dword ptr [rdx + 0x88]'
    assert cb[0x130D6A1].op_str=='dword ptr [rcx + 0xc], eax'
    getter=im.instructions(0x30F150)
    assert len(getter)==2 and getter[0].op_str=='rax, [rcx + 0xb0]'
    setup=im.instructions(0x1B65A20);s={i.address:i for i in setup}
    for site,operand in {0x1B65A25:'0x30f150',0x1B65A36:'ecx, dword ptr [rax + 0x30]',
        0x1B65A40:'dword ptr [rbx + 0x4c], ecx',0x1B65A53:'ecx, dword ptr [rax + 0xc]',
        0x1B65A56:'dword ptr [rbx + 0x7c], ecx'}.items():
        assert s[site].op_str==operand
    rows=[]
    for q in (67101,67102,67607,67802,67803,67804):
        plain=P/f'research/horizon-other-quest-source/decoded/questData_{q}.bin'
        source=P/f'research/horizon-other-quest-source/quest/questData_{q}.mib'
        staged=P/f'staging/Horizon-other-module/resources/nativePC/quest/questData_{q}.mib'
        assert source.read_bytes()==staged.read_bytes()
        flags=struct.unpack_from('<I',plain.read_bytes(),0x84)[0]
        assert not flags&0x400
        rows.append({'quest_id':q,'source_flags':hex(flags),'bit400':False,
            'staged_preserves_source':True,'mib_sha256':hashlib.sha256(staged.read_bytes()).hexdigest()})
    report={'status':'six_quest_source_flags_reach_native_manager_bit400_unset',
        'exe_sha256':im.meta['exe_sha256'],'sequential_reads':reads,'stream_method_widths':widths,
        'flow':'MIB84 DWORD -> temporary88 (RBP-18) -> resource body0C (resource+BC) -> manager7C at1B65A56',
        'setup_fragment':[im.format(i) for i in setup],'quests':rows,
        'consequence':'State9 after bit400 test is not the default route implied by these six source files. Preserve flags and trace ordinary result processing; do not set400 to force clear.',
        'remaining':['This proves the original manager flag producer, not that no subsequent runtime mutation exists.',
            'Ordinary success/failure/abandon producers and normal completion UI callbacks still need closure.',
            'Static disassembly/data only; no native execution, save edit or game-directory write.']}
    (P/'research/horizon-other-quest-result-flags.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Six source/staged MIB flags and parser -> resource -> manager flow verified: bit400 unset for all targets.')


if __name__=='__main__':main()
