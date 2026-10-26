"""Inventory explicit callback-field displacement candidates; receiver identity remains separate."""
import json
from analyze_hzd_code import CodeImage,P,X86_OP_MEM
im=CodeImage();needle=bytes.fromhex('a82b0000');at=0;cache={};rows=[];unknown=[]
while True:
 at=im.code.find(needle,at)
 if at<0:break
 address=im.base+at;at+=1
 if not all(im.valid[at-1:at+3]):continue
 owner=im.owner(address)
 if not owner:continue
 if owner[0] not in cache:cache[owner[0]]=im.instructions(owner[0])
 xs=cache[owner[0]]
 if not xs:unknown.append(hex(address));continue
 for n,i in enumerate(xs):
  if i.address<=address<i.address+i.size and any(o.type==X86_OP_MEM and o.mem.disp==0x2BA8 for o in i.operands):
   rows.append({'rva':hex(i.address),'fragment':hex(owner[0]),'instruction':im.format(i),'context':[im.format(j) for j in xs[max(0,n-10):n+5]]})
report={'exe_sha256':im.meta['exe_sha256'],'field':'0x2BA8','references':rows,'undecoded_fragment_candidates':unknown,'limits':['Field-offset candidates only; quest-row receiver identity must be proven separately.','No execution or native predicate replacement.']}
(P/'research/horizon-other-accept-callback-field-uses.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'references':len(rows),'undecoded_candidates':len(unknown)}))
for row in rows:print(row['fragment'],row['instruction'])
