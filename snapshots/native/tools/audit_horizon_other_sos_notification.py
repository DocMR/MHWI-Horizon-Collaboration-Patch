"""Trace late-join bit transition to its native SOS text, statically."""
import json,struct,hashlib
from analyze_hzd_code import CodeImage,P
from build_horizon_other_text_candidate import parse
from pc_unwind_groups import decode_group
im=CodeImage()
def block(a,z):
 assert all(im.valid[a-im.base:z-im.base])
 xs=list(im.md.disasm(im.code[a-im.base:z-im.base],a));assert sum(i.size for i in xs)==z-a
 return xs
prefix=block(0x18BBDB0,0x18BBDE7)
table=0x18BBEE8;assert all(im.valid[table-im.base:table-im.base+24*4])
target=struct.unpack_from('<I',im.code,table-im.base+19*4)[0];assert target==0x18BBEAD
arm=block(target,0x18BBEB6);assert im.target(arm[0])==0x5012E40 and arm[1].operands[0].imm==0x18BBED8
end=block(0x18BBED8,0x18BBEE8);assert end[3].operands[0].imm==0x2244930
snap=json.loads((P/'research/equip-descriptor-separate-snapshots.json').read_text(encoding='utf-8'))
descriptor=next(r for r in snap['tables'][1]['entries'] if r['index']==16)
assert descriptor['result_global']=='0x5012e40' and descriptor['path']=='common\\text\\cm_dialog_jpn'
textfile=P/'MHWI解包/chunk/common/text/steam/cm_dialog_eng.gmd'
if not textfile.exists():textfile=P/'MHWI解包/chunk/common/text/cm_dialog_eng.gmd'
data=textfile.read_bytes();_,texts,keys=parse(data)
assert keys[52]==b'DLG_SOS_OPEN_02' and b'SOS flare' in texts[52]
paths=[]
for root,call in [(0xB3C180,0xB3C2B8),(0x1A82BB0,0x1A82DB2)]:
 _,xs=decode_group(im,root);n=next(k for k,i in enumerate(xs) if i.address==call)
 assert xs[n].operands[0].imm==0x18BBDB0
 assert xs[n-2].op_str=='edx, 0x34' and xs[n-1].op_str=='ecx, [rdx - 0x21]'
 paths.append({'root':hex(root),'call':hex(call),'text_group':19,'text_index':52,'instructions':[im.format(i) for i in xs[n-2:n+9]]})
report={'exe_sha256':im.meta['exe_sha256'],'paths':paths,'dispatcher_instructions':[im.format(i) for i in prefix+arm+end],
 'descriptor':descriptor,'text_file':str(textfile.relative_to(P)),'text_sha256':hashlib.sha256(data).hexdigest(),'key':keys[52].decode(),'english_text':texts[52].decode(),
 'conclusion':'The observed bit-change notification resolves to the native SOS-flare-unlocked message, supporting late-join/SOS state identity rather than quest completion.',
 'limits':['No inference that all indirect consumers or network behavior are covered.','No code executed, no save bits changed, no dynamic test.']}
(P/'research/horizon-other-sos-notification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'paths':len(paths),'key':report['key'],'text':report['english_text']}))
