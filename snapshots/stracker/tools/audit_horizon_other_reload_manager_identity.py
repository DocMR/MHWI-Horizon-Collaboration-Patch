"""Static native pending-state owner identity and actor update call provenance."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage();methods={};maps={}
 for a in [0x148EA1,0x12575C0,0x2038A0A]:
  f,b=decode_group(im,a);assert b
  maps[a]={i.address:i for i in b}
  methods[hex(a)]={'fragments':[list(v) for v in f],
   'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in b)).hexdigest(),
   'instructions':[im.format(i) for i in b]}
 def span(a,z):
  raw=im.code[a-im.base:z-im.base];assert 0 not in im.valid[a-im.base:z-im.base]
  b=list(im.md.disasm(raw,a));assert sum(i.size for i in b)==z-a
  return {i.address:i for i in b},{'range':[hex(a),hex(z)],'sha256':hashlib.sha256(raw).hexdigest(),'instructions':[im.format(i) for i in b]}
 r=maps[0x148EA1];assert im.target(r[0x148EA1])==0x3241DB8
 assert im.exe[im.raw(0x3241DB8):im.raw(0x3241DB8)+21]==b'cPlayerReloadManager\0'
 assert im.target(r[0x148EAC])==0x4FA0EC0 and r[0x148EB3].op_str=='r9d, 0x300'
 assert im.pointer(0x3241D60+16)==0x12574E0
 i,init=span(0x12574E0,0x12574F0);assert i[0x12574E8].op_str=='0x12575c0'
 c=maps[0x12575C0];assert im.target(c[0x12575C6])==0x3241DD0
 o,bind=span(0x203257F,0x2032594)
 assert o[0x203257F].op_str=='0x12575c0'
 assert o[0x2032589].op_str=='qword ptr [rbx + 0x12610], rax'
 assert o[0x2032590].op_str=='qword ptr [rax + 8], rbx'
 u=maps[0x2038A0A];assert u[0x2038A03].op_str=='rcx, qword ptr [rbx + 0x12610]'
 assert u[0x2038A0A].op_str=='0x1257e30'
 report={'status':'reload_manager_type_actor_binding_and_update_call_verified',
  'exe_sha256':im.meta['exe_sha256'],'name':'cPlayerReloadManager','size':768,
  'dti':'0x4fa0ec0','dti_vtable':'0x3241d60','object_vtable':'0x3241dd0',
  'initializer':init,'binding':bind,'methods':methods,
  'relationship':'Actor+12610 owns manager; manager+8 points back tosame actor. Actor native update20388D0 directly calls1257E30. Its pending completion sets actor1261C/12630 bit2 consumed by separate skill controller callback12F8150.',
  'limits':['Actor update entry scheduling and skill callback dispatcher order are not proven by this directcall alone.',
   'Class name does not establish all pending predicate semantics; original gates retained.',
   'No complete Shield-Weaver effect or all lifecycle/remote-path claim.',
   'No native execution or game-directory writes.']}
 (P/'research/horizon-other-reload-manager-identity.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('cPlayerReloadManager identity, actor12610 ownership and native update->pending completion call verified.')
if __name__=='__main__':main()
