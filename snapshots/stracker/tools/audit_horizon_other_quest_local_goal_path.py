"""Certify the parallel native local goal path, including its inline tables."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from capstone.x86 import X86_OP_IMM


def main():
    im=CodeImage();start,table,selectors,end=0x1B68C40,0x1B68FF0,0x1B68FF8,0x1B6905D
    raw=im.code[start-im.base:end-im.base]
    assert len(raw)==end-start and all(im.valid[start-im.base:end-im.base])
    body=list(im.md.disasm(im.code[start-im.base:table-im.base],start))
    assert sum(i.size for i in body)==table-start and body[-1].mnemonic=='ret'
    b={i.address:i for i in body}
    targets=struct.unpack('<2I',im.code[table-im.base:selectors-im.base])
    cases=list(im.code[selectors-im.base:end-im.base])
    assert len(cases)==101 and max(cases)==1 and all(v in b for v in targets)
    for i in body:
        if i.mnemonic.startswith('j') and i.operands[0].type==X86_OP_IMM:
            assert i.operands[0].imm in b or (i.address==0x1B68C73 and i.operands[0].imm==0x1B69060)
    for site,operand in {0x1B68C51:'ebp, r8d',0x1B68C5A:'esi, edx',
        0x1B68C5C:'dword ptr [rax + 0x4c], 0x641',
        0x1B68F39:'esi, 0x64',0x1B68F45:'eax, byte ptr [rdx + rsi + 0x1b68ff8]',
        0x1B68F4D:'ecx, dword ptr [rdx + rax*4 + 0x1b68ff0]',
        0x1B68FB9:'dword ptr [rbx + 0x38], 2',0x1B68FBF:'dword ptr [rbx + 0x54], 2',
        0x1B68FC5:'r9d, r9d',0x1B68FC8:'r8d, ebp',0x1B68FCB:'edx, esi',
        0x1B68FD0:'0x1b69060'}.items():assert b[site].op_str==operand
    counter=im.instructions(0x1B69060);c={i.address:i for i in counter}
    assert c[0x1B69089].op_str=='r13d, r9b'
    assert c[0x1B690FF].op_str=='r13b, r13b'
    assert c[0x1B69102].op_str=='0x1b69125'
    assert c[0x1B6911C].op_str=='edx, [r9 + 0x30]'
    assert c[0x1B69120].op_str=='0x1b37640'
    assert c[0x1B691E1].op_str=='word ptr [rbx]'
    rows=im.instructions(0x1B68711);n=next(n for n,i in enumerate(rows) if i.address==0x1B68711)
    assert rows[n].op_str=='0x1b68c40'
    quests=(67101,67102,67607,67802,67803,67804)
    assert all(q!=0x641 for q in quests)
    report={'status':'complete_parallel_native_goal_path_recovered_sender_not_yet_certified',
        'exe_sha256':im.meta['exe_sha256'],'start':hex(start),'end':hex(end),
        'sha256':hashlib.sha256(raw).hexdigest(),'instruction_count':len(body),
        'target_table':[hex(v) for v in targets],'selector_count':len(cases),
        'selector_table':cases,'instructions':[im.format(i) for i in body],
        'caller_window':[im.format(i) for i in rows[n-8:n+3]],
        'flow':'1B68711 ->1B68C40 retains native record/stat processing; if phase38==2 and class54==2 then forwards targetID/kind to1B69060 withR9b0. Quest1601 has a separate original direct tail route, not used by these six targets.',
        'fourth_argument':'Compared with packet receive routeR9b1, R9b0 controls the post-count request1B37640(manager500EC40,event30) when at least one increment occurred. It does not itself gate the count increment.',
        'implementation_decision':'Preserve both original paths and their fourth argument. Do not add deduplication or assumeR9b1 suppresses native target counting.',
        'limits':['The local/packet path names describe caller context only; full authority and exactly-once relationship still need source audit.',
            'Inline101-entry table is part of native stat/record handling, not a new quest exclusion table.',
            'No packet sender discovered by this report; event0/1 gameplay names remain pending.',
            'Static disassembly only; no execution, dynamic test, save edit or game-directory write.']}
    (P/'research/horizon-other-quest-local-goal-path.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'Parallel native goal path fully decoded ({len(body)} instructions,2 targets,101 selectors); post-count flag role verified.')


if __name__=='__main__':main()
