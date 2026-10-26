"""Certify serialized skill fields through native read/copy instructions; no execution."""
import hashlib,json,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage();methods={};by={}
 for entry in [0x1F15F1,0x1A211B0,0x1A21440,0x1321AF0,0x21756A0]:
  if im.owner(entry): fragments,body=decode_group(im,entry)
  else: fragments,body=[],im.instructions(entry)
  assert body
  by[entry]={i.address:i for i in body}
  methods[hex(entry)]={'fragments':[list(f) for f in fragments],
    'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
    'instructions':[im.format(i) for i in body]}
 assert im.target(by[0x1F15F1][0x1F15F1])==0x33EC160
 assert im.exe[im.raw(0x33EC160):im.raw(0x33EC160)+11]==b'rSkillData\0'
 assert im.pointer(0x33EC170+16)==0x1A211B0
 assert im.target(by[0x1A211B0][0x1A211C6])==0x33EC190
 assert im.pointer(0x33EC190+0x50)==0x1A21440
 b=by[0x1A21440]
 assert b[0x1A21467].op_str=='eax, 0x18091001' and b[0x1A2147D].op_str=='ecx, 0xbc'
 assert b[0x1A214E5].op_str=='qword ptr [rax + 8]' and b[0x1A214EB].op_str=='word ptr [rbp - 0x50], ax'
 assert b[0x1A21513].op_str=='byte ptr [rbp - 0x4e], al'
 assert b[0x1A21600].op_str=='0x1321af0'
 c=by[0x1321AF0]
 for a,s in [(0x1321AF0,'eax, word ptr [rdx]'),(0x1321AF3,'word ptr [rcx + 8], ax'),
             (0x1321AF7,'eax, byte ptr [rdx + 2]'),(0x1321AFB,'byte ptr [rcx + 0xa], al')]:assert c[a].op_str==s
 assert im.target(by[0x21756A0][0x21756A6])==0x34DF3C8
 assert im.pointer(0x34DF3C8+8)==0x2175950 and im.pointer(0x34DF3C8+16)==0x2175990
 raw=(P/'research/horizon-other-source/common/equip/skill_data.skl_dat').read_bytes()
 assert struct.unpack_from('<IH',raw)==(0x18091001,0xbc)
 assert len(raw)==10+35*501
 row=raw[10+467*35:10+468*35];point,threshold=struct.unpack_from('<HB',row)
 assert (point,threshold)==(209,5)
 assert all(v==0 for v in struct.unpack_from('<6I',row,3))
 report={'status':'skill209_record467_native_id_and_threshold_mapping_verified',
   'exe_sha256':im.meta['exe_sha256'],'methods':methods,
   'source_sha256':hashlib.sha256(raw).hexdigest(),
   'record':{'index':467,'raw':row.hex(),'skill_id':point,'threshold':threshold,'deferred_prerequisites':[0]*6},
   'field_map':[{'file':0,'temporary':0,'native':8,'width':2,'meaning':'skill point ID'},
                {'file':2,'temporary':2,'native':10,'width':1,'meaning':'count threshold'},
                {'file':3,'temporary':4,'native':12,'width':24,'meaning':'six deferred prerequisite record IDs'},
                {'file':27,'temporary':28,'native':36,'width':8,'meaning':'four trailing words; semantics not inferred'}],
   'selection_implication':'When native count+80+24*209 reaches5, ordinary-record selection can choose ordinal467 at+84+24*209. Existing mode gate, equipment accumulation and actual effect remain necessary.',
   'limits':['File path/DTI pairing at1B786C3..1B786D8 observed separately; singleton5012048 population still to bind.',
     'Do not directly write selected record467 or bypass accumulation.',
     'No complete armor-to-skill or Shield-Weaver effect/lifecycle claim.',
     'Static analysis only; no native execution or game-directory writes.']}
 (P/'research/horizon-other-skill-record-reader.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('SkillData native reader/copy verified: record467 skill209 threshold5, no deferred prerequisites; effects pending.')
if __name__=='__main__':main()
