"""Resolve bounded instruction paths; never execute game code or inline tables."""
import hashlib,json
from analyze_hzd_code import CodeImage,P

def main():
    im=CodeImage()
    def span(a,z):
        payload=im.code[a-im.base:z-im.base]
        assert 0 not in im.valid[a-im.base:z-im.base]
        body=list(im.md.disasm(payload,a))
        assert body and sum(i.size for i in body)==z-a
        return {'start':hex(a),'end':hex(z),'sha256':hashlib.sha256(payload).hexdigest(),
                'instructions':[im.format(i) for i in body]}, {i.address:i for i in body}
    one,b=span(0x9E50A4,0x9E50C3)
    assert b[0x9E50A4].op_str=='rcx, qword ptr [rip + 0x4629b95]' and im.target(b[0x9E50A4])==0x500EC40
    assert b[0x9E50AE].op_str=='edx, 0xd1' and b[0x9E50BE].op_str=='0x1b37640'
    two,b=span(0xA36414,0xA3645E)
    assert b[0xA36414].op_str=='rax, qword ptr [rbx]'
    assert b[0xA36425].op_str=='r8, qword ptr [rax + 0x290]'
    assert b[0xA3641D].op_str=='edx, 0xd0' and b[0xA36456].op_str=='edx, 0xd1'
    assert b[0xA3645B].op_str=='r8' and b[0xA36449].op_str=='rcx, rbx'
    three,b=span(0xA7E40B,0xA7E412)
    assert b[0xA7E40B].op_str=='edx, 0xd1' and b[0xA7E410].op_str=='0xa7e453'
    three_tail,b=span(0xA7E453,0xA7E46C)
    assert im.target(b[0xA7E453])==0x5012DE8 and b[0xA7E45A].op_str=='0x2244930'
    assert b[0xA7E45F].op_str=='qword ptr [rbx + 0x8a8], rax'
    ctor,b=span(0xA35BDA,0xA35BEF)
    assert im.target(b[0xA35BE5])==0x308E348 and b[0xA35BEC].op_str=='qword ptr [rbx], rax'
    assert im.pointer(0x308E348+0x248)==0xA36150
    assert im.pointer(0x308E348+0x290)==0x1CCA6B0
    virtual=im.instructions(0x1CCA6B0)
    assert virtual and virtual[-1].op_str=='qword ptr [rax + 0x3d0]'
    assert any(i.op_str=='qword ptr [rax + 0x3c8]' for i in virtual)
    passthrough=im.instructions(im.pointer(0x308E348+0x3C8))
    assert len(passthrough)==2 and passthrough[0].op_str=='eax, edx'
    dispatch={'vtable':'0x308e348','constructor_install':ctor,
              'update_slot':'0x248','update':'0xa36150','slot290':'0x1cca6b0',
              'slot290_body':[im.format(i) for i in virtual],
              'slot3c8':[im.format(i) for i in passthrough],
              'slot3d0':hex(im.pointer(0x308E348+0x3D0)),
              'meaning':'For this constructor-installed table,209 passes through slot3C8 unchanged, then1C01850/1CCA740 and tail virtual3D0. Actual receiver type/derived overrides and downstream semantics remain uncertified.',
              'boundary_note':'Contiguous code pointers beginning308E2C8 span adjacent tables; that run is not this vtable start. Constructor installation certifies308E348.'}
    report={'status' :'bounded_paths_resolved_not_full_effect_audit','exe_sha256':im.meta['exe_sha256'],
      'constructor_table_dispatch':dispatch,
      'sites':[{'site':'0x9e50ae','path':[one],
                'meaning':'Passes209 to1B37640 with singleton500EC40, R8D3/R9B1/stack bool1; not direct known skill query.'},
               {'site':'0xa36456','path':[two],
                'meaning':'Selects208/209 before actor virtual290 with floating XMM2 and R9D10000; constructor-installed table resolves method1CCA6B0; actual receiver provenance remains unresolved.'},
               {'site':'0xa7e40b','path':[three,three_tail],
                'meaning':'Direct branch to2244930 with singleton5012DE8; returned pointer stored object+8A8, not direct known skill query.'}],
      'limits':['These are exact bounded paths; enclosing functions/CHAININFO groups are not certified fully decoded.',
                'Inline switch tables caused linear-disassembly failure; no table bytes reclassified as executable.',
                'No site is proven a Shield-Weaver mechanism; table-specific virtual290 is resolved; actual receiver provenance and transitive semantics remain unresolved.',
                'No target execution, dynamic tests, installation or game-directory writes.']}
    (P/'research/horizon-other-shield-query-gaps.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Three bounded paths and constructor-table virtual290 verified; actual receiver provenance/effect incomplete.')
if __name__=='__main__':main()
