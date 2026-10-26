"""Certify condition display consumers and keep admission status unproven."""
import json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();rows=[]
for root,address,expected in [(0x10D1E60,0x10D2330,'0fb74d4a'),(0x10F5530,0x10F5AD6,'440fb7684a'),(0x1154DC0,0x1154ED9,'0fb74e4a')]:
 fs,xs=decode_group(im,root);by={i.address:i for i in xs};assert by[address].bytes.hex()==expected
 rows.append({'root':hex(root),'condition_read':im.format(by[address]),'label_calls':[im.format(i) for i in xs if i.mnemonic=='call' and i.operands[0].imm==0x193AD50]})
_,xs=decode_group(im,0x1976320);by={i.address:i for i in xs}
for a,raw in {0x1976349:'e862eeffff',0x197634E:'83f802',0x1976382:'8b9f64030000',0x1976388:'83c0fb',0x197638B:'83f801'}.items():assert by[a].bytes.hex()==raw
report={'exe_sha256':im.meta['exe_sha256'],'condition_display_paths':rows,
 'nearby_property_accessor':{'rva':'0x1976320','instructions':[im.format(i) for i in xs],
 'observed_behavior':'Categories 3 and 1 both use the same row+364 property branch; category2 and categories5/6 have alternate handling.',
 'semantic_status':'Property identity and complete admission semantics unproven. Do not replace this helper with rank-floor checks.'},
 'limits':['UI text flow is not a complete native admission predicate.','The 131 explicit +364 writer candidates include unrelated object types; field-offset coincidence is not identity.','No target code execution, game launch or save modification.']}
(P/'research/horizon-other-rank-ui-flow.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'condition_ui_paths':len(rows),'category_3_to_1_property_branch_preserved':True,'full_admission_proven':False}))
