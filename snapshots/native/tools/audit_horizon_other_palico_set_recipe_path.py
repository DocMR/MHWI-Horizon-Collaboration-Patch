"""Certify retained Palico series and full-set recipe registration, statically."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from audit_horizon_other_equipment import rows, CAT_ARMOR_FMT


def main():
    im = CodeImage()
    _, reader = decode_group(im, 0x1A0BD10)
    _, consumer = decode_group(im, 0xBCD610)
    copy = im.instructions(0x11E7320)
    by = {i.address: i for i in reader + consumer + copy}
    expected = {
        0x1A0BDC2: 'qword ptr [rax + 8]',
        0x1A0BDC8: 'word ptr [rbp - 0x34], ax',
        0x1A0BDF0: 'byte ptr [rbp - 0x32], al',
        0x1A0BE17: 'byte ptr [rbp - 0x31], al',
        0x11E7325: 'eax, word ptr [rdx + 4]',
        0x11E7329: 'word ptr [rcx + 0x14], ax',
        0x11E732D: 'eax, byte ptr [rdx + 6]',
        0x11E7331: 'byte ptr [rcx + 0xc], al',
        0x11E7334: 'eax, byte ptr [rdx + 7]',
        0x11E7338: 'byte ptr [rcx + 0x34], al',
        0xBCD783: 'eax, byte ptr [rax + 0xc]',
        0xBCD787: 'edx, word ptr [rsi + 0x14]',
        0xBCD7A4: 'qword ptr [r15 + rcx*8 + 0x1950], rbx',
        0xBCD7AF: 'qword ptr [r15 + r8*8 + 0x1940], rsi',
        0xBCD846: 'byte ptr [rsi + 0x34], 1',
        0xBCD84C: 'qword ptr [r15 + rax*8 + 0x51b0], r14',
    }
    for address, operand in expected.items():
        assert by[address].op_str == operand, (hex(address), by[address].op_str)

    _, source = rows(P/'research/horizon-other-source/common/equip/otomoArmor.oam_dat', CAT_ARMOR_FMT)
    _, pc = rows(P/'MHWI解包/chunk/common/equip/otomoArmor.oam_dat', CAT_ARMOR_FMT)
    targets = [i for i, row in enumerate(source) if row[16] in (59, 166, 189)]
    assert targets == [61, 126, 149, 223, 288, 311]
    recipes = json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))['recipes']
    associations = []
    for index in targets:
        record = source[index]
        assert record == pc[index] and record[3] == 1
        matches = [r for r in recipes if r['file'] == 'ot_equip.eq_crt'
                   and r['category'] == record[2]+1 and r['equipment_index'] == record[0]]
        assert len(matches) == 1 and matches[0]['pc_record_identical']
        associations.append(dict(row=index, equipment_index=record[0], series=record[1],
            slot=record[2], full_set=record[3], recipe_row=matches[0]['recipe_row']))
    assert {r['series'] for r in associations} == {66, 156, 179}
    for series in (66, 156, 179):
        assert {r['slot'] for r in associations if r['series'] == series} == {0, 1}
    edits = json.loads((P/'research/horizon-other-palico-exclusion-plan.json').read_text(encoding='utf-8'))['edits']
    assert all(int(e['rva'], 16)+len(bytes.fromhex(e['replacement_hex'])) <= 0xBCD75C for e in edits)
    evidence = []
    for name, code in [('reader', reader), ('copy', copy), ('recipe_consumer', consumer)]:
        evidence.append(dict(name=name, instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest()))
    report = dict(status='Static inherited series/full-set registration path',
        exe_sha256=im.meta['exe_sha256'], associations=associations, evidence=evidence,
        fields=dict(series=dict(serialized_offset=4, temporary_offset=4, native_offset=0x14),
                    slot=dict(serialized_offset=6, temporary_offset=6, native_offset=0xC),
                    full_set=dict(serialized_offset=7, temporary_offset=7, native_offset=0x34)),
        consumers=dict(series_recipe_registration='0xbcd783..0xbcd7af',
                       full_set_registration='0xbcd841..0xbcd84c'),
        finding='Existing exclusion edits precede and preserve series grouping and full-set registration; no additional set-registration hook is justified here.',
        limits=['Registration is not proof of crafting payment, inventory mutation, save persistence or all menu callers.',
                'Hunter layered/full-set handling is separate and is not certified by this Palico report.',
                'No target code execution, dynamic tests, or game-directory writes.'])
    (P/'research/horizon-other-palico-set-recipe-path.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(target_rows=len(associations), series=3, preserved_edits=len(edits))))


if __name__ == '__main__':
    main()
