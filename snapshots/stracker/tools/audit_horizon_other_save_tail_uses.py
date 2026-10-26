"""Inventory explicit displacements into the unused-by-original-map state tails."""
import re,json,struct
from analyze_hzd_code import CodeImage,P,X86_OP_MEM
from capstone.x86 import X86_REG_RAX,X86_REG_EAX,X86_REG_AX,X86_REG_AL
im=CodeImage()
ranges=[(base+44,base+64) for base in (0x104B88,0x104BD8,0x104C20,0x104C68,0x104CB0)]
ranges.append((0x1484B4+352*4,0x1484B4+512*4))
cache={};rows=[];unknown=[];rejected=[];candidates=0
# These historical byte matches cross instruction fields, not save displacements.
false_matches={0x2758315:(0x2758315,'e98a140000'),
               0x29781C1:(0x29781C0,'80b98a14000000'),
               0x297993D:(0x297993B,'4889818c140000')}
for m in re.finditer(rb'[\x00-\xff][\x4b-\x4c]\x10\x00|[\x00-\xff][\x8a-\x8c]\x14\x00',im.code):
 at=m.start();value=struct.unpack_from('<I',im.code,at)[0]
 if not any(a<=value<z for a,z in ranges) or not all(im.valid[at:at+4]):continue
 candidates+=1;address=im.base+at;owner=im.owner(address)
 if address in false_matches:
  start,expected=false_matches[address];raw=bytes.fromhex(expected);offset=start-im.base
  assert im.code[offset:offset+len(raw)]==raw and all(im.valid[offset:offset+len(raw)])
  instruction=list(im.md.disasm(raw,start))
  assert len(instruction)==1 and instruction[0].size==len(raw)
  assert not any(o.type==X86_OP_MEM and o.mem.disp==value for o in instruction[0].operands)
  rejected.append({'candidate':hex(address),'instruction':im.format(instruction[0]),
                   'reason':'matched bytes are not a memory displacement into the audited ranges'})
  continue
 if not owner:unknown.append(hex(address));continue
 if owner[0] not in cache:cache[owner[0]]=im.instructions(owner[0])
 xs=cache[owner[0]]
 if not xs:unknown.append(hex(address));continue
 for n,i in enumerate(xs):
  if i.address<=address<i.address+i.size and any(o.type==X86_OP_MEM and o.mem.disp==value for o in i.operands):
   prior=next((k for k in range(n-1,-1,-1) if set(xs[k].regs_access()[1])&{X86_REG_RAX,X86_REG_EAX,X86_REG_AX,X86_REG_AL}),None)
   zero_source=prior is not None and xs[prior].bytes.hex()=='33c0' and all(j.mnemonic not in ('call','jmp') and not j.mnemonic.startswith('j') for j in xs[prior+1:n])
   rows.append({'linear_zero_source':im.format(xs[prior]) if zero_source else None,'rva':hex(i.address),'displacement':hex(value),'fragment':hex(owner[0]),'instruction':im.format(i),'context':[im.format(j) for j in xs[max(0,n-4):n+5]]})
report={'exe_sha256':im.meta['exe_sha256'],'ranges':[[hex(a),hex(z)] for a,z in ranges],'valid_byte_candidates':candidates,'direct_tail_displacement_uses':rows,'unresolved_candidates':unknown,'rejected_cross_field_matches':rejected,
 'limits':['Explicit displacement uses only; base-plus-variable-index and computed pointers are not covered.','Zero explicit tail uses does not alone prove exclusive index ownership.','No native execution or save modification.']}
(P/'research/horizon-other-save-tail-uses.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'candidates':candidates,'direct_tail_uses':len(rows),'unresolved':len(unknown)}))
for row in rows:print(row['instruction'])
