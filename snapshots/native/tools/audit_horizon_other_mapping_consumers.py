"""Decode every currently known mapping consumer and retain field/index evidence."""
import json
from analyze_hzd_code import CodeImage, P, X86_OP_MEM, X86_OP_IMM
from pc_unwind_groups import decode_group
im=CodeImage()
audit=json.loads((P/'research/horizon-other-save-mapping-audit.json').read_text(encoding='utf-8'))
leaves={0x1355C43:0x1355C40,0x1355C52:0x1355C50,0x13569C2:0x13569C0}
groups={}
for ref in audit['mapping_range_rip_references']:
 address=int(ref['rva'],16)
 owner=im.owner(address)
 start=owner[0] if owner else leaves[address]
 if owner:
  fragments,xs=decode_group(im,start)
 else:
  xs=im.instructions(start);assert xs
  fragments=[(start,xs[-1].address+xs[-1].size,0)]
 root=fragments[0][0]
 if root not in groups:
  fields=[];bounds=[]
  for i in xs:
   for operand in i.operands:
    if operand.type==X86_OP_MEM and operand.mem.disp in (0x104B88,0x104BC8,0x104BD8,0x104C20,0x104C68,0x104CB0,0x1484B4):
     fields.append({'field':hex(operand.mem.disp),'instruction':im.format(i),'access_flags':operand.access})
    if operand.type==X86_OP_IMM and operand.imm in (352,358,512):bounds.append(im.format(i))
  groups[root]={'root':hex(root),'fragments':[list(f) for f in fragments],'mapping_references':[],
    'state_fields':fields,'literal_capacity_or_count_candidates':bounds,'instructions':[im.format(i) for i in xs]}
 assert any(i.address==address and im.target(i)==0x324FAC0 for i in xs)
 groups[root]['mapping_references'].append(ref)
report={'exe_sha256':im.meta['exe_sha256'],'reference_count':len(audit['mapping_range_rip_references']),'consumers':list(groups.values()),
 'installation_allowed':False,'limits':['Full groups/three bounded leaf bodies for known direct MOV/LEA references only.','Field access flags and numeric literals are evidence, not semantic or free-index certification.','No native code or save state executed or written.']}
(P/'research/horizon-other-mapping-consumers.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'references':report['reference_count'],'consumer_groups':len(groups),'field_accesses':sum(len(g['state_fields']) for g in groups.values())}))
