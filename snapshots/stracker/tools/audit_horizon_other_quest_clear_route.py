"""Verify the native completion route for the six extended quest IDs statically."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group

def main():
    im=CodeImage()
    methods={}
    bodies={}
    for entry in (0x13610D0,0x1369540):
        fragments,body=decode_group(im,entry)
        bodies[entry]={i.address:i for i in body}
        methods[hex(entry)]={'fragments':[list(f) for f in fragments],
            'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            'instructions':[im.format(i) for i in body]}
    w=bodies[0x13610D0]
    for site,operand in {0x13610E6:'r15d, edx',0x13610F3:'edx, r15d',
        0x13610F6:'rcx, rsi',0x13610F9:'0x1369540'}.items():
        assert w[site].op_str==operand
    # No condition bypasses the initial clear writer inside this wrapper.
    assert not any(i.mnemonic.startswith('j') for i in w.values() if i.address<0x13610F9)
    b=bodies[0x1369540]
    for site,operand in {
        0x136955B:'edi, edx',0x1369580:'eax, edi',0x1369584:'eax, dword ptr [r15 + rdx*4 + 4]',
        0x136958F:'eax, -1',0x136959C:'rax, 5',0x13695A0:'ecx, 0x1f',
        0x13695AD:'eax, ecx',0x13695B0:'dword ptr [rdx + 0x104c20], eax',
        0x13695B9:'dword ptr [r14 + rax*4 + 0x1484b4]'}.items():
        assert b[site].op_str==operand
    assert b[0x13695AD].mnemonic=='bts' and b[0x13695B9].mnemonic=='inc'
    assert im.target(b[0x1369569])==0x324FAC0
    start,end=0x1B616BC,0x1B616D3
    raw=im.code[start-im.base:end-im.base]
    assert all(im.valid[start-im.base:end-im.base])
    caller=list(im.md.disasm(raw,start));assert sum(i.size for i in caller)==len(raw)
    c={i.address:i for i in caller}
    assert c[0x1B616C8].op_str=='edx, dword ptr [rdi + 0x4c]'
    assert c[0x1B616CE].op_str=='0x13610d0' and im.target(c[0x1B616BC])==0x5013950
    assert all(im.valid[0x1B62098-im.base:0x1B620FC-im.base])
    dispatch={i.address:i for entry in (0x1B60420,0x1B60433) for i in im.instructions(entry)}
    assert dispatch[0x1B60427].op_str=='eax, dword ptr [rdi + 0x38]'
    assert dispatch[0x1B6042A].op_str=='eax, 0x18'
    assert dispatch[0x1B6045A].op_str=='ecx, dword ptr [rdx + rax*4 + 0x1b62098]'
    assert im.target(dispatch[0x1B6043B])==0 and dispatch[0x1B60464].op_str=='rcx'
    switches=struct.unpack('<25I',im.code[0x1B62098-im.base:0x1B620FC-im.base])
    assert switches[9]==start
    candidate=json.loads((P/'staging/Horizon-other-module/merge/quest-save-map-candidate.json').read_text(encoding='utf-8'))
    ids=candidate['candidate_entries'][:-1]
    rows=[]
    for row in candidate['new_index_candidates']:
        quest,index=row['quest_id'],row['index']
        assert ids[index]==quest and index<512
        # Native special aliases in the wrapper/writer do not involve these IDs.
        assert quest not in (102,50891,50892,5003,805,806,961,971)
        rows.append({'quest_id':quest,'candidate_index':index,
            'clear_word_offset':hex(0x104C20+(index//32)*4),'bit':index%32,
            'counter_offset':hex(0x1484B4+index*4)})
    report={'status':'native_completion_wrapper_and_writer_route_verified_not_gameplay_acceptance',
        'exe_sha256':im.meta['exe_sha256'],'methods':methods,
        'completion_state':9,'caller_window':[im.format(i) for i in caller],
        'dispatch_window':[im.format(i) for i in dispatch.values() if 0x1B60427<=i.address<=0x1B60464],
        'selected_quest_source':'native completion manager+4C -> EDX; current character from5013950/1B8DBB0',
        'applicability':'State9 is one existing route, not a certified default result route for these six quests. Their initial source flags lack400; see horizon-other-quest-result-flags.json.',
        'target_writes_if_candidate_mapping_active':rows,
        'implementation_decision':'Retain the native success wrapper and clear writer. Existing extended mapping redirection is the relevant delta; do not add a second writer or fabricate success.',
        'remaining':['The quest success/failed/abandoned producers that enter completion state9 are not proved by this bounded caller window.',
            'Candidate appended-index ownership, all admission and save/reload paths still need full closure; no installation permission is granted.',
            'Four64-byte serialized bitmaps are independently recorded in horizon-other-save-mapping-audit.json; this report is not a dynamic save test.'],
        'dynamic_tests_executed':False}
    (P/'research/horizon-other-quest-clear-route.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'Native state9 -> initial clear writer and {len(rows)} candidate bit/counter locations verified; no duplicate clear hook added.')

if __name__=='__main__':main()
