"""Identify the209 candidate component via native registration/construction/binding."""
import hashlib,json
from analyze_hzd_code import CodeImage,P
from pc_unwind_groups import decode_group

def main():
    im=CodeImage();out={};bodies={}
    for address in [0x20E981,0x22512A0,0x2251B40,0x1BFD760]:
        fragments,body=decode_group(im,address);assert body
        bodies[address]={i.address:i for i in body}
        out[hex(address)]={'fragments':[list(f) for f in fragments],
           'sha256':hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
           'instructions':[im.format(i) for i in body]}
    b=bodies[0x20E981]
    name=im.target(b[0x20E981]);assert im.exe[im.raw(name):im.raw(name)+16]==b'cpAnimationTree\0'
    assert im.target(b[0x20E98C])==0x51C3F48 and b[0x20E993].op_str=='r9d, 0x4388'
    assert im.target(b[0x20E9A2])==0x3506BF0 and im.target(b[0x20E9B0])==0x51C3F48
    assert im.pointer(0x3506BF0+8)==0x22512A0
    assert bodies[0x22512A0][0x22512F4].op_str=='0x2251b40'
    b=bodies[0x2251B40]
    assert im.target(b[0x2251B64])==0x3506DE0 and b[0x2251B6B].op_str=='qword ptr [rbx], rax'
    b=bodies[0x1BFD760]
    assert b[0x1BFD8EF].op_str=='ecx, 0x4388' and b[0x1BFD901].op_str=='0x2251b40'
    assert b[0x1BFD906].op_str=='rbx, rax' and b[0x1BFDA34].op_str=='qword ptr [rdi + 0x470], rbx'
    # Preserve the full path so register provenance and failed-allocation branch
    # remain reviewable rather than treating offset coincidence as type proof.
    report={'status':'candidate_animation_component_identity_verified_not_skill_binding',
       'exe_sha256':im.meta['exe_sha256'],'name':'cpAnimationTree','name_rva':hex(name),
       'dti':'0x51c3f48','dti_vtable':'0x3506bf0','object_vtable':'0x3506de0','size':0x4388,
       'construction':'DTI factory22512A0 ->2251B40; constructor installs3506DE0',
       'binding':'1BFD760 allocates4388/calls2251B40, keeps returned pointer inRBX and writes object+470 at1BFDA34',
       'relationship_to_prior_report':'The+470 receiver used by1C01880/2254100 is animation-tree storage in this native initialization path.209 there is passed as a decomposed animation resource identifier, not proven skill209.',
       'methods':out,'limits':['Actor concrete subtype/derived overrides remain separate questions.',
          'Animation may accompany equipment effects; this does not prove or disprove any complete Shield-Weaver mechanism.',
          'Do not modify this chain solely because its identifier equals skill209.',
          'Shield-Weaver skill accumulation, defense/reaction/hit/recovery lifecycle still incomplete.',
          'Static analysis only; no native execution or game-directory writes.']}
    (P/'research/horizon-other-animation-candidate-identity.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Verified cpAnimationTree registration, factory, constructor and object+470 binding; not skill209 proof.')
if __name__=='__main__':main()
