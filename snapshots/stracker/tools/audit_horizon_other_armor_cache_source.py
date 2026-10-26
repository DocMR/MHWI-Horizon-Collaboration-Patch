"""Certify one native character-equipment cache update route, without execution."""
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage();methods={};maps={}
 for a in [0x12ECDF0,0x13548A0,0x19B0210,0x279DE0]:
  if im.owner(a):f,b=decode_group(im,a)
  else:f,b=[],im.instructions(a)
  assert b;maps[a]={i.address:i for i in b}
  methods[hex(a)]={'fragments':[list(v) for v in f],
    'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in b)).hexdigest(),
    'instructions':[im.format(i) for i in b]}
 b=maps[0x12ECDF0]
 assert im.target(b[0x12ECE25])==0x5013950 and b[0x12ECE30].op_str=='0x1b8dbb0'
 calls=[0x12ECE3D,0x12ECE60,0x12ECE83,0x12ECEA6,0x12ECEC9]
 stores=[0x12ECE42,0x12ECE65,0x12ECE88,0x12ECEAB,0x12ECED3]
 for n,(call,store) in enumerate(zip(calls,stores)):
  assert b[call].op_str=='0x13548a0'
  assert b[store].op_str==f'dword ptr [rbx + 0x{0x1373c+4*n:x}], eax'
 assert b[0x12ED086].op_str=='0x2041700' and b[0x12ED0A3].op_str=='0x2041ea0'
 g=maps[0x13548A0]
 assert g[0x1354902].op_str=='r9d, dword ptr [rcx + rax*4 + 0xa8]'
 assert g[0x1354913].op_str=='rcx, rax, 0x98' and g[0x135491A].op_str=='rax, [r10 + 0x41068]'
 assert g[0x1354926].op_str=='eax, dword ptr [rax + 0x24]'
 r=maps[0x19B0210];c=maps[0x279DE0]
 assert r[0x19B06C7].op_str=='word ptr [rbp + 0x33], ax'
 assert c[0x279ED4].op_str=='eax, word ptr [rdx + 0x3c]'
 assert c[0x279ED8].op_str=='word ptr [rcx + 0x1e], ax'
 data=(P/'research/horizon-other-source/common/equip/armor.am_dat').read_bytes()
 for row in [445,1064,1681,2290,2905]:assert struct.unpack_from('<H',data,10+row*60+53)[0]==523
 report={'status':'native_equipment_cache_source_route_and_armor_id_conversion_verified',
  'exe_sha256':im.meta['exe_sha256'],'methods':methods,
  'native_id':{'serialized_offset':53,'temporary_offset':60,'native_offset':30,'value':523,
               'meaning':'Reader tempRBP-9;RBP+33=temp3C;279DE0 copies to native1E used by manager index.'},
  'route':'12ECDF0 mode3 path obtains current character through5013950/1B8DBB0, queries13548A0 selectors1..5 and writes owner+1373C..1374C, then calls2041700/tail2041EA0.',
  'character_selection':'13548A0 normal path uses character+A8[selector] as equipment record index, stride98 base41068 and field24; alternate mode2691A0 has preserved special logic.',
  'limits':['One native cache source route, not all equip/change/load/network paths or caller mode provenance.',
   'Completion does not establish skill refresh ordering in every lifecycle.',
   'Shield-Weaver defense/reaction/depletion/recovery effects still incomplete.',
   'No native code execution or game-directory writes.']}
 (P/'research/horizon-other-armor-cache-source.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('Native character->five armor cache route and serialized523->native1E verified; full lifecycle/effects pending.')
if __name__=='__main__':main()
