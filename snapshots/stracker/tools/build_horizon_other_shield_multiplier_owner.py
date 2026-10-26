"""Certify the full native intermediate multiplier owner and insertion bytes."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();fragments,body=decode_group(im,0x11B9D44);start=fragments[0][0];cursor=start
for a,z,u in fragments:
 assert a==cursor;cursor=z
raw=im.code[start-im.base:cursor-im.base];assert len(raw)<=16384 and 0 not in im.valid[start-im.base:cursor-im.base]
b={i.address:i for i in body};assert b[0x11B9D44].op_str=='rcx, qword ptr [rbx + 8]' and b[0x11B9D48].op_str=='r9d, r9d'
assert im.code[0x11B9D44-im.base:0x11B9D4B-im.base].hex()=='488b4b084533c9'
assert b[0x11B9D53].op_str=='0x1922190'
assert not any('ymm' in i.op_str for i in body)
digest=hashlib.sha256(raw).digest()
lines=['// Generated complete native intermediate arithmetic owner.','constexpr TreeOwnerSpec kShieldMultiplierOwner = {'+f'0x{start:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}};','']
(P/'staging/Horizon-other-module/src/generated_shield_multiplier_owner.inc').write_text('\n'.join(lines),encoding='utf-8')
(P/'research/horizon-other-shield-multiplier-owner.json').write_text(json.dumps({'exe_sha256':im.meta['exe_sha256'],'rva':hex(start),'bytes':len(raw),'sha256':digest.hex(),'fragments':[list(f) for f in fragments],'site':'0x11b9d44','original':'488b4b084533c9','accumulator':'XMM6 low float; complete owner contains no YMM operands','instructions':[im.format(i) for i in body],'executed':False},indent=2),encoding='utf-8')
print('Full multiplier owner, intermediate7-byte insertion and native skill14 continuation verified.')
