"""Extract only quest_open.qol variants and compare to PC; no installation."""
from pathlib import Path
import json,hashlib,struct
P=Path(__file__).resolve().parents[1]
loader=P/'tools/extract_ps_stormslinger.py'
ns={'__file__':str(loader)}
exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0],ns)
quests=(67101,67102,67607,67802,67803,67804)
rows=[]; source=P/'research/horizon-other-quest-open-source';source.mkdir(exist_ok=True)
archives=json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))['archives']
pc=(P/'MHWI解包/chunk/common/story/quest_open.qol').read_bytes()
for archive in archives:
 ar=ns['Archive'](Path(archive['archive']))
 try:
  for row in ar.entries():
   if row['path'].lower()!='common/story/quest_open.qol':continue
   data=ar.read(row['offset'],row['size'])
   output=source/(ar.p.stem+'.qol');output.write_bytes(data)
   rows.append({'archive':str(ar.p),'file':str(output.relative_to(P)),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'same_as_pc':data==pc,'target_dword_occurrences':{str(q):[i for i in range(len(data)-3) if data[i:i+4]==struct.pack('<I',q)] for q in quests}})
 finally:ar.f.close()
report={'variants':rows,'pc_bytes':len(pc),'pc_sha256':hashlib.sha256(pc).hexdigest(),'pc_header_hex':pc[:32].hex(),'pc_target_dword_occurrences':{str(q):[i for i in range(len(pc)-3) if pc[i:i+4]==struct.pack('<I',q)] for q in quests},'format_status':'Header and full-byte identity only; native serialized row layout not yet certified','precedence':'Archive directory candidate order, runtime precedence not proven','installation_performed':False}
(P/'research/horizon-other-quest-open-table-sources.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps(report,indent=2))
