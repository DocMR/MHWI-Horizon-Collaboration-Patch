"""Generate complete original status lifecycle hashes; never execute target code."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();owners=[];lines=['// Generated complete native status lifecycle owner guards.','constexpr TreeOwnerSpec kShieldLifecycleOwners[] = {']
for start in [0x11B6450,0x11C34A0,0x11B68A0]:
 fragments,body=decode_group(im,start);assert len(fragments)==1 and fragments[0][0]==start
 end=fragments[0][1];raw=im.code[start-im.base:end-im.base];assert len(raw)<=16384 and 0 not in im.valid[start-im.base:end-im.base]
 digest=hashlib.sha256(raw).digest();owners.append({'rva':hex(start),'bytes':len(raw),'sha256':digest.hex(),'instructions':len(body)})
 lines.append('    {'+f'0x{start:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
lines+=['};','']
(P/'staging/Horizon-other-module/src/generated_shield_lifecycle_owners.inc').write_text('\n'.join(lines),encoding='utf-8')
(P/'research/horizon-other-shield-lifecycle-owners.json').write_text(json.dumps({'exe_sha256':im.meta['exe_sha256'],'owners':owners,'executed':False},indent=2),encoding='utf-8')
print('Three complete status lifecycle original-owner guards generated.')
