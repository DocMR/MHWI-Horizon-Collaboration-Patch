"""Certify mapping expansion side effects from original static instructions."""
import json,struct
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage()
_,init=decode_group(im,0x1358310);by={i.address:i for i in init}
expected={0x13583D7:'3bf8',0x13583D9:'0f84f3000000',0x13583E1:'8b448c20',0x13583E5:'83f8ff',0x135842D:'0fabd0',0x1358430:'898429b04c1000',0x13584D8:'83ffff',0x13584DB:'0f85effeffff'}
for address,raw in expected.items():assert by[address].bytes.hex()==raw
sources=(0x3250260,0x3250270,0x3250280,0x3250290,0x32502E0,0x32502A0,0x32502B0,0x32502C0,0x32502D0,0x32502F0)
# Verify each literal block is actually loaded by the original function.
assert set(sources).issubset({im.target(i) for i in init})
values=[]
for source in sources:values.extend(struct.unpack_from('<4i',im.exe,im.raw(source)))
assert values[0]==1 and -1 in values
exceptions=values[:values.index(-1)]
quests=(67101,67102,67607,67802,67803,67804)
assert not set(quests)&set(exceptions)
_,snapshot=decode_group(im,0x1368040)
clears=[i for i in snapshot if i.mnemonic=='mov' and i.op_str.startswith('qword ptr [r14 + 0x104b') and i.op_str.endswith(', rax')]
assert len(clears)==8
assert {i.operands[0].mem.disp for i in clears}==set(range(0x104B88,0x104BC8,8))
assert any(i.address==0x136812A and i.bytes.hex()=='41c686c84b100001' for i in snapshot)
# The six appended indices must fit the actual character stream payload,
# not merely a runtime buffer inferred from neighboring fields.
streams=[]
for root,seed,copy_count,dec,loop,end,slot in (
 (0x1361D90,0x1362D10,0x1362D1D,0x1362D33,0x1362D37,0x1362DB4,'0x70'),
 (0x136BEF0,0x136CDB4,0x136CDC1,0x136CDE3,0x136CDE7,0x136CE64,'0x60')):
 _,body=decode_group(im,root);by_address={i.address:i for i in body}
 assert by_address[seed].op_str=='r13d, 5'
 assert by_address[copy_count].op_str=='ebp, r13d'
 assert by_address[dec].op_str=='rbp, 1'
 assert by_address[loop].mnemonic=='jne'
 assert by_address[loop].operands[0].imm==dec-0x13
 after=[i for i in body if loop<i.address<end]
 assert not any(any(i.reg_name(r) in ('rbp','ebp','bp','bpl') for r in i.regs_access()[1]) for i in after)
 # There is no alternate branch into this post-loop stream block.
 assert not any(i.mnemonic.startswith('j') and i.operands and i.operands[0].type==2 and
                loop<i.operands[0].imm<end for i in body)
 for field in (0x104BD8,0x104C20):
  n=next(n for n,i in enumerate(body) if i.mnemonic=='lea' and
         any(op.type==3 and op.mem.disp==field for op in i.operands))
  block=body[n:n+5]
  assert any(i.op_str=='r8d, [rbp + 0x40]' for i in block)
  assert any(i.mnemonic=='call' and i.op_str==f'qword ptr [rax + {slot}]' for i in block)
  streams.append({'root':hex(root),'field':hex(field),'bytes':64,'slot':slot,
                  'instructions':[im.format(i) for i in block]})
assert 352+len(quests)<=64*8
report={'exe_sha256':im.meta['exe_sha256'],'maintenance':{'root':'0x1358310','exception_ids':exceptions,'target_exceptions':[],
 'state_field':'0x104CB0','effect_on_extended_targets':'All six are outside the exception list and take the bit-set path when enumerated by the extended mapping. This does not certify what the bit means.',
 'certified_instructions':[im.format(by[a]) for a in expected]},
 'character_streams':{'open_field':'0x104bd8','clear_field':'0x104c20','bytes_each':64,'bits_each':512,'appended_index_range':[352,357],'fits_original_payload':True,'blocks':streams,'limit':'Static buffer/stream linkage only; no actual save execution or file acceptance.'},
 'completion_snapshot':{'root':'0x1368040','bitmap':'0x104B88','bytes':64,'valid_flag':'0x104BC8','clear_instructions':[im.format(i) for i in clears],
 'meaning':'Rebuilt from original completion/category tests; not an additional independently proven serialized bitmap.'},
 'integration_constraints':['Do not interpret 104CB0 as user progress without tracing its consumers and producers.','Mapping replacement must preserve the original lifecycle behavior or deliberately adapt certified differences.','No mapping installation or save write is authorized by this report.'], 'dynamic_tests_executed':False}
(P/'research/horizon-other-mapping-lifecycle.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'maintenance_exception_ids':len(exceptions),'targets_outside_exceptions':6,'snapshot_bytes':64}))
