"""Bind a native entity notification to the already certified goal counter."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    fragments,wrapper=decode_group(im,0x1B68B10)
    w={i.address:i for i in wrapper}
    for site,operand in {
        0x1B68B23:'edi, r8d',0x1B68B26:'rsi, rdx',
        0x1B68B41:'dword ptr [rbx + 0x38], 2',0x1B68B47:'dword ptr [rbx + 0x54], 2',
        0x1B68B4D:'dword ptr [rbx + 0x3c], 2',0x1B68B60:'edi, 1',
        0x1B68B6C:'0x1cb4fb0',0x1B68B71:'dword ptr [rbx + 0x4c], 0x25f',
        0x1B68B78:'esi, eax',0x1B68BD9:'r9b, 1',0x1B68BDC:'r8d, edi',
        0x1B68BDF:'edx, esi',0x1B68BE1:'rcx, rbx',0x1B68BE4:'0x1b69060'}.items():
        assert w[site].op_str==operand,im.format(w[site])
    assert w[0x1B68B63].mnemonic=='ja'
    target=im.instructions(0x1CB4FB0)
    assert len(target)==2 and target[0].op_str=='eax, dword ptr [rcx + 0x12280]'
    # Authenticated no-pdata tail wrapper; do not infer a RET boundary.
    start,end=0x1CC2D90,0x1CC2DA2
    raw=im.code[start-im.base:end-im.base]
    assert all(im.valid[start-im.base:end-im.base])
    tail=list(im.md.disasm(raw,start));assert sum(i.size for i in tail)==len(raw)
    t={i.address:i for i in tail}
    assert t[0x1CC2D90].op_str=='r8d, edx' and t[0x1CC2D93].op_str=='rdx, rcx'
    assert im.target(t[0x1CC2D96])==0x500ED30
    assert t[0x1CC2D9D].mnemonic=='jmp' and t[0x1CC2D9D].op_str=='0x1b68b10'
    assert im.code[end-im.base]==0xCC
    notification=im.instructions(0x16B9661)
    n={i.address:i for i in notification}
    for site,operand in {
        0x16B9629:'ecx, dword ptr [rsi + 0x18]',0x16B962C:'edx, edx',
        0x16B962E:'0x16b3ed0',0x16B9633:'rdi, rax',0x16B9642:'0x1bad520',
        0x16B9650:'ebx, byte ptr [rsi + 0x20]',0x16B9657:'0x1bad520',
        0x16B965C:'rcx, rax',0x16B965F:'edx, ebx',0x16B9661:'0x1cc2d90'}.items():
        assert n[site].op_str==operand
    resolver=im.md.disasm(im.code[0x1BAD520-im.base:0x1BAD525-im.base],0x1BAD520)
    resolver=list(resolver)
    assert len(resolver)==1 and resolver[0].op_str=='0xa16d00'
    entity=im.instructions(0xA16D00)
    e={i.address:i for i in entity}
    assert e[0xA16D00].op_str=='rax, qword ptr [rcx + 0x138]'
    assert e[0xA16D0C].op_str=='byte ptr [rax + 0xc], 0xe'
    quests=(67101,67102,67607,67802,67803,67804)
    assert all(q!=0x25F for q in quests)
    report={'status':'native_entity_notification_to_goal_counter_segment_verified_type_labels_pending',
        'exe_sha256':im.meta['exe_sha256'],
        'notification_window':[im.format(i) for i in notification if 0x16B961A<=i.address<0x16B966B],
        'entity_resolver':[im.format(i) for i in resolver+entity],
        'tail_bridge':[im.format(i) for i in tail],
        'wrapper_fragments':[list(f) for f in fragments],
        'wrapper_sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in wrapper)).hexdigest(),
        'wrapper_instructions':[im.format(i) for i in wrapper],
        'concrete_type_followup':'cPacketEmQuestDie registration/constructors/discriminator independently certified in horizon-other-quest-die-packet-type.json.',
        'flow':['Notification18 lookup -> object138 valid entity; notification20 byte becomes event kind.',
            'Native1CC2D90 supplies manager500ED30 and forwards entity/kind to1B68B10.',
            'Wrapper preserves conditional critical-section handling and native phase/class checks; accepts only kind<=1 and nonnull entity.',
            'Entity12280 supplies target ID; native1B69060 receives that ID, original kind and R9b1.',
            'The hardcoded quest607 special branch in this wrapper does not match any of the six targets.'],
        'implementation_decision':'Keep this native entity notification and threshold route; no new entity-death hook or custom target counter is justified by this segment.',
        'limits':['Semantic meaning of kind0/1 remains to certify; concrete packet type is resolved in the followup report, but field identity alone is not a kill/capture label.',
            'Lookup helper16B3ED0 semantics and remote/local authority delivery outside this bounded branch remain pending.',
            'Does not claim all target events or exactly-once delivery proved.',
            'Static data/code only; no target execution, dynamic test, save edit or game-directory write.']}
    (P/'research/horizon-other-quest-entity-goal-notification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Native notification -> valid entity -> entity target ID -> original goal counter route certified; original gates retained.')


if __name__=='__main__':main()
