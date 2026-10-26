"""Recover complete native result-controller code plus its inline state table."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from capstone.x86 import X86_OP_IMM, X86_OP_MEM


def main():
    im=CodeImage()
    start,code_end,table,end=0x1B602B0,0x1B62096,0x1B62098,0x1B620FC
    raw=im.code[start-im.base:end-im.base]
    assert len(raw)==end-start and all(im.valid[start-im.base:end-im.base])
    body=list(im.md.disasm(im.code[start-im.base:code_end-im.base],start))
    assert sum(i.size for i in body)==code_end-start and body[-1].mnemonic=='ret'
    assert im.code[code_end-im.base:table-im.base].hex()=='6690'
    b={i.address:i for i in body}
    states=struct.unpack('<25I',im.code[table-im.base:end-im.base])
    assert all(target in b for target in states)
    direct=[]
    indirect=[]
    for i in body:
        if i.mnemonic.startswith('j'):
            if i.operands[0].type==X86_OP_IMM:
                assert i.operands[0].imm in b,im.format(i)
                direct.append(i.address)
            else:
                indirect.append(i.address)
    assert indirect==[0x1B60464]
    assert b[0x1B60427].op_str=='eax, dword ptr [rdi + 0x38]'
    assert b[0x1B6042A].op_str=='eax, 0x18'
    assert b[0x1B6045A].op_str=='ecx, dword ptr [rdx + rax*4 + 0x1b62098]'
    assert states[9]==0x1B616BC
    assert b[0x1B61049].op_str=='dword ptr [rdi + 0x7c], 0x400'
    assert b[0x1B61050].op_str=='0x1b6105e'
    transitions=[]
    clear_calls=[]
    for n,i in enumerate(body):
        if (i.mnemonic=='mov' and len(i.operands)==2 and i.operands[0].type==X86_OP_MEM and
            i.operands[0].mem.disp==0x38 and i.operands[1].type==X86_OP_IMM):
            transitions.append({'site':hex(i.address),'state':i.operands[1].imm,
                'window':[im.format(v) for v in body[max(0,n-3):n+3]]})
        if i.mnemonic=='call' and i.operands[0].type==X86_OP_IMM and i.operands[0].imm==0x13610D0:
            clear_calls.append(hex(i.address))
    assert clear_calls==['0x1b616ce']
    into_clear=[r for r in transitions if r['state']==9]
    assert len(into_clear)==1 and into_clear[0]['site']=='0x1b61052'
    # The similarly named FSM action writes its explicit quest ID directly.
    handler=im.instructions(0xB42EA0)
    h={i.address:i for i in handler}
    assert h[0xB42EE9].op_str=='edx, dword ptr [rbx + 8]'
    assert h[0xB42EF4].mnemonic=='jmp' and h[0xB42EF4].op_str=='0x1369540'
    report={
        'status':'complete_result_controller_code_and_state_table_recovered_semantic_producers_pending',
        'exe_sha256':im.meta['exe_sha256'],'start':hex(start),'code_end':hex(code_end),
        'inline_table':hex(table),'end':hex(end),'sha256':hashlib.sha256(raw).hexdigest(),
        'instruction_count':len(body),'direct_branch_count':len(direct),
        'indirect_dispatches':[hex(v) for v in indirect],
        'state_targets':[hex(v) for v in states],'immediate_state_writes':transitions,
        'clear_wrapper_calls_in_complete_owner':clear_calls,
        'clear_state_transition':'Native manager7C bit400 set selects state9 at1B61052; clear bit continues1B6105E and native checks. No new forced transition is required.',
        'fsm_action_distinction':{'name':'QuestClearFlagOn_qt','handler':'0xb42ea0',
            'instructions':[im.format(i) for i in handler],
            'argument':'Script action+8 explicit quest ID, not automatically current quest or whole-quest success.'},
        'instructions':[im.format(i) for i in body],
        'limits':['All direct branch boundaries and the sole indirect25-state table verified for this complete owner. External calls and writes from other owners remain to trace.',
            'Meaning and producers of manager7C.bit400 and other result enums are not inferred from state numbers.',
            'Six-quest success/failure/abandon/save acceptance remains incomplete; no hook or save writes added.',
            'Static disassembly only, no target execution or dynamic tests.']}
    (P/'research/horizon-other-quest-result-controller.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'Complete result controller: {len(body)} instructions, {len(direct)} direct branches,25 state targets; one clear-wrapper call and one immediate state9 transition.')


if __name__=='__main__':main()
