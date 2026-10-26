"""Certify primary resource load ordering; disassembly only, no target execution."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P


def main():
    im=CodeImage()
    a,z=0x1B7BFE0,0x1B7C21E
    raw=im.code[a-im.base:z-im.base]
    assert all(im.valid[a-im.base:z-im.base])
    body=list(im.md.disasm(raw,a))
    assert sum(i.size for i in body)==len(raw)
    b={i.address:i for i in body}
    checks={
        0x1B7BFFF: 'eax, dword ptr [rcx + 0x38]',
        0x1B7C005: 'eax, 0xb',
        0x1B7C011: 'r8d, dword ptr [rcx + rax*4 + 0x1b7c754]',
        0x1B7C078: 'esi, 0x1cf',
        0x1B7C0CB: 'dword ptr [rdi + 0x38], 2',
        0x1B7C0D7: 'edx, dword ptr [rdi + 0x3c]',
        0x1B7C0E1: '0x1b80f10',
        0x1B7C0E8: '0x1b7c026',
        0x1B7C0EE: 'dword ptr [rdi + 0x38], 3',
        0x1B7C125: 'r9d, 1',
        0x1B7C132: 'qword ptr [rax + 0x58]',
        0x1B7C14B: '0x2216e40',
        0x1B7C156: 'rcx, [rdi + rdx*8]',
        0x1B7C15E: '0x18af170',
        0x1B7C16B: '0x22174d0',
        0x1B7C178: '0x22174d0',
        0x1B7C18C: 'qword ptr [r14], rsi',
        0x1B7C192: 'r12, 0x10',
        0x1B7C196: 'r14, 8',
        0x1B7C19A: 'r15d, 0x1cf',
        0x1B7C1AB: 'dword ptr [rdi + 0x38], 4',
        0x1B7C1CF: '0x279080',
    }
    for site,operand in checks.items():
        assert b[site].op_str==operand,im.format(b[site])
    for site,target in [(0x1B7C071,0x500F4D0),(0x1B7C0FC,0x5011F40),
                        (0x1B7C106,0x500F4D8),(0x1B7C10D,0x5011F40)]:
        assert im.target(b[site])==target
    table=im.code[0x1B7C754-im.base:0x1B7C784-im.base]
    assert len(table)==48 and all(im.valid[0x1B7C754-im.base:0x1B7C784-im.base])
    stages=struct.unpack('<12I',table)
    assert stages[:5]==(0x1B7C01F,0x1B7C043,0x1B7C0D7,0x1B7C0FA,0x1B7C1B7)
    poll=im.instructions(0x1B80F10)
    assert len(poll)==10
    pb={i.address:i for i in poll}
    assert pb[0x1B80F1B].op_str=='dword ptr [rcx + rdx*8 + 0x4c], 3'
    assert pb[0x1B80F13].op_str=='0x1b80f25'
    snapshots=json.loads((P/'research/equip-descriptor-separate-snapshots.json').read_text(encoding='utf-8'))
    row=next(r for r in snapshots['tables'][0]['entries'] if r['index']==5)
    assert row['type']=='0x4f86028' and row['path']=='common\\pl\\pl_skill_param'
    assert int(row['result_global'],16)==0x5011F40+5*8==0x5011F68
    report={
        'status':'primary_request_poll_assignment_order_verified_resource_lifetime_still_borrowed',
        'exe_sha256':im.meta['exe_sha256'],
        'linear_window':{'start':hex(a),'end':hex(z),'instructions':len(body),
            'sha256':hashlib.sha256(raw).hexdigest(),'code':[im.format(i) for i in body]},
        'switch_stage_targets':[hex(v) for v in stages],
        'poll_helper':[im.format(i) for i in poll],
        'skill_descriptor':row,
        'order':[
            'stage1 requests primary descriptor table463 and enters2',
            'stage2 polls loader group from receiver+3C; false retains stage2, true enters3',
            'stage3 gets each resource via manager5123A00 virtual58 with flags1, assigns owning global slot via18AF170 and releases temporary references',
            'null descriptor clears owning slot; after463 iterations receiver+38 becomes4',
            'stage4 later calls armor index279080, so index build follows primary assignment in this native state machine'],
        'integration_consequences':[
            'Early lifecycle hook preparation must not be gated on already-populated skill resource globals: the resource is populated later by the native loader.',
            'Equipment callbacks borrow the resource synchronously and re-read its live slot/type; do not cache a pointer across loader replacement or unload.',
            'Non-null slot/type/field readability is not an independently proved ready flag or lifetime guarantee.',
            'Do not use text-table stage6/7 as the primary skill-resource stage2/3 gate.'],
        'limits':['Contiguous primary stages0..4 prefix, not all loader states or callgraph ordering relative to character construction.',
            'Poll checks group state3 (or returns true for group -1); this does not define every resource ready bit.',
            'Shared assignment reference-count semantics recorded in pc-text-resource-assignment.json; no pin/release calls executed here.',
            'Static code only; no runtime observations, dynamic tests or game-directory writes.']}
    (P/'research/horizon-other-shield-resource-load-order.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'Primary stages2/3, skill descriptor5 and native assignment order verified ({len(body)} instructions); no live-ready claim.')


if __name__=='__main__':main()
