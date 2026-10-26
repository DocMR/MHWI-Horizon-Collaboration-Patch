"""Certify raw armor index population/read and actor equipment snapshot offsets."""
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage()
 def span(a,z):
  raw=im.code[a-im.base:z-im.base];assert 0 not in im.valid[a-im.base:z-im.base]
  b=list(im.md.disasm(raw,a));assert sum(i.size for i in b)==z-a
  return b,{'range':[hex(a),hex(z)],'sha256':hashlib.sha256(raw).hexdigest(),'instructions':[im.format(i) for i in b]}
 b,pop=span(0x279080,0x2792CD);p={i.address:i for i in b}
 assert im.target(p[0x27921C])==0x5012128 and p[0x279245].op_str=='0x462700'
 assert p[0x27924D].op_str=='r9d, word ptr [rax + 0x1e]' and p[0x279252].op_str=='eax, byte ptr [rax + 0xc]'
 writes=[0x27926B,0x279276,0x279284,0x279292,0x2792A0,0x2792AE]
 bases=[8,0x2EF8,0x5DE8,0x8CD8,0xBBC8,0xEAB8]
 targets=struct.unpack_from('<6I',im.code,0x2792E8-im.base)
 assert targets==(0x279267,0x279272,0x279280,0x27928E,0x27929C,0x2792AA)
 for addr,base in zip(writes,bases):assert f'+ 0x{base:x}]' in p[addr].op_str if base!=8 else '+ 8]' in p[addr].op_str
 q,read=span(0x279740,0x2797F3);q={i.address:i for i in q}
 assert q[0x279743].op_str=='edx, 5'
 readtargets=struct.unpack_from('<6I',im.code,0x2797F4-im.base)
 assert readtargets==(0x279762,0x27977B,0x279793,0x2797AB,0x2797C3,0x2797DB)
 f,snapshot=decode_group(im,0x19A7450);s={i.address:i for i in snapshot}
 assert s[0x19A7493].op_str=='rdx, [r10 + 0x1376c]' and s[0x19A749A].op_str=='r9, [r10 + 0x1373c]'
 assert s[0x19A74A5].op_str=='dword ptr [rax - 0x10], r8d' and s[0x19A74AC].op_str=='dword ptr [rax - 0xc], ecx'
 assert s[0x19A74CF].op_str=='rax, 0x54' and s[0x19A74D3].op_str=='r8d, 6'
 slots=[{'slot':n,'actor_equipment_id_offset':hex(0x1373C+4*n),'actor_decorations_offset':hex(0x1376C+12*n),
         'snapshot_category_offset':hex(0x54+0x54*n),'snapshot_id_offset':hex(0x58+0x54*n),
         'manager_pointer_array':hex(bases[n])} for n in range(6)]
 report={'status':'actor_snapshot_and_unfiltered_armor_index_paths_verified_current_cache_update_pending',
  'exe_sha256':im.meta['exe_sha256'],'population':pop,'getter':read,
  'population_switch':[hex(x) for x in targets],'getter_switch':[hex(x) for x in readtargets],
  'snapshot':{'fragments':[list(x) for x in f],'instructions':[im.format(i) for i in snapshot]},
  'slots':slots,'index_capacity_per_slot':751,
  'finding':'Manager state0 traverses entire armor resource5012128 and inserts pointer by record slot+0C/native ID+1E. No278A00 recipe exclusion gate in this state0 path. Getter279740 reads same slot arrays, rejecting ID>=751.1671510 uses manager5011F00+127E8.',
  'shield_weaver':'Declared native ID523 is within getter bound for slots0..4. Conditional on source-to-runtime ID binding and actor cached IDs523, these arrays return the matching original records; no new skill accumulation required.',
  'limits':['Snapshot source is actor cached fields; their producer/current equipment lifecycle not yet certified.',
   'Native ID523 file-to-runtime+1E mapping remains to certify explicitly.',
   'This is not a crafting availability or Shield-Weaver effect completion claim.',
   'Static analysis only; no target execution or game-directory writes.']}
 (P/'research/horizon-other-equipped-armor-index.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('Six snapshot slots and unfiltered native armor index population/getter verified; cache producer/effects pending.')
if __name__=='__main__':main()
