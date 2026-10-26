"""Static armor serialized-to-native skill fields and accumulation binding."""
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage();methods={};maps={}
 for entry in [0x1EC411,0x19AFF80,0x19B0210,0x279DE0]:
  if im.owner(entry): f,b=decode_group(im,entry)
  else:f,b=[],im.instructions(entry)
  assert b;maps[entry]={i.address:i for i in b}
  methods[hex(entry)]={'fragments':[list(v) for v in f],
    'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in b)).hexdigest(),
    'instructions':[im.format(i) for i in b]}
 assert im.target(maps[0x1EC411][0x1EC411])==0x33DE870
 assert im.exe[im.raw(0x33DE870):im.raw(0x33DE870)+11]==b'rArmorData\0'
 assert im.pointer(0x33DE850+16)==0x19AFF80
 assert im.target(maps[0x19AFF80][0x19AFF96])==0x33DE880
 assert im.pointer(0x33DE880+0x50)==0x19B0210
 r=maps[0x19B0210];assert r[0x19B0770].op_str=='0x279de0'
 assert r[0x19B059F].op_str=='word ptr [rbp + 0x1d], ax'
 assert r[0x19B05C7].op_str=='byte ptr [rbp + 0x27], al'
 c=maps[0x279DE0]
 for addr,s in [(0x279E83,'eax, word ptr [rdx + 0x26]'),(0x279E87,'word ptr [rcx + 0x36], ax'),
                (0x279E8B,'eax, byte ptr [rdx + 0x30]'),(0x279E8F,'byte ptr [rcx + 0x31], al')]:assert c[addr].op_str==s
 # Bounded tail-call wrapper, no pdata: exclude following INT3 padding.
 a,z=0x1671510,0x167153A;payload=im.code[a-im.base:z-im.base]
 assert 0 not in im.valid[a-im.base:z-im.base]
 wrapper=list(im.md.disasm(payload,a));assert sum(i.size for i in wrapper)==z-a
 assert any(i.address==0x1671532 and i.op_str=='0x279740' for i in wrapper)
 accum=json.loads((P/'research/horizon-other-skill-accumulator.json').read_text(encoding='utf-8'))
 assert any('012F8359:' in s and 'word ptr [rax + 0x36]' in s for s in accum['instructions'])
 assert any('012F8369:' in s and 'byte ptr [rax + 0x31]' in s for s in accum['instructions'])
 assert any('012F836D:' in s and 'byte ptr [rbx + rdx*8 + 0x80], cl' in s for s in accum['instructions'])
 data=(P/'research/horizon-other-source/common/equip/armor.am_dat').read_bytes()
 rows=[]
 for index in [445,1064,1681,2290,2905]:
  raw=data[10+index*60:10+(index+1)*60]
  assert struct.unpack_from('<H',raw,34)[0]==209 and raw[36]==1
  rows.append({'row':index,'raw':raw.hex(),'skill_id':209,'points':1})
 report={'status':'armor_first_skill_native_fields_verified_snapshot_identity_pending',
   'exe_sha256':im.meta['exe_sha256'],'methods':methods,
   'wrapper':[im.format(i) for i in wrapper],
   'fields':[{'file':34,'temporary':38,'native':54,'width':2,'meaning':'first skill point ID'},
             {'file':36,'temporary':48,'native':49,'width':1,'meaning':'first skill point count'}],
   'temporary_base':'Reader temp beginsRBP-9; storesRBP+1D/+27 ->temp26/+30.',
   'rows':rows,
   'accumulator':'12F8359 readsword36, 12F8369 readsbyte31; 12F836D adds value to controller+80+24*ID. On these converted records each contributes1 to209.',
   'limits':['Current equipment snapshot slot/class identity and manager279740 population still to certify end-to-end.',
    'Five matching source records do not prove all five currently equipped or refreshed in game.',
    'Serial-to-temp offsets follow reader order and existing60-byte schema; no target reader executed.',
    'No defense/reaction/depletion/recovery effect implementation claim.',
    'No game files written or native code executed.']}
 (P/'research/horizon-other-armor-skill-fields.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('Five Shield-Weaver records209/1 ->native36/31 ->accumulator count verified; snapshot/effects pending.')
if __name__=='__main__':main()
