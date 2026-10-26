"""Bind result-consumer calls to native UI phases and outgoing transitions."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P


def main():
    im=CodeImage()
    def window(a,z):
        raw=im.code[a-im.base:z-im.base]
        assert len(raw)==z-a and all(im.valid[a-im.base:z-im.base])
        rows=list(im.md.disasm(raw,a))
        assert sum(i.size for i in rows)==len(raw)
        return rows
    dispatch=window(0x1EB639B,0x1EB6406)
    d={i.address:i for i in dispatch}
    assert d[0x1EB63A4].op_str=='eax, dword ptr [rdi + 0x28f0]'
    assert d[0x1EB63AA].op_str=='eax, 0x3c'
    assert d[0x1EB63FA].op_str=='eax, dword ptr [rdx + rax*4 + 0x1eb8e5c]'
    tables=[]
    for address in (0x1EB8E5C,0x1EAD82C):
        raw=im.code[address-im.base:address-im.base+61*4]
        assert len(raw)==244 and all(im.valid[address-im.base:address-im.base+244])
        tables.append(struct.unpack('<61I',raw))
    consumer,advance=tables
    assert [n for n,t in enumerate(consumer) if t==0x1EB64FF]==[59]
    assert [n for n,t in enumerate(consumer) if t==0x1EB6FD2]==[11]
    cases=[window(0x1EB64FF,0x1EB652C),window(0x1EB6FD2,0x1EB700C)]
    c={i.address:i for rows in cases for i in rows}
    for site,operand in {0x1EB650A:'0x1eb44d0',0x1EB6522:'0x1eace10',
                         0x1EB6FD5:'0x1eb44d0',0x1EB7007:'0x1eace10'}.items():
        assert c[site].op_str==operand
    prefix=window(0x1EACE10,0x1EACE54)
    p={i.address:i for i in prefix}
    assert p[0x1EACE16].op_str=='eax, dword ptr [rcx + 0x28f0]'
    assert p[0x1EACE48].op_str=='edx, dword ptr [rcx + rax*4 + 0x1ead82c]'
    assert advance[59]==0x1EACE63 and advance[11]==0x1EAD3BA
    outgoing=[window(0x1EACE63,0x1EACE72),window(0x1EAD3BA,0x1EAD411),
              window(0x1EAD740,0x1EAD777)]
    o={i.address:i for rows in outgoing for i in rows}
    for site,operand in {0x1EACE63:'dword ptr [rdi + 0x28f0], 0x3c',
        0x1EAD3D0:'0x1ead740',0x1EAD402:'dword ptr [rdi + 0x28f0], 0xc',
        0x1EAD749:'dword ptr [rdi + 0x28f0], 0x32',
        0x1EAD75F:'eax, 0x38',0x1EAD764:'ecx, 0x36',
        0x1EAD76C:'dword ptr [rdi + 0x28f0], eax'}.items():assert o[site].op_str==operand
    report={'status':'result_ui_call_phases_and_local_advance_routes_certified_not_global_once_proof',
        'exe_sha256':im.meta['exe_sha256'],'field':'consumer+28F0',
        'dispatch':[im.format(i) for i in dispatch],
        'consumer_table_rva':'0x1eb8e5c','consumer_table':[hex(t) for t in consumer],
        'advance_table_rva':'0x1ead82c','advance_table':[hex(t) for t in advance],
        'call_phases':[{'phase':59,'consumer_call':'0x1eb650a','advance_call':'0x1eb6522','outgoing':[60]},
                       {'phase':11,'consumer_call':'0x1eb6fd5','advance_call':'0x1eb7007','outgoing':[12,50,54,56]}],
        'case_code':[[im.format(i) for i in rows] for rows in cases],
        'advance_prefix':[im.format(i) for i in prefix],
        'advance_cases':[[im.format(i) for i in rows] for rows in outgoing],
        'finding':'The two native result-consumer calls are phase-specific and followed by native advancement. Preserve this native sequencing instead of adding an every-frame completion callback.',
        'limits':['Outgoing phase mapping assumes advance sees the original phase; intervening external native calls and global phase writers/re-entry still require audit.',
            'Does not prove phases11 and59 are mutually exclusive across all UI lifetimes or that the consumer is globally invoked exactly once.',
            'Result-category gameplay labels and six-quest result producer closure remain incomplete.',
            'Only static code/table reads; no target execution, dynamic test or game-directory write.']}
    (P/'research/horizon-other-quest-result-consumer-phases.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Result consumer phases11/59 and native outgoing phase routes certified; global once-only semantics remain unproved.')


if __name__=='__main__':main()
