"""Inventory saved-code references to the native quest-open bitmap; static only."""
import json, struct

from analyze_hzd_code import CodeImage, P
from capstone.x86 import X86_OP_MEM
from pc_unwind_groups import decode_group
im=CodeImage()
needle=struct.pack('<I',0x104BD8)
positions=[]
at=0
while True:
 at=im.code.find(needle,at)
 if at<0: break
 if all(im.valid[at:at+4]): positions.append(im.base+at)
 at+=1
rows=[]; unavailable=[]; seen=set()
for address in positions:
 owner=im.owner(address)
 if not owner:
  unavailable.append(hex(address)); continue
 if owner[0] in seen: continue
 seen.add(owner[0])
 try: fragments,xs=decode_group(im,owner[0])
 except AssertionError:
  unavailable.append(hex(address)); continue
 for n,i in enumerate(xs):
  if not any(o.type==X86_OP_MEM and o.mem.disp==0x104BD8 for o in i.operands):continue
  rows.append({'owner':hex(fragments[0][0]),'rva':hex(i.address),'instruction':im.format(i),
   'context':[im.format(j) for j in xs[max(0,n-9):n+10]],
   'direct_calls':[hex(j.operands[0].imm) for j in xs if j.mnemonic=='call' and j.operands and j.operands[0].type==2]})
groups=[]
for root in (0x134F070,0x1365F00,0x13697A0):
 fragments,xs=decode_group(im,root)
 groups.append({'root':hex(root),'fragments':[list(f) for f in fragments],'instructions':[im.format(i) for i in xs]})
report={'native_groups':groups,'exe_sha256':im.meta['exe_sha256'],'field':'0x104BD8','valid_byte_occurrences':len(positions),'references':rows,'unresolved_occurrences':unavailable,
 'limits':['Static exported-code field-displacement inventory; indirect field access and computed addresses are not covered.','No open bits seeded; no target code or save state executed or written.']}
(P/'research/horizon-other-open-bitmap-references.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'occurrences':len(positions),'references':len(rows),'unresolved':len(unavailable)}))
for row in rows:print(row['owner'],row['instruction'])
