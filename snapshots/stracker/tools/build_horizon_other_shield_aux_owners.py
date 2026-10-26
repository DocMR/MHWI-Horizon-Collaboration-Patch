"""Generate full native auxiliary lifecycle owner guards without target execution."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();owners=[];lines=['// Generated original damage state plus damage/combine/sync lifecycle guards.','constexpr TreeOwnerSpec kShieldAuxiliaryOwners[] = {']
for start in [0x1216EF0,0x1217340,0x12EA460,0x12EAA40,0x11927A0,0x1193610,0x121B8D0,0x118EFE0]:
 fragments,body=decode_group(im,start);assert fragments[0][0]==start
 cursor=start
 for begin,finish,unwind in fragments:
  assert begin==cursor
  cursor=finish
 end=cursor;raw=im.code[start-im.base:end-im.base];assert len(raw)<=16384 and 0 not in im.valid[start-im.base:end-im.base]
 digest=hashlib.sha256(raw).digest();owners.append({'rva':hex(start),'bytes':len(raw),'sha256':digest.hex(),'instructions':len(body),'fragments':[list(f) for f in fragments]})
 lines.append('    {'+f'0x{start:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
lines+=['};','']
(P/'staging/Horizon-other-module/src/generated_shield_aux_owners.inc').write_text('\n'.join(lines),encoding='utf-8')
(P/'research/horizon-other-shield-auxiliary-owners.json').write_text(json.dumps({'exe_sha256':im.meta['exe_sha256'],'owners':owners,'executed':False},indent=2),encoding='utf-8')
print('Eight complete auxiliary lifecycle/state-update owner guards generated.')
