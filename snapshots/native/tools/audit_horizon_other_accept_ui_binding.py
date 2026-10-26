"""Bind native quest-accept UI identity and callback from static code/data."""
import json,hashlib
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage()
registration={i.address:i for i in im.instructions(0x145770)}
assert im.target(registration[0x145781])==0x32244F0
assert im.target(registration[0x14578C])==0x4F9E930
at=im.raw(0x32244F0);assert im.exe[at:at+16]==b'uGUIQuestAccept\0'
factory={i.address:i for i in im.instructions(0x1135260)}
assert im.target(factory[0x113526A])==0x4F9E930
assert im.target(factory[0x11352B6])==0x3224518
assert factory[0x11352C7].bytes.hex()=='488903'
entries=[]
for n in range(200):
 target=im.pointer(0x3224518+n*8)
 if not im.base<=target<im.base+len(im.code):break
 entries.append({'slot':n,'rva':hex(target)})
assert n==93
assert im.pointer(0x3224518+76*8)==0x1135BA0
functions=[]
for root in (0x1135BA0,0x1135410):
 fs,xs=decode_group(im,root)
 functions.append({'root':hex(root),'fragments':[list(f) for f in fs],'instructions':[im.format(i) for i in xs]})
callback=im.instructions(0x11356B0)
assert callback and any(i.bytes.hex()=='c680a82b000001' for i in callback)
assert im.pointer(0x3224840+2*8)==0x11356B0
fs,selected=decode_group(im,0x1B659B0)
selected_by={i.address:i for i in selected}
assert selected_by[0x1B65A36].bytes.hex()=='8b4830'
assert selected_by[0x1B65A40].bytes.hex()=='894b4c'
assert selected_by[0x1B65A4C].bytes.hex()=='0fbe4834'
assert selected_by[0x1B65A50].bytes.hex()=='894b50'
report={'selected_quest_materialization':{'root':'0x1B659B0','fragment_count':len(fs),'instruction_count':len(selected),'observed_fields':'row+30 ID -> manager+4C; row+34 star -> manager+50; also reads completion and SOS state later in the same function','instructions':[im.format(i) for i in selected]},'exe_sha256':im.meta['exe_sha256'],'type_name':'uGUIQuestAccept','registration':'0x145770','dti':'0x4F9E930','factory':'0x1135260','instance_vtable':'0x3224518','consecutive_code_pointer_entries':entries,
 'table_boundary_limit':'93 executable entries until non-code data; not a proof of complete base ABI ownership.',
 'functions':functions,'callback':{'vtable':'0x3224840','slot':2,'rva':'0x11356B0','instructions':[im.format(i) for i in callback]},
 'limits':['Named UI identity and dispatch graph only; full admission predicate not identified by this report.','Callback receiver provenance and following state-machine transition still require tracing.','No native code executed or hooked.']}
(P/'research/horizon-other-accept-ui-binding.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'native_type':report['type_name'],'vtable_entries':len(entries),'full_admission_proven':False}))
