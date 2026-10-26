"""Compare six source goal/stage IDs with retained official PC quest records."""
from pathlib import Path
import sys,json,struct,hashlib
P=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(P/'.runtime/python'))
from cryptography.hazmat.decrepit.ciphers.algorithms import Blowfish
from cryptography.hazmat.primitives.ciphers import Cipher,modes
from build_horizon_other_text_candidate import parse

def swap(data):return b''.join(data[i:i+4][::-1] for i in range(0,len(data),4))
def main():
 cipher=Cipher(Blowfish(b'TZNgJfzyD2WKiuV4SglmI6oN5jP2hhRJcBwzUooyfIUTM4ptDYGjuRTP'),modes.ECB())
 pc=[]
 for path in sorted((P/'MHWI解包/chunk/quest').glob('questData_*.mib')):
  raw=path.read_bytes()
  if len(raw)%8:raise ValueError(path)
  decrypt=cipher.decryptor();plain=swap(decrypt.update(swap(raw))+decrypt.finalize())
  assert struct.unpack_from('<I',plain)[0]==0x18091001,path
  q=struct.unpack_from('<I',plain,10)[0]
  text=P/f'MHWI解包/chunk/common/text/quest/q{q:05d}_eng.gmd'
  if not text.exists():continue
  _,values,_=parse(text.read_bytes())
  pc.append({'quest_id':q,'stage':struct.unpack_from('<I',plain,0x1b)[0],
   'goal':struct.unpack_from('<IHH',plain,0x57),'title':values[0].decode('utf-8'),
   'objective':values[1].decode('utf-8'),'mib_sha256':hashlib.sha256(raw).hexdigest(),
   'text_sha256':hashlib.sha256(text.read_bytes()).hexdigest()})
 rows=[]
 for q in [67101,67102,67607,67802,67803,67804]:
  plain=(P/f'research/horizon-other-quest-source/decoded/questData_{q}.bin').read_bytes()
  goal=struct.unpack_from('<IHH',plain,0x57);stage=struct.unpack_from('<I',plain,0x1b)[0]
  matches=[r for r in pc if r['goal'][:2]==goal[:2] and r['stage']==stage]
  rows.append({'quest_id':q,'source_stage_raw':stage,'source_goal_raw':goal,'same_goal_type_target_and_stage_pc_references':matches[:4],
   'all_match_count':len(matches),
   'same_type_target_other_stage':[r for r in pc if r['goal'][:2]==goal[:2] and r['stage']!=stage][:4],
   'same_target_different_flags':[r for r in pc if r['goal'][1]==goal[1] and r['goal'][0]!=goal[0]][:3]})
 report={'status':'pc_goal_stage_reference_comparison','pc_quest_records_with_english_text':len(pc),'quests':rows,
 'limits':['Matching numeric fields plus PC objective text supports native enum reuse; does not prove every spawn resource or quest scene loads.',
 'Goal counts intentionally remain PS source counts; comparison does not replace multi-target or small-monster configuration.',
 'Stage offset1B is recorded as a serialized raw field here; matching PC records are corroboration, not a new full stage-loader proof.'],
 'game_modified':False,'ps_archives_accessed':False}
 (P/'research/horizon-complete-pc-quest-reference-20261021.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print(json.dumps({'pc_quests':len(pc),'matches':[{ 'quest_id':r['quest_id'],'count':r['all_match_count'],'examples':[(v['quest_id'],v['objective']) for v in r['same_goal_type_target_and_stage_pc_references'][:2]],'other_stage':[(v['quest_id'],v['stage'],v['objective']) for v in r['same_type_target_other_stage']], 'other_flags':[(v['quest_id'],v['goal'],v['objective']) for v in r['same_target_different_flags']]} for r in rows]}))
if __name__=='__main__':main()
