"""Certify table-specific downstream dispatch and identifier decomposition, statically."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
    im=CodeImage();entries=[0x1C01850,0x1CCA740,0x1CCA7E0,0x1C01880,0x2254100,0x2254150,0x1CCA010]
    methods={}
    bodies={}
    for entry in entries:
        if im.owner(entry): fragments,body=decode_group(im,entry)
        else: fragments,body=[],im.instructions(entry)
        assert body
        bodies[entry]={i.address:i for i in body}
        methods[hex(entry)]={'fragments':[list(f) for f in fragments], 'bounds':'CHAININFO group' if fragments else 'bounded leaf',
            'code_sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            'instructions':[im.format(i) for i in body]}
    b=bodies[0x1C01850];assert b[0x1C0186F].op_str=='qword ptr [rax + 0x2a0]'
    assert im.pointer(0x308E348+0x2A0)==0x1CCA7E0
    assert bodies[0x1CCA7E0][0x1CCA82E].op_str=='0x1c01880'
    b=bodies[0x1C01880]
    for a,s in [(0x1C01957,'rcx, qword ptr [rbx + 0x470]'),(0x1C01965,'edx, 0x1ff'),
                (0x1C01961,'r8d, 0xc'),(0x1C0196B,'r8d, 0xf'),(0x1C01975,'0x2254100')]:
        assert b[a].op_str==s
    assert bodies[0x2254100][0x225412F].op_str=='ecx, 0x1ff'
    assert bodies[0x1CCA740][0x1CCA79F].op_str=='dword ptr [rax + 8], ebp'
    assert bodies[0x1CCA740][0x1CCA7AA].op_str=='edx, dword ptr [rax + 0x18]'
    report={'status':'table_specific_resource_lookup_chain_verified_semantics_pending',
       'exe_sha256':im.meta['exe_sha256'],'methods':methods,
       'chain':['vtable308E348+290 ->1CCA6B0','1C01850 ->virtual2A0=1CCA7E0',
                '1CCA7E0 ->1C01880','1C01880: actor+470 ->2254100'],
       'identifier_decomposition':{'input':209,'low9':209&511,'bank4':(209>>12)&15,
           'formula':'low9=id&0x1FF; bank4=(id>>12)&0xF',
           'lookup':'2254100 selects bank count/pointer array, compares record+18 low9 to requested low9'},
       'side_chain':'1CCA740 uses actor+470 ->2254150(component,0), iterates resource virtual90/80, matches record+8 to input; record+18 passed actor virtual3E0=1CCA010.',
       'limits':['Specific constructor-installed vtable only; actual receiver provenance and derived overrides unresolved.',
                 'Resource component concrete type and semantic meaning of identifier remain uncertified.',
                 'This identifier decomposition is not proof of skill209 lookup or Shield-Weaver effects.',
                 'No whole-game absence claim; other candidate paths still require PS/PC semantic comparison.',
                 'No native execution or game-directory writes.']}
    (P/'research/horizon-other-shield-downstream.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Seven full instruction groups certified; table dispatch and ID decomposition verified; Shield-Weaver effect incomplete.')
if __name__=='__main__':main()
