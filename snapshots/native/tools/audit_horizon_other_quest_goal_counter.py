"""Certify six source objective records and native matching/count thresholds."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    parser=im.instructions(0x1A10170);p={i.address:i for i in parser}
    # Reuse the independently width-checked sequential parser offset evidence.
    reads={int(r['site'],16):r for r in json.loads(
        (P/'research/horizon-other-quest-result-flags.json').read_text(encoding='utf-8'))['sequential_reads']}
    for site,offset,width,store in (
        (0x1A10486,0x57,4,'dword ptr [rbp - 0x48], eax'),
        (0x1A10496,0x5B,2,'word ptr [rbp - 0x38], ax'),
        (0x1A104A7,0x5D,2,'word ptr [rbp - 0x30], ax')):
        assert reads[site]['offset']==hex(offset) and reads[site]['width']==width
        assert p[site+3].op_str==store
    a=0x130D560;z=next(v for v in im.starts if v>a)
    code=list(im.md.disasm(im.code[a-im.base:z-im.base],a));c={i.address:i for i in code}
    for site,operand in {0x130D61B:'eax, dword ptr [rdx + 0x58]',
        0x130D61E:'dword ptr [rcx + 0x50], eax',0x130D625:'word ptr [rcx + 0x60], ax',
        0x130D62D:'word ptr [rcx + 0x68], ax'}.items():assert c[site].op_str==operand
    functions={};bodies={}
    for entry in (0x1B659B0,0x1B69060,0x1B71A80):
        fragments,body=decode_group(im,entry);bodies[entry]={i.address:i for i in body}
        functions[hex(entry)]={'fragments':[list(v) for v in fragments],
            'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            'instructions':[im.format(i) for i in body]}
    s=bodies[0x1B659B0]
    assert s[0x1B65A39].op_str=='rdx, [rbx + 0xb6]'
    assert s[0x1B65A72].op_str=='r8d, dword ptr [rbp + rcx*4 + 0x50]'
    assert s[0x1B65A77].op_str=='dword ptr [rdx - 0x1e], r8d'
    assert s[0x1B65A80].op_str=='word ptr [rdx - 0x1a], ax'
    assert s[0x1B65A89].op_str=='word ptr [rdx - 0x18], ax'
    assert s[0x1B65A8D].op_str=='word ptr [rdx - 0x16], si'
    b=bodies[0x1B69060]
    for site,operand in {0x1B6907F:'rbx, [rcx + 0xa0]',0x1B690A0:'ecx, dword ptr [rbx - 8]',
        0x1B69147:'cl, 0x30',0x1B69150:'r8d, word ptr [rbx - 4]',
        0x1B69155:'r8d, ebp',0x1B6915F:'cl, 0x10',0x1B69164:'r12d, 1',
        0x1B6916E:'cl, 0x20',0x1B69173:'r12d, r12d',
        0x1B691E1:'word ptr [rbx]',0x1B691EA:'ax, word ptr [rbx - 2]',
        0x1B691EE:'0x1b690f4',0x1B691F9:'0x1b71a80'}.items():assert b[site].op_str==operand
    assert b[0x1B691E1].mnemonic=='inc' and b[0x1B691EE].mnemonic=='jb'
    marker=bodies[0x1B71A80]
    assert marker[0x1B71AA2].op_str=='rdx, [rbx + rbx*2]'
    assert marker[0x1B71AC3].op_str=='eax, 0x1a'
    assert marker[0x1B71AC7].op_str=='dword ptr [rdi + rdx*8 + 0x98], eax'
    rows=[]
    expected={67101:(49,0,1),67102:(33,49,8),67607:(49,20,2),
              67802:(49,94,1),67803:(49,95,1),67804:(49,64,1)}
    for q,first in expected.items():
        raw=(P/f'research/horizon-other-quest-source/decoded/questData_{q}.bin').read_bytes()
        goals=[struct.unpack_from('<IHH',raw,o) for o in (0x57,0x5F,0x68,0x70)]
        assert goals[0]==first and goals[1:]==[(0,0,0)]*3 and raw[0x67]==1
        rows.append({'quest_id':q,'first_objective':{'raw_flags':hex(first[0]),'target_id':first[1],
            'required_count':first[2]},'other_slots_zero':True,
            'event0_allowed_by_low_flags':bool(first[0]&0x20),
            'event1_allowed_by_low_flags':bool(first[0]&0x10)})
    callers=[]
    for site in (0x1B68BE4,0x1B68FD0):
        body=im.instructions(site);n=next(n for n,i in enumerate(body) if i.address==site)
        assert body[n].op_str=='0x1b69060'
        callers.append({'site':hex(site),'window':[im.format(i) for i in body[n-5:n+2]]})
    report={'status':'source_objective_fields_and_native_counter_to_marker_route_verified_event_upstream_pending',
        'exe_sha256':im.meta['exe_sha256'],'quests':rows,'functions':functions,'callers':callers,
        'native_layout':'manager+98+18*index flagsDWORD; +9C targetWORD; +9E requiredWORD; +A0 currentWORD',
        'source_first_goal':'MIB57/5B/5D -> temporary58/68/70 -> resource body50/60/68 -> manager98/9C/9E, currentA0 initialized zero.',
        'flow':'1B69060 checks active/disabled flags, target ID and event kind; increments native WORD count and calls1B71A80 when unsigned count>=required. Marker sets bit26 at corresponding18-stride objective.',
        'decision':'Retain native matching/count/marker behavior for all six source records; no custom kill counter or forced clear is needed for this certified segment.',
        'limits':['Event-kind0/1 gameplay names and upstream enemy/capture/collection producers are not independently certified here.',
            'The six records use only first objective, mode1. Do not expand this report to all objective formats or quests.',
            'External authority gates, repeated-event delivery and whole save acceptance remain native and pending final closure.',
            'Static code/data inspection only; no native execution, dynamic test or game-directory write.']}
    (P/'research/horizon-other-quest-goal-counter.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Six objective payloads mapped to native target/count fields and threshold->bit26 route; no counter replacement.')


if __name__=='__main__':main()
