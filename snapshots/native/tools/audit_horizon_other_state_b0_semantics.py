"""Certify observable state-bit branches without inventing a progress meaning."""
import json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage()
report={'exe_sha256':im.meta['exe_sha256'],'field':'0x104CB0','paths':[],'semantic_name':'unresolved; do not call player progress'}
for root, checks in [(0xB3C180,{0xB3C232:'e8a92d8100',0xB3C25C:'e81fd48200',0xB3C283:'e8582d8100',0xB3C28D:'403af0',0xB3C295:'7455'}),
 (0x1A82BB0,{0x1A82D2C:'e8afc28cff',0x1A82D56:'e825698eff',0x1A82D7D:'e85ec28cff',0x1A82D87:'403af0',0x1A82D8F:'7455'})]:
 fs,xs=decode_group(im,root);by={i.address:i for i in xs}
 for address,raw in checks.items():assert by[address].bytes.hex()==raw
 report['paths'].append({'root':hex(root),'observed_behavior':'Read current quest bit, set through original writer, read again, and branch on before/after change. Further category/network side effects are conditional.', 'instructions':[im.format(by[a]) for a in checks]})
_,xs=decode_group(im,0x193AFE0);by={i.address:i for i in xs}
for address,raw in {0x193AFE6:'e8c5a10300',0x193AFEB:'85c0',0x193AFEF:'83f807',0x193AFF4:'32c0',0x193AFFB:'b001'}.items():assert by[address].bytes.hex()==raw
report['category_predicate']={'rva':'0x193AFE0','true_categories':[0,7],'all_six_source_category':3,'proposed_optional_category':1,'result_for_six_before_and_after':False,'instructions':[im.format(i) for i in xs]}
registration=0x30BEFB0
assert im.pointer(registration)==0x30C0AE8
assert im.pointer(registration+0x18)==0xB3C180
offset=im.raw(0x30C0AE8)
assert im.exe[offset:offset+len(b'LateJoinEnable_qt\0')]==b'LateJoinEnable_qt\0'
report['named_native_action']={'record':'0x30BEFB0','name_rva':'0x30C0AE8','name':'LateJoinEnable_qt','handler_slot':'0x30BEFC8','handler':'0xB3C180','meaning':'The bit read/set/read path belongs to the named native late-join-enable action; this is not a quest completion getter.'}
report['limits']=['Category predicate equivalence covers only this helper, not all category consumers.','Before/after observation does not establish first-clear, reward, cinematics or player-progress semantics.','Direct caller inventory retains 29 validated edges, with indirect dispatch not covered.','No game code, surrogate mechanism or save writes executed.']
(P/'research/horizon-other-state-b0-semantics.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'before_after_paths':len(report['paths']),'category_predicate_equivalent_for_six':True,'semantic_name_proven':False}))
