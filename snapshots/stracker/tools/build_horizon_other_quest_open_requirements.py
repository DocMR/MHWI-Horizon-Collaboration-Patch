"""Record source rank floors and externally verified Firebreak prerequisite."""
import json
from pathlib import Path
P=Path(__file__).resolve().parents[1]
quests=json.loads((P/'research/horizon-other-quest-classification.json').read_text(encoding='utf-8'))['quests']
report={'schema_version':1,'module':'horizon-other',
 'requirements':[{'quest_id':q['quest_id'],'condition_a':q['source_words_52_54'][0],
                 'condition_b':q['source_words_52_54'][1],
                 'required_clear':67803 if q['quest_id']==67804 else None} for q in quests],
 'prerequisite_source':{'url':'https://game.capcom.com/world/us/schedule-master.html?utc=-5',
  'publisher':'Capcom','accessed':'2026-10-15',
  'short_excerpt':'Available after completing the event quest "The Survivor".',
  'access_note':'Official search-index excerpt; direct unparameterized page returned403. Spanish/Italian/German official index excerpts corroborate.'},
 'implementation':'src/horizon_rank_requirements.cpp: source_quest_open_requirements',
 'installation_allowed':False,
 'limits':['Requirements-met is not complete native admission; preserve remaining native checks.',
           'Completion fact must come from real per-character completion, not SOS state or synthetic grants.',
           'No extra Frozen-Wilds-to-Survivor prerequisite inferred.',
           'Native fact providers, open-state lifecycle and final bootstrap remain to connect.']}
for target in (P/'research/horizon-other-quest-open-requirements.json',P/'staging/Horizon-other-module/merge/quest-open-requirements.json'):
 target.write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'quests':len(quests),'explicit_clear_prerequisites':1,'installation_allowed':False}))
