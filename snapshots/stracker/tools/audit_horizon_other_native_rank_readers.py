"""Certify named native rank checks and emit reader identities, without execution."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
im=CodeImage()
paths=[]
for name_rva,registration_site,callback_site,callback,reader,field in [
 (0x30BC0C0,0xB37877,0xB3789E,0xB35930,0x1354AF0,0x90),
 (0x30BDFF0,0xB3B280,0xB3B2A8,0xB35A90,0x13540B0,0xD4)]:
 code={i.address:i for i in im.instructions(registration_site)}
 assert im.target(code[registration_site])==name_rva and im.target(code[callback_site])==callback
 at=im.raw(name_rva);name=im.exe[at:at+100].split(b'\0')[0].decode()
 assert name in ('cFSMQuest::checkHunterRank_Transition','cFSMQuest::checkMasterRank_Transition')
 xs=im.instructions(callback);assert xs
 assert im.target(xs[1])==0x5013950
 assert xs[2].operands[0].imm==0x1B8DBB0 and xs[-1].operands[0].imm==reader
 paths.append({'name':name,'registration_site':hex(registration_site),'callback':hex(callback),
               'reader':hex(reader),'character_field':hex(field),'callback_instructions':[im.format(i) for i in xs]})
hr={i.address:i for i in im.instructions(0x1354AF0)}
assert hr[0x1354AFE].op_str=='ecx, dword ptr [rbx + 0x90]'
assert hr[0x1354AF9].operands[0].imm==0x240720
assert im.instructions(0x240720)[0].bytes.hex()=='32c0'
assert im.instructions(0x13540B0)[0].op_str=='eax, dword ptr [rcx + 0xd4]'
lines=['// Generated certified native rank readers and mode dependency.', 'constexpr TreeOwnerSpec kNativeRankOwners[] = {']
owners=[]
for start in (0x1354AF0,0x13540B0,0x240720):
 xs=im.instructions(start);assert xs
 end=xs[-1].address+xs[-1].size
 raw=im.code[start-im.base:end-im.base]
 assert all(im.valid[start-im.base:end-im.base]) and sum(i.size for i in xs)==len(raw)
 digest=hashlib.sha256(raw).digest()
 lines.append('    {'+f'0x{start:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
 owners.append({'rva':hex(start),'bytes':len(raw),'sha256':digest.hex(),'instructions':[im.format(i) for i in xs]})
lines+=['};','']
module=P/'staging/Horizon-other-module'
(module/'src/generated_native_rank_owners.inc').write_text('\n'.join(lines),encoding='utf-8')
report={'exe_sha256':im.meta['exe_sha256'],'paths':paths,'owners':owners,
 'hr_mode_note':'Original reader contains unsigned min(HR,10) branch; certified current mode helper240720 returnsfalse. Keep native reader rather than bypassing this behavior.',
 'limits':['Named rank read provenance only; full admission remains separate.','Caller must own live character lifetime. No game process inspected or target code executed.']}
for target in (P/'research/horizon-other-native-rank-readers.json',module/'merge/native-rank-readers.json'):
 target.write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'named_rank_paths':2,'certified_owners':3,'dynamic_tests_executed':False}))
