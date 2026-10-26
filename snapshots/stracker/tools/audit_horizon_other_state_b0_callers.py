"""Find validated direct callers of quest state-bit producers and consumers without execution."""
import json, struct
from analyze_hzd_code import CodeImage, P, X86_OP_IMM
from pc_unwind_groups import decode_group
im=CodeImage(); targets={0x134EFE0,0x1369680}; candidates=[]
for opcode in (b'\xe8',b'\xe9'):
 at=0
 while True:
  at=im.code.find(opcode,at)
  if at<0:break
  if at+5<=len(im.code) and all(im.valid[at:at+5]):
   target=im.base+at+5+struct.unpack_from('<i',im.code,at+1)[0]
   if target in targets:candidates.append((im.base+at,target))
  at+=1
cache={}; rows=[]; unresolved=[]
for address,target in candidates:
 owner=im.owner(address)
 if not owner:unresolved.append(hex(address));continue
 if owner[0] not in cache:
  try:cache[owner[0]]=decode_group(im,owner[0])
  except AssertionError:
   local=im.instructions(owner[0])
   cache[owner[0]]=([owner],local) if local else None
 group=cache[owner[0]]
 if not group:unresolved.append(hex(address));continue
 fs,xs=group
 match=next((n for n,i in enumerate(xs) if i.address==address),None)
 if match is None:continue
 i=xs[match]
 assert i.mnemonic in ('call','jmp') and i.operands[0].type==X86_OP_IMM and i.operands[0].imm==target
 rows.append({'rva':hex(address),'target':hex(target),'root':hex(fs[0][0]),'decoded_scope':'complete group or validated local fragment; not universal caller coverage','context':[im.format(j) for j in xs[max(0,match-22):match+8]]})
report={'exe_sha256':im.meta['exe_sha256'],'direct_edges':rows,'unresolved_candidates':unresolved,
 'limits':['Only validated direct rel32 calls/tails in retained code; indirect dispatch not covered.','No preconditions inferred just from caller presence; no save or game code executed.']}
(P/'research/horizon-other-state-b0-callers.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'validated_edges':len(rows),'unresolved':len(unresolved)}))
for row in rows:print(row['rva'],row['target'],row['root'])
