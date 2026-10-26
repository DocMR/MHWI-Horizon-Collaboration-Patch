"""Read original GUI header/objects/initial properties, without changing assets.
Format reference: https://github.com/Fexty12573/mhw-gui-editor (GUIFile/GUITypes/GUIObject/GUIInitParam).
"""
import ctypes as c,re,struct,json
from pathlib import Path
P=Path(__file__).resolve().parents[1]
source=(P/'research/mhw-gui-editor-reference/GUITypes.h').read_text();body=source.split('struct GUIHeader {',1)[1].split('};',1)[0]
fields=[]
for line in body.splitlines():
 line=line.split('//')[0].strip();m=re.match(r'(char|u32|u64|time_t) (\w+)(?:\[(\d+)\])?;',line)
 if m:
  typ,name,count=m.groups();ct={'char':c.c_char,'u32':c.c_uint32,'u64':c.c_uint64,'time_t':c.c_int64}[typ];fields.append((name,ct*int(count) if count else ct))
class Header(c.Structure):_fields_=fields
class GUI:
 def __init__(self,path):
  self.b=Path(path).read_bytes();self.h=Header.from_buffer_copy(self.b);assert self.h.fileType==b'GUI' and self.h.guiVersion==144150 and self.h.fileSize==len(self.b)
 def string(self,offset):
  a=self.h.stringOffset+offset;assert 0<=a<len(self.b);return self.b[a:self.b.index(b'\0',a)].decode('utf-8')
 def params(self,index,count):
  out=[]
  for n in range(index,index+count):
   a=self.h.initParamOffset+n*32;typ,use=struct.unpack_from('<2B',self.b,a);name=self.string(struct.unpack_from('<I',self.b,a+16)[0]);v=struct.unpack_from('<I',self.b,a+24)[0]
   value=v
   if typ==4:value=list(struct.unpack_from('<4f',self.b,self.h.keyValue128Offset+v))
   elif typ in [2,15]:value=struct.unpack_from('<f',self.b,self.h.keyValue32Offset+v)[0]
   elif typ==3:value=bool(self.b[self.h.keyValue8Offset+v])
   elif typ==6:value=self.string(struct.unpack_from('<Q',self.b,self.h.keyValue32Offset+v)[0])
   elif typ not in [17,18]:value=struct.unpack_from('<I',self.b,self.h.keyValue32Offset+v)[0]
   out.append(dict(name=name,type=typ,use=use,value=value))
  return out
 def objects(self):
  out=[]
  for n in range(self.h.objectNum):
   a=self.h.objectOffset+n*56;identity,num,animated,padding,next_,child,name,typ,params,sequence,extend=struct.unpack_from('<IBBHiiQQQQq',self.b,a)
   out.append(dict(index=n,id=identity,name=self.string(name),type=typ,child=child,next=next_,params=self.params(params,num)))
  return out
if __name__=='__main__':
 import sys
 for arg in sys.argv[1:]:
  g=GUI(arg);print(arg,'objects',g.h.objectNum,'instances',g.h.instanceNum);print(json.dumps(g.objects(),ensure_ascii=False,indent=2))

