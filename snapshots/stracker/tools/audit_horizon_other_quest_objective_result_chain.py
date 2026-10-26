"""Bind six source objective modes to native condition/result generation."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    parser=im.instructions(0x1A10170);pb={i.address:i for i in parser}
    cursor=6;mode_offset=None
    for i in parser:
        if not 0x1A1026D<=i.address<=0x1A10505:continue
        width=0
        if i.mnemonic=='call' and i.op_str in ('qword ptr [rax + 0x10]','qword ptr [rax + 0x30]'):width=4
        elif i.mnemonic=='call' and i.op_str=='qword ptr [rax + 8]':width=2
        elif i.mnemonic=='movzx' and i.op_str=='eax, byte ptr [rcx + rax]':width=1
        if width:
            if i.address==0x1A10505:mode_offset=cursor
            cursor+=width
    assert mode_offset==0x67 and pb[0x1A1050D].op_str=='byte ptr [rbp - 0x28], al'
    a=0x130D560;z=next(v for v in im.starts if v>a)
    copy=list(im.md.disasm(im.code[a-im.base:z-im.base],a));cb={i.address:i for i in copy}
    assert cb[0x130D647].op_str=='eax, byte ptr [rdx + 0x78]'
    assert cb[0x130D64B].op_str=='byte ptr [rcx + 0x70], al'
    functions={}
    bodies={}
    for address in (0x1B659B0,0x1B4EE80,0x1B6F540,0x1B71A80):
        if im.owner(address):
            fragments,body=decode_group(im,address)
        else:
            fragments,body=[],im.instructions(address)
            assert body and body[0].address==address and body[-1].mnemonic=='ret'
        bodies[address]={i.address:i for i in body}
        functions[hex(address)]={'fragments':[list(f) for f in fragments],
            'instruction_count':len(body),'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            'instructions':[im.format(i) for i in body]}
    setup=bodies[0x1B659B0]
    assert setup[0x1B65B3E].op_str=='eax, byte ptr [rbp + 0x70]'
    assert setup[0x1B65B45].op_str=='dword ptr [rbx + 0x88], eax'
    predicate=bodies[0x1B4EE80]
    for site,operand in {0x1B4EE80:'edx, dword ptr [rcx + 0x88]',0x1B4EE8C:'0x1b4ef51',
        0x1B4EF51:'eax, dword ptr [rcx + 0x98]',0x1B4EF57:'eax, 0x18',
        0x1B4EF5B:'0x1b4ef74',0x1B4EF5D:'eax, 0x1b',0x1B4EF61:'0x1b4ef74',
        0x1B4EF63:'eax, 0x1a',0x1B4EF70:'r8d, eax'}.items():assert predicate[site].op_str==operand
    assert predicate[0x1B4EF70].mnemonic=='cmovb'
    main={i.address:i for i in im.instructions(0x1B60433)}
    assert main[0x1B60982].op_str=='0x1b4ee80'
    assert main[0x1B60989].op_str=='0x1b60a4a'
    assert main[0x1B6099E].op_str=='edx, dword ptr [rdi + 0x13180]'
    assert main[0x1B609A7].op_str=='0x1b6f540'
    producer=bodies[0x1B6F540]
    assert producer[0x1B6F58F].op_str=='dword ptr [rbx + 0x54], 3'
    assert producer[0x1B6F599].op_str=='0x1b6f640'
    assert producer[0x1B6F566].op_str=='r9b, 3'
    marker=bodies[0x1B71A80]
    assert marker[0x1B71AC3].mnemonic=='bts' and marker[0x1B71AC3].op_str=='eax, 0x1a'
    assert marker[0x1B71AC7].op_str=='dword ptr [rdi + rdx*8 + 0x98], eax'
    rows=[]
    for q in (67101,67102,67607,67802,67803,67804):
        path=P/f'research/horizon-other-quest-source/decoded/questData_{q}.bin'
        raw=path.read_bytes();assert raw[mode_offset]==1
        rows.append({'quest_id':q,'objective_mode':raw[mode_offset],
            'plaintext_sha256':hashlib.sha256(raw).hexdigest()})
    report={'status':'six_mode1_objective_predicate_to_native_result3_generation_verified_event_sources_pending',
        'exe_sha256':im.meta['exe_sha256'],'quests':rows,
        'mode_flow':'MIB67 byte -> temporary78/RBP-28 -> resource body70 -> manager88',
        'mode1_predicate':'manager98 bit24 set AND bit27 clear AND bit26 set',
        'result_flow':'Main controller1B60982 tests1B4EE80; true reaches1B6F540 with native carried manager13180 value. Native route either dispatches category3 or, after existing gates, sets manager54=3 and begins native result processing.',
        'bit26_writer':'1B71A80 marks a selected objective entry via1B71AC3/C7; exact event callers are not yet certified here.',
        'completion_connection':'Native result category3 is accepted by ordinary consumer query2, documented in horizon-other-quest-normal-result-route.json. Extra native gates remain.',
        'functions':functions,
        'controller_window':[im.format(i) for i in main.values() if 0x1B6097F<=i.address<0x1B609B0],
        'limits':['Mode/bit conditions and native result generation are certified, not every monster/capture/delivery event that marks the objective.',
            'Preserve native online/authority/293F5 and existing-result gates; no unconditional result3 setter or counter-zero shortcut is added.',
            'Exact failure/abandon gameplay labels and all six goal-event sources remain incomplete.',
            'Static data/code only; no target execution, dynamic test or game-directory write.']}
    (P/'research/horizon-other-quest-objective-result-chain.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('All six source objective modes1 linked to native objective-bit predicate and result3 generation; original gates retained.')


if __name__=='__main__':main()
