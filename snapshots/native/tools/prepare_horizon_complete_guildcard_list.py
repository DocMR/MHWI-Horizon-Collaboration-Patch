"""Adapt only eleven Horizon GCOD list flags; preserve every other PC byte."""
from pathlib import Path
import hashlib,json,struct
P=Path(__file__).resolve().parents[1]
OUT=P/'staging/Horizon-complete-guildcard-list-repair-20261022/nativePC'
def main():
 report=[]
 for filename,ids in [('gc_data_title1.gcod',list(range(782,787))+list(range(1500,1505))),('gc_data_pose.gcod',[13])]:
  src=P/'MHWI解包/chunk/common/guildcard'/filename
  raw=src.read_bytes();count=struct.unpack_from('<I',raw,6)[0];assert len(raw)==10+count*20
  converted=bytearray(raw);offsets=[]
  for identity in ids:
   offset=10+(identity-1)*20
   assert struct.unpack_from('<5I',raw,offset)==(identity,12,10 if identity>=1500 else 1,0,2)
   struct.pack_into('<I',converted,offset+16,0);offsets.append(offset+16)
  changed=[n for n,(a,b) in enumerate(zip(raw,converted)) if a!=b]
  assert changed==offsets
  for identity in range(1,count+1):
   old=struct.unpack_from('<5I',raw,10+(identity-1)*20);new=struct.unpack_from('<5I',converted,10+(identity-1)*20)
   assert (new==old) if identity not in ids else (new==old[:4]+(0,))
  target=OUT/'common/guildcard'/filename;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(converted)
  report.append({'path':target.relative_to(OUT).as_posix(),'count':count,'ids':ids,'changed_byte_offsets':changed,'base_sha256':hashlib.sha256(raw).hexdigest(),'sha256':hashlib.sha256(converted).hexdigest()})
 (P/'research/horizon-complete-c2-live-failures-20261021/guildcard-list-resource-repair.json').write_text(json.dumps({'files':report,'native_consumer':'60FA8D requires runtime field18==0 before checking unlock bits','save_modified':False,'installed':False},indent=2),encoding='utf-8')
 print('PASS eleven exact platform-list fields adapted; all other PC bytes unchanged; not installed')
if __name__=='__main__':main()
