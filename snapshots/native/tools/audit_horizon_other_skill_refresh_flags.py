"""Static native skill refresh registration, dirty-bit writer and bounded UI caller."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
 im=CodeImage()
 def span(a,z):
  raw=im.code[a-im.base:z-im.base];assert 0 not in im.valid[a-im.base:z-im.base]
  b=list(im.md.disasm(raw,a));assert sum(i.size for i in b)==z-a
  return {i.address:i for i in b},{'range':[hex(a),hex(z)],'sha256':hashlib.sha256(raw).hexdigest(),'instructions':[im.format(i) for i in b]}
 w,writer=span(0x1258BE4,0x1258C08)
 assert w[0x1258BEE].op_str=='eax, 2' and w[0x1258BF2].op_str=='dword ptr [rcx + 0x1261c], eax'
 assert w[0x1258BFE].op_str=='eax, 2' and w[0x1258C02].op_str=='dword ptr [rcx + 0x12630], eax'
 c,caller=span(0x11A418D,0x11A41B7)
 assert c[0x11A419D].op_str=='edx, byte ptr [rdi + 0x501e]'
 assert c[0x11A41A4].op_str=='rcx, qword ptr [rcx + 0x126c8]' and c[0x11A41AB].op_str=='0x12ecdf0'
 f,b=decode_group(im,0x12F8150);q={i.address:i for i in b}
 assert q[0x12F816C].op_str=='edx, dword ptr [rax + 0x1261c]' and q[0x12F8172].op_str=='edx, 2'
 assert q[0x12F8178].op_str=='dl, byte ptr [rcx + 0x68]'
 assert im.pointer(0x324B1F0+0x30)==0x12F8150
 f2,ctor=decode_group(im,0x12F7CE0);d={i.address:i for i in ctor}
 assert im.target(d[0x12F7CEE])==0x324B1F0 and d[0x12F7CFA].op_str=='qword ptr [rbx], rax'
 groups={}; owners={}
 for entry in [0x1258800,0x1258710,0x2041700]:
  fragments,items=decode_group(im,entry);assert items
  owners[entry]={i.address:i for i in items}
  groups[hex(entry)]={'fragments':[list(v) for v in fragments],
      'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in items)).hexdigest(),
      'instructions':[im.format(i) for i in items]}
 o=owners[0x1258800]
 assert o[0x1258840].op_str=='0x1258c08'
 assert o[0x1258BD5].op_str=='dword ptr [rbx + 0x2ac], 0'
 assert o[0x1258BE2].op_str=='0x1258c08'
 assert o[0x12588D0].op_str=='0x12f1fa0'
 assert owners[0x1258710][0x1258739].op_str=='0x1258800'
 assert owners[0x2041700][0x2041860].op_str=='0x1258710'
 report={'status' :'skill_refresh_callback_and_dirty_bit_verified_full_equip_sequence_pending',
  'exe_sha256':im.meta['exe_sha256'],'writer_owner_groups':groups,'dirty_writer':writer,'cache_update_caller':caller,
  'callback':{'vtable':'0x324b1f0','slot':'0x30','target':'0x12f8150',
    'constructor':[im.format(i) for i in ctor],'body':[im.format(i) for i in b]},
  'dirty_contract':'1258BE4..1258C08 loads owner fromcontroller+8, setsbit2 inowner1261C/12630.12F8150 checks owner1261C bit2 OR controller68 after enabled flag+5C bit1.',
  'cache_caller_contract':'11A41AB passes delayed/requested byte+501E to actor126C8 component12ECDF0 under prior float comparison; clears request byte after return.',
  'limits':['Writer belongs to full227-instruction1258800. After initial1B42300 returnsnonnull and native processing, setsbit2 only when accumulated pending maskcontroller2AC==0. Caller2041700 has path1258710->1258800; conditions/other paths retained.',
   'Cache update2041700 setsbit1; it must not be conflated with skill refreshbit2.',
   'A conditional synchronous update path is now proven, not every equipment change. Initial lookup null and nonzero pending mask defer/skip thisbit2 write; pending completion and frame callback scheduling remain unresolved.',
   'No native execution or game-directory writes; Shield-Weaver effects remain incomplete.']}
 (P/'research/horizon-other-skill-refresh-flags.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print('Full227-instruction writer owner and conditional2041700->1258710->1258800 dirtybit2 path verified; pending completion/scheduling unresolved.')
if __name__=='__main__':main()
