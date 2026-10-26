"""Retain exact authenticated PC quest writers for an isolated memory fixture."""
from pathlib import Path
import hashlib,json
from analyze_hzd_code import CodeImage
P=Path(__file__).resolve().parents[1]
def main():
 c=CodeImage();rows=[]
 for rva,name in [(0x1369540,'complete'),(0x13697a0,'open')]:
  instructions=c.instructions(rva);assert instructions and instructions[0].address==rva
  end=instructions[-1].address+instructions[-1].size
  data=c.code[rva-c.base:end-c.base]
  assert all(c.valid[rva-c.base:end-c.base]) and sum(i.size for i in instructions)==len(data)
  out=P/f'research/horizon-complete-native-{name}-fixture.bin';out.write_bytes(data)
  rows.append({'rva':hex(rva),'end':hex(end),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'file':out.relative_to(P).as_posix()})
 (P/'research/horizon-complete-native-quest-writers-source.json').write_text(json.dumps({'exe_sha256':c.meta['exe_sha256'],'functions':rows,
  'fixture_boundary':'Exact writer bodies and real Windows critical-section APIs. Engine observer at5DC320 returns false in fixture; no quest-result caller, inventory or save-stream execution.'},indent=2)+'\n',encoding='utf-8')
 print('Retained exact PC writer bodies; no PS extraction or game mutation')
if __name__=='__main__':main()
