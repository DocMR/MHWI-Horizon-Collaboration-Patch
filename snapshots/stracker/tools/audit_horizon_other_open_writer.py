"""Certify the native per-character quest-open writer for deferred integration."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group
im=CodeImage();fs,xs=decode_group(im,0x13697A0);by={i.address:i for i in xs}
for a,raw in {0x13697C9:'4c8d3df062ee01',0x136980B:'440fabc0',0x136980F:'8981d84b1000'}.items():assert by[a].bytes.hex()==raw
assert xs[-1].mnemonic=='jmp' and im.target(xs[-1])==0x2E13008
end=xs[-1].address+xs[-1].size
raw=im.code[0x13697A0-im.base:end-im.base];assert all(im.valid[0x13697A0-im.base:end-im.base])
digest=hashlib.sha256(raw).digest()
module=P/'staging/Horizon-other-module'
(module/'src/generated_quest_open_owner.inc').write_text('// Generated native open writer identity.\nconstexpr TreeOwnerSpec kNativeOpenOwner = {'+f'0x13697A0, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}};\n',encoding='utf-8')
_,getter=decode_group(im,0x134F070)
getter_raw=im.code[getter[0].address-im.base:getter[-1].address+getter[-1].size-im.base]
assert all(im.valid[getter[0].address-im.base:getter[-1].address+getter[-1].size-im.base])
getter_hash=hashlib.sha256(getter_raw).digest()
(module/'src/generated_quest_open_getter_owner.inc').write_text('// Generated native open getter identity.\nconstexpr TreeOwnerSpec kNativeOpenGetterOwner = {'+f'0x134F070, {len(getter_raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in getter_hash)+'}};\n',encoding='utf-8')
report={'exe_sha256':im.meta['exe_sha256'],'rva':'0x13697A0','bytes':len(raw),'sha256':digest.hex(),
 'query_hook':{'rva':'0x134F070','bytes':len(getter_raw),'sha256':getter_hash.hex(),
               'rule':'Only six catalog IDs refresh known open requirements, then call original getter.',
               'implementation':'src/horizon_quest_open_hook.cpp'},
 'instructions':[im.format(i) for i in xs],'implementation':'src/horizon_quest_open_maintenance.cpp',
 'installation_allowed':False,'limits':['Bind original byte identity before mapping edits.','Refresh must follow real native load/rank/completion updates with character lifetime held.','Does not replace other native admission checks or mark completion.','Implementation compiled only; not executed, not deployed.']}
for target in (P/'research/horizon-other-open-writer.json',module/'merge/quest-open-maintenance.json'):
 target.write_text(json.dumps(report,indent=2),encoding='utf-8')
print(json.dumps({'writer_bytes':len(raw),'installation_allowed':False}))
