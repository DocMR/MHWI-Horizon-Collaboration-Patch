"""Join target Palico rows with native group fields consumed by crafting."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    _,code=decode_group(im,0xBCD610)
    by={i.address:i for i in code}
    for address,operand in {0xBCD79C:'rdi, 6',
        0xBCD7A4:'qword ptr [r15 + rcx*8 + 0x1950], rbx',
        0xBCD7AF:'qword ptr [r15 + r8*8 + 0x1940], rsi',
        0xBCD7EF:'r14, [rdi + 0x1928]'}.items():
        assert by[address].op_str==operand
    associations=json.loads((P/'research/horizon-other-palico-set-recipe-path.json').read_text(encoding='utf-8'))['associations']
    assert len(associations)==6
    rows=[]
    for source in associations:
        slot,series=source['slot'],source['series']
        assert slot in (0,1) and source['full_set']==1
        equip=0x1940+slot*8-0x1928
        recipe=0x1950+(slot+1)*8-0x1928
        assert equip==0x18+slot*8 and recipe==0x30+slot*8
        rows.append(dict(**source,group_offset=hex(0x1928+series*64),
            equipment_pointer_offset=hex(equip),recipe_pointer_offset=hex(recipe)))
    edits=json.loads((P/'research/horizon-other-palico-exclusion-plan.json').read_text(encoding='utf-8'))['edits']
    assert all(int(e['rva'],16)+len(bytes.fromhex(e['replacement_hex']))<=0xBCD75C for e in edits)
    report=dict(exe_sha256=im.meta['exe_sha256'],rows=rows,
        registration_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest(),
        finding='Six target head/body equipment and recipe pointer fields match the exact row+18/+30 arrays consumed by1DC2570, including the two-pointer output loop.',
        limits=['Static association and field correspondence, not executed menu validation.',
            'Menu filtering/admission and recipe acquisition prerequisite handling still need end-to-end proof.',
            'Full-set mode may charge selected recipe only; do not invent additional per-piece charges.',
            'Save persistence and post-craft equip confirmation not complete.'])
    (P/'research/horizon-other-palico-craft-binding.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(target_rows=len(rows),series=sorted({r['series'] for r in rows}),preserved_exclusion_edits=len(edits))))


if __name__=='__main__':
    main()
