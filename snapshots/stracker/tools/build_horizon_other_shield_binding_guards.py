"""Extract the equipment endpoint subset of existing shared native byte guards."""
import hashlib
import json
import re
from analyze_hzd_code import CodeImage, P

def main():
    im=CodeImage()
    source=P/'runtime/src/generated_stormbolt_native_code_guard.h'
    text=source.read_text(encoding='utf-8')
    endpoints=[0x1922190,0x19A2D20,0x1F604B0,0x1F628A0,0xCCC620,0x130C710,
               0x2171490,0x222F910,0x23D8180,0x23D82A0,0x2300260,0x1B42010]
    spans={int(rva,16):name for rva,name in re.findall(
        r'\{0x([0-9A-Fa-f]+), (\w+), sizeof\(\w+\)\}',text)}
    records=[]
    lines=['// Generated equipment-only subset; shared source remains unchanged.',
           'constexpr TreeOwnerSpec kShieldBindingOwners[] = {']
    for rva in endpoints:
        name=spans[rva]
        match=re.search(r'unsigned char '+re.escape(name)+r'\[\] = \{(.*?)\};',text,re.S)
        assert match,name
        raw=bytes(int(x,16) for x in re.findall(r'0x([0-9a-fA-F]{2})',match[1]))
        assert 0<len(raw)<=16384 and all(im.valid[rva-im.base:rva-im.base+len(raw)])
        assert raw==im.code[rva-im.base:rva-im.base+len(raw)]
        body=list(im.md.disasm(raw,rva))
        assert sum(i.size for i in body)==len(raw)
        digest=hashlib.sha256(raw).digest()
        lines.append('    {'+f'0x{rva:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
        records.append({'rva':hex(rva),'shared_span':name,'bytes':len(raw),
            'sha256':digest.hex(),'instructions':len(body)})
    lines+=['};','']
    (P/'staging/Horizon-other-module/src/generated_shield_binding_owners.inc').write_text('\n'.join(lines),encoding='utf-8')
    (P/'research/horizon-other-shield-binding-guards.json').write_text(json.dumps({
        'status':'equipment_endpoint_guard_subset_matches_retained_native_code',
        'exe_sha256':im.meta['exe_sha256'],'shared_source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
        'endpoints':records,'limits':['Reuses existing shared endpoint-span boundaries; byte comparison and complete decoding do not establish all callees or resource readiness.',
        'Factory emits typed addresses only after host executable certification and native guard verification; no runtime calls were made here.',
        'No shooting/session/class48 hooks imported; no game-directory writes.']},indent=2),encoding='utf-8')
    print(f'{len(records)} equipment-only native endpoint guards extracted and statically checked.')

if __name__=='__main__':main()
