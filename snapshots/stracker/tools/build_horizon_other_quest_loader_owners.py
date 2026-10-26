"""Certify complete quest loader/allocation owners without executing them."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
im=CodeImage()
lines=['// Generated complete original quest-resource code identities.','constexpr TreeOwnerSpec kQuestResourceOwners[] = {']
owners=[]
for start in (0x478A20,0x1A10170,0x32A330,0x32A350,0x249B70,0x19751B0):
    if start == 0x478A20:
        # This reader spans five chained unwind fragments, not only its prefix.
        fragments,ins=decode_group(im,start)
        assert fragments[0][0] == start
        assert all(left[1] == right[0] for left,right in zip(fragments,fragments[1:]))
        expected_end=fragments[-1][1]
    else:
        ins=im.instructions(start)
        owner=im.owner(start)
        expected_end=owner[1] if owner else None
    assert ins and ins[0].address == start
    end=ins[-1].address+ins[-1].size
    cursor=start
    for i in ins:
        assert i.address == cursor
        cursor += i.size
    raw=im.code[start-im.base:end-im.base]
    assert all(im.valid[start-im.base:end-im.base])
    assert expected_end is None or end == expected_end
    digest=hashlib.sha256(raw).digest()
    owners.append({'rva':start,'bytes':len(raw),'sha256':digest.hex()})
    lines.append('    {'+f'0x{start:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
lines+=['};','']
(P/'staging/Horizon-other-module/src/generated_quest_resource_owners.inc').write_text('\n'.join(lines),encoding='utf-8')
classification={'schema_version':1,'module':'horizon-other','exe_sha256':im.meta['exe_sha256'],
 'owner':owners[-1],'quest_ids':[67101,67102,67607,67802,67803,67804],
 'source_category':3,'replacement_category':1,
 'unrelated_rule':'Call original category function with unchanged arguments',
 'preserved_native_checks':['star filter F2C320','star getter 1976240','rank classification 1976110','open getter 134F070','admission conditions'],
 'installation_allowed':False,'reason':'Implementation queued only; complete open/save/bootstrap integration pending'}
(P/'staging/Horizon-other-module/merge/quest-classification.json').write_text(json.dumps(classification,indent=2),encoding='utf-8')
print('Six complete quest-resource/classification owner identities generated.')
