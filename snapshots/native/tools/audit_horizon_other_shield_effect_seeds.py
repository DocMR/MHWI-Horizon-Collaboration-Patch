"""Revalidate PS skill209 consumer windows and bounded PC direct-slot candidates."""
import hashlib,json,re
from analyze_hzd_code import CodeImage,P
from analyze_ps_stormbolt_types import PsCodeImage

def main():
 ps=PsCodeImage();pc=CodeImage();seeds=[]
 for a,z,load,query,param in [(0x2C3411A,0x2C34152,0x2C3411A,0x2C34127,0x2C3414B),
                              (0x2C55B3B,0x2C55B67,0x2C55B3B,0x2C55B42,0x2C55B60)]:
  owner=ps.owner(a);assert owner and owner[0]<=a<z<=owner[1]
  raw=ps.read(a,z-a);body=list(ps.md.disasm(raw,a));assert sum(i.size for i in body)==len(raw)
  b={i.address:i for i in body}
  assert '0x12758]' in b[load].op_str and '0x141c]' in b[query].op_str
  assert b[param].mnemonic=='cmp' and 'word ptr' in b[param].op_str and '+ 0x24], 0' in b[param].op_str
  seeds.append({'range':[hex(a),hex(z)],'elf_owner':[hex(v) for v in owner],
    'sha256':hashlib.sha256(raw).hexdigest(),'instructions':[ps.format(i) for i in body],
    'contract':'PS actor+12758 ->skill selected record209 at141C; nonzero record ordinal ->global6F01250 pointer array+C8 ->record parameter24 signed positive gate.'})
 hits=[];unresolved=[];seen=set()
 for m in re.finditer(b'\x1c\x14\x00\x00',pc.code):
  address=pc.base+m.start();owner=pc.owner(address)
  if not owner:continue
  body=pc.instructions(address)
  if not body:unresolved.append(hex(address));continue
  for n,i in enumerate(body):
   if i.address<=address<i.address+i.size and '0x141c]' in i.op_str and i.address not in seen:
    seen.add(i.address)
    hits.append({'site':hex(i.address),'owner':hex(owner[0]),
       'window':[pc.format(j) for j in body[max(0,n-4):n+4]],
       'interpretation':'Indexed object1D0-stride writes or stack local; not certified skill-controller read.'})
 report={'status':'ps_skill209_consumer_seeds_verified_pc_equivalence_pending',
  'ps_sha256':hashlib.sha256(ps.data).hexdigest(),'pc_sha256':pc.meta['exe_sha256'],
  'ps_consumer_seeds':seeds,'pc_direct_displacement_candidates':hits,
  'pc_undecoded_raw_candidates':unresolved,
  'finding':'PS consumption of selected record209 and positive parameter24 gate is directly evidenced; PC skill data/accumulation alone cannot substitute for this consumption.',
  'limits':['Two bounded PS windows, not complete PS effect functions or all consumers.',
   'PC scan covers exactdisp141C only with decodable individual pdata fragment; register-index queries, propagated constants, absent pdata and unreadable groups are not covered.',
   'No match is not proof of missing PC implementation; semantic PS/PC pairing still required.',
   'Defense/reaction/depletion/recovery/lifecycle implementation remains incomplete.',
   'Static disassembly only; no target code executed or game-directory writes.']}
 (P/'research/horizon-other-shield-effect-seeds.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
 print(json.dumps({'ps_consumer_windows':len(seeds),'pc_slot_candidates':len(hits),'pc_undecoded_raw_candidates':len(unresolved),'effect_complete':False}))
if __name__=='__main__':main()
