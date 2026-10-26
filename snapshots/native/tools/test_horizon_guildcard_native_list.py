"""Execute authentic first-pass list instructions against all PC GCOD rows, isolated."""
import ctypes as c,struct,json
from pathlib import Path
from analyze_hzd_code import CodeImage,P
im=CodeImage();start,end=0x60fa83,0x60faa6
assert 0 not in im.valid[start-im.base:end-im.base]
original=im.code[start-im.base:end-im.base]
xs=list(im.md.disasm(original,start));assert sum(i.size for i in xs)==len(original)
k=c.WinDLL('kernel32',use_last_error=True);k.VirtualAlloc.argtypes=[c.c_void_p,c.c_size_t,c.c_uint32,c.c_uint32];k.VirtualAlloc.restype=c.c_void_p
k.VirtualFree.argtypes=[c.c_void_p,c.c_size_t,c.c_uint32]
size=0x610000;image=k.VirtualAlloc(None,size,0x3000,0x40);assert image
try:
 c.memmove(image+start,original,len(original))
 # Preserve native nonvolatile registers and enter with the original zero ESI/EBP.
 head=bytes.fromhex('56 55 41 54 33 f6 33 ed 48 8b c1 e9')
 thunk=0x60fa00;head+=struct.pack('<i',start-(thunk+len(head)+4));c.memmove(image+thunk,head,len(head))
 for off,code in [(end,bytes.fromhex('44 89 e0 41 5c 5d 5e c3')),(0x60fb66,bytes.fromhex('33 c0 41 5c 5d 5e c3'))]:c.memmove(image+off,code,len(code))
 run=c.CFUNCTYPE(c.c_uint32,c.c_void_p)(image+thunk);changes=[];tested=0
 for name,ids in [('gc_data_title1.gcod',list(range(782,787))+list(range(1500,1505))),('gc_data_pose.gcod',[13])]:
  old=(P/'MHWI解包/chunk/common/guildcard'/name).read_bytes();new=(P/'staging/Horizon-complete-guildcard-list-repair-20261022/nativePC/common/guildcard'/name).read_bytes()
  count=struct.unpack_from('<I',old,6)[0]
  for index in range(count):
   results=[]
   for raw in [old,new]:
    identity,condition,group,param,platform=struct.unpack_from('<5I',raw,10+index*20)
    native=c.create_string_buffer(32);struct.pack_into('<5I',native,8,condition,group,param,identity,platform)
    value=run(c.addressof(native));assert value==(identity if condition!=0xffffffff and platform==0 else 0);results.append(value)
   if results[0]!=results[1]:changes.append((name,index+1));assert index+1 in ids and results==[0,index+1]
   tested+=1
 assert len(changes)==11
 out={'status':'passed_authentic_native_first_pass','rows':tested,'changed_entries':changes,'memory_scope':'isolated VirtualAlloc in this test process only','game_modified':False,'limits':['Native lookup/loading and GUI rendering are not executed; live selection/save remain required.']}
 (P/'research/horizon-complete-c2-live-failures-20261021/guildcard-native-list-fixture.json').write_text(json.dumps(out,indent=2),encoding='utf-8')
 print('PASS authentic list filter: 1660 rows, only ten titles and one pose become eligible; all other outcomes unchanged')
finally:assert k.VirtualFree(image,0,0x8000)
