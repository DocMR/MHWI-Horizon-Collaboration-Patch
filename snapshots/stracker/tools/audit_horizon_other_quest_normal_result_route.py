"""Certify the ordinary result consumer's conditional clear call, statically."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from analyze_ps_stormbolt_types import PsCodeImage


def main():
    im=CodeImage()
    fragments,body=decode_group(im,0x1EB44D0)
    b={i.address:i for i in body}
    for site,operand in {
        0x1EB4534:'dl, 2',0x1EB453D:'0x1b66e00',0x1EB4544:'0x1eb4bfa',
        0x1EB4558:'0x1b57a10',0x1EB456C:'0x134ef50',
        0x1EB4571:'byte ptr [rsi + 0x28fd], al',0x1EB4581:'eax, 0xf',
        0x1EB4587:'0x1eb45b9',0x1EB45A4:'0x134f070',0x1EB45AC:'0x1eb4a45',
        0x1EB45B9:'byte ptr [rcx + 0x131e8], 0',0x1EB45C0:'0x1eb4a4c',
        0x1EB45CD:'0x1b57a10',0x1EB45D7:'0x1b8dbb0',
        0x1EB45DF:'edx, edi',0x1EB45E1:'0x13610d0',
        0x1EB45ED:'0x1eb4605',0x1EB4C1C:'0x1361230'}.items():
        assert b[site].op_str==operand,im.format(b[site])
    for site,target in ((0x1EB451C,0x500ED30), # RCX manager loaded before category query
                        (0x1EB45C6,0x5013950)):
        assert im.target(b[site])==target
    getter=im.instructions(0x1B57A10)
    assert len(getter)==2 and getter[0].op_str=='eax, dword ptr [rcx + 0x4c]'
    predicate=im.instructions(0x1B66E00)
    pb={i.address:i for i in predicate}
    assert pb[0x1B66E0F].op_str=='edx, dword ptr [r8 + rax*4 + 0x1b66e84]'
    assert all(im.valid[0x1B66E84-im.base:0x1B66EA0-im.base])
    targets=struct.unpack('<7I',im.code[0x1B66E84-im.base:0x1B66EA0-im.base])
    assert targets[2]==0x1B66E3C
    for site,operand in {0x1B66E3C:'eax, dword ptr [rcx + 0x54]',
        0x1B66E3F:'eax, 3',0x1B66E42:'eax, 1',0x1B66E45:'0x1b66e27',
        0x1B66E47:'al, 1'}.items():assert pb[site].op_str==operand
    assert pb[0x1B66E45].mnemonic=='ja'
    # Revalidate the two already located direct callers at real boundaries.
    callers=[]
    for address in (0x1EB650A,0x1EB6FD5):
        rows=im.instructions(address)
        n=next(n for n,i in enumerate(rows) if i.address==address)
        assert rows[n].op_str=='0x1eb44d0' and rows[n-1].op_str=='rcx, rdi'
        callers.append({'site':hex(address),'window':[im.format(i) for i in rows[n-2:n+3]]})
    # Existing first-clear branch is downstream of the shared completion bit.
    # Keep this branch instead of adding a second reward/clear callback.
    for site,operand in {0x1EB45E6:'byte ptr [rsi + 0x28fd], 0',
                          0x1EB45ED:'0x1eb4605',0x1EB45F6:'0x1b57a10',
                          0x1EB45FB:'edx, eax',0x1EB4600:'0x1e973a0'}.items():
        assert b[site].op_str==operand
    assert b[0x1EB45ED].mnemonic=='jne'
    complete=im.instructions(0x134EF50);cb={i.address:i for i in complete}
    assert cb[0x134EF63].op_str=='0x19751b0'
    assert cb[0x134EF68].op_str=='eax, 2'
    assert im.target(cb[0x134EF7C])==0x324FAC0
    assert cb[0x134EFC0].op_str=='eax, dword ptr [rdi + rax*4 + 0x104c20]'
    assert cb[0x134EFCA].mnemonic=='setb'
    references=(P/'staging/Horizon-other-module/src/generated_quest_mapping_references.inc').read_text(encoding='utf-8')
    assert '0x134EF7C' in references
    # Six IDs all take this bounded comparison path; unrelated switch tables
    # are data and must not be disassembled as if they were missing code.
    owned_ids=(67101,67102,67607,67802,67803,67804)
    assert all(identity>0x10522 and identity!=0x108DF for identity in owned_ids)
    owned_path=[]
    for start,end in ((0x1E973A0,0x1E973B5),(0x1E97A94,0x1E97AA0),
                      (0x1E97CED,0x1E97CF9),(0x1E97F81,0x1E97F8D),
                      (0x1E98240,0x1E9824C),(0x1E98377,0x1E98387),
                      (0x1E98405,0x1E9840B)):
        raw=im.code[start-im.base:end-im.base]
        assert all(im.valid[start-im.base:end-im.base])
        rows=list(im.md.disasm(raw,start));assert sum(i.size for i in rows)==len(raw)
        owned_path.extend(rows)
    path_by={i.address:i for i in owned_path}
    for address,target in {0x1E973AF:'0x1e97a94',0x1E97A9A:'0x1e97ced',
                           0x1E97CF3:'0x1e97f81',0x1E97F87:'0x1e98240',
                           0x1E98246:'0x1e98377',0x1E98385:'0x1e98405'}.items():
        assert path_by[address].op_str==target
    assert path_by[0x1E98377].op_str=='edx, 0x10522'
    assert path_by[0x1E9837F].op_str=='edx, 0x108df'
    assert owned_path[-1].mnemonic=='ret'
    ps=PsCodeImage()
    ps_body=ps.instructions(0x44D2080);assert ps_body
    ps_by={i.address:i for i in ps_body}
    for address,operand in {0x44D208A:'esi, 0x4ef',0x44D2092:'esi, 0x619',
                            0x44D209E:'esi, 0xc995',0x44D20A4:'0x44d220e',
                            0x44D220E:'esi, 0x10510',0x44D221A:'eax, [rsi - 0x10511]',
                            0x44D2220:'eax, 0x11',0x44D2223:'0x44d26c1',
                            0x44D26C1:'esi, 0x108df',0x44D26C7:'0x44d2d55'}.items():
        assert ps_by[address].op_str==operand
    ps_path=[i for i in ps_body if any(a<=i.address<z for a,z in (
        (0x44D2080,0x44D20AA),(0x44D220E,0x44D2229),
        (0x44D26C1,0x44D26CD),(0x44D2D55,0x44D2D60)))]
    assert ps_path[-1].mnemonic=='ret'
    parent=ps.instructions(0x44B7A4C);parent_by={i.address:i for i in parent}
    for address,operand in {0x44B7A30:'0x2f01d50',0x44B7A35:'byte ptr [r13 + 0x2221], 0',
                            0x44B7A3D:'0x44b7a51',0x44B7A49:'esi, dword ptr [rax + 0x28]',
                            0x44B7A4C:'0x44d2080'}.items():
        assert parent_by[address].op_str==operand
    report={
        'status':'normal_result_consumer_conditional_clear_call_verified_result_labels_pending',
        'exe_sha256':im.meta['exe_sha256'],'owner':'0x1eb44d0',
        'owner_fragments':[list(f) for f in fragments], 'owner_instruction_count':len(body),
        'owner_code_sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
        'result_query':'1B66E00(manager,2) accepts unsigned(manager54-3)<=1, i.e. native values3/4; all other values take branch1EB4BFA here.',
        'clear_call_gates':['Native result-query2 passes.',
            'If manager7C bit15 is set, original quest-open query must return true.',
            'Native manager131E8 gate must be zero.'],
        'pc_owned_first_clear_consumer':{'root':'0x1e973a0','owned_ids':list(owned_ids),
            'effect':'All six take comparison-only path to epilogue; no calls or game-data writes on that path.',
            'instructions':[im.format(i) for i in owned_path],
            'limit':'Matched paired PS consumer separately; this does not cover all reward producers.'},
        'ps_paired_first_clear_consumer':{'root':'0x44d2080','parent_call':'0x44b7a4c',
            'parent_prior_call':'0x44b7a30 -> 0x2f01d50', 'parent_skip_flag':'result+0x2221',
            'effect_for_owned_ids':'Same empty comparison-to-epilogue path as PC; no additional migration hook for these six IDs in this consumer.',
            'instructions':[ps.format(i) for i in ps_path],
            'limit':'Only this paired consumer; independent quest rewards and other source producers are outside this conclusion.'},
        'first_clear_branch':{'pre_write_query':'0x1eb456c -> 0x134ef50', 'snapshot':'consumer+0x28fd',
            'completion_write':'0x1eb45e1 -> 0x13610d0',
            'already_clear_branch':'0x1eb45ed skips0x1e973a0 when prior completion is nonzero',
            'first_clear_consumer_call':'0x1eb4600 -> 0x1e973a0(current quest)',
            'owned_policy':'category1 uses the redirected mapping and actual104C20 completion bit; never category2 always-clear shortcut',
            'scope':'Call gating and empty owned-ID paths in the paired PC/PS consumer are certified; other reward producers remain outside this scope.'},
        'record_flow':['Current quest getter1B57A10 reads manager+4C.',
            'Current character getter1B8DBB0 uses singleton5013950.',
            'Original completion query134EF50 result is saved at consumer28FD before write.',
            '1EB45E1 invokes13610D0 on the same current quest; subsequent28FD check preserves first/repeat branch distinction.'],
        'consumer_prefix':[im.format(i) for i in body if i.address<0x1EB4608],
        'predicate_code':[im.format(i) for i in predicate],
        'predicate_table':[hex(v) for v in targets],'direct_callers':callers,
        'implementation_decision':'Reuse this conditional native result route with extended quest mapping. Do not add another clear writer or set source flag400.',
        'limits':['Native result values3/4 are not yet assigned gameplay labels by this report; failure/abandon producer enums remain to certify.',
            'This is the direct clear-wrapper branch in this consumer, not proof no other completion path exists; later1361230 processing remains native.',
            'Caller instruction boundaries revalidated; surrounding caller UI states and exactly-once lifecycle still need closure.',
            'Static disassembly only, no target mechanism execution, dynamic test, save edit or game-directory write.']}
    (P/'research/horizon-other-quest-normal-result-route.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'Ordinary result consumer ({len(body)} instructions), native3/4 clear gate and two direct callers certified.')


if __name__=='__main__':main()
