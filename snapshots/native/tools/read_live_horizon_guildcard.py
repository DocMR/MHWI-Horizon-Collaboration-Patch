"""Read current guildcard tables/open bits/quest registration; no writes or calls."""
import ctypes as c,hashlib,json,struct,time
from pathlib import Path
from trace_live_quest_readonly import k,find_game,modules
P=Path(__file__).resolve().parents[1]
def main():
 pid=find_game()
 if pid is None:raise RuntimeError('Game not running')
 game=next(x for x in modules(pid) if x['name'].lower()=='monsterhunterworld.exe');base=game['base']
 assert hashlib.sha256(Path(game['path']).read_bytes()).hexdigest()==json.loads((P/'research/exported-text-metadata.json').read_text())['exe_sha256']
 h=k.OpenProcess(0x1010,False,pid)
 if not h:raise c.WinError(c.get_last_error())
 def read(a,n):
  b=c.create_string_buffer(n);got=c.c_size_t()
  if not k.ReadProcessMemory(h,a,b,n,c.byref(got)) or got.value!=n:raise c.WinError(c.get_last_error())
  return b.raw
 def q(a):return struct.unpack('<Q',read(a,8))[0]
 def u(a):return struct.unpack('<I',read(a,4))[0]
 try:
  manager=q(base+0x5013950);header=read(manager+0xa0,16);index=struct.unpack_from('<i',header)[0];storage=struct.unpack_from('<Q',header,8)[0]
  assert 0<=index<3 and storage
  character=storage+index*0x26cc00;rows=[]
  for pose,slot,ids in [(False,0x5012a28,list(range(782,787))+list(range(1500,1505))),(True,0x5012a40,[13])]:
   resource=q(base+slot)
   for identity in ids:
    item={'pose':pose,'id':identity,'resource':hex(resource)}
    if resource:
     count=u(resource+0xb8);item['count']=count
     if identity-1<count:
      entry=q(q(resource+0xc8)+(identity-1)*8);item['entry']=hex(entry)
      if entry:item['fields']={hex(o):u(entry+o) for o in [8,12,16,20,24]}
    bit=identity-1;offset=0x16de68 if pose else 0x16dbe0
    item['open']=bool(u(character+offset+(bit//32)*4)&(1<<(bit%32)));rows.append(item)
  quest=q(base+0x500ed30);count=q(quest+0xcea8);assert count<=1024
  ids=[]
  for n in range(count):
   resource=q(quest+0xaea8+n*8)
   if resource:ids.append(u(resource+0xe0))
  assert read(manager+0xa0,16)==header
  report={'pid':pid,'rows':rows,'registered_quest_count':count,'horizon_registered':[i for i in ids if i in [67101,67102,67607,67802,67803,67804]],'memory_writes':False,'remote_calls':False,'save_file_access':False}
  out=P/'research'/('horizon-guildcard-live-'+time.strftime('%Y%m%d-%H%M%S')+f'-{pid}.json');out.write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps({'path':str(out),**report}))
 finally:k.CloseHandle(h)
if __name__=='__main__':main()
