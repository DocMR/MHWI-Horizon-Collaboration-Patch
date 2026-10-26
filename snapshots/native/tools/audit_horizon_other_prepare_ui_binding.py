"""Bind native quest-prepare class to the accept-state consumer."""
import json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();name=0x3224CE8;at=im.raw(name)
assert im.exe[at:at+17]==b'uGUIQuestPrepare\0'
registration=im.instructions(0x1458A1);by={i.address:i for i in registration}
assert im.target(by[0x1458A1])==name
assert any(im.target(i)==0x4F9E9D8 for i in registration)
getter=im.instructions(0x113C150);assert len(getter)==2 and im.target(getter[0])==0x4F9E9D8
assert im.pointer(0x3224D18+4*8)==0x113C150
assert im.pointer(0x3224D18+81*8)==0x113F750
_,ctor=decode_group(im,0x1137BD0);cb={i.address:i for i in ctor}
assert im.target(cb[0x1137C0E])==0x3224D18
assert any(i.mnemonic=='mov' and i.operands[0].type==3 and i.operands[0].mem.disp==0 and i.op_str.endswith(', rax') for i in ctor)
_,update=decode_group(im,0x113F750);ub={i.address:i for i in update}
for a in (0x113F780,0x113F79A):assert ub[a].operands[0].imm==0x113F480
report={'exe_sha256':im.meta['exe_sha256'],'type':'uGUIQuestPrepare','name_rva':hex(name),'dti':'0x4F9E9D8','vtable':'0x3224D18','getter_slot':4,
 'update_slot':81,'update':'0x113F750','state_consumer':'0x113F480','registration':[im.format(i) for i in registration],
 'constructor':[im.format(i) for i in ctor],'update_instructions':[im.format(i) for i in update],
 'conclusion':'The +2BA8 consumer is dispatched by the native QuestPrepare class update, not by the smaller QuestAccept object itself.',
 'limits':['QuestAccept callback R8 provenance into the prepare object remains unresolved.','Class identity does not prove complete admission/HR/MR checks or execution ordering.','No native code executed, no hooks installed.']}
(P/'research/horizon-other-prepare-ui-binding.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'native_type':report['type'],'update_slot':81,'consumer':'0x113F480'}))
