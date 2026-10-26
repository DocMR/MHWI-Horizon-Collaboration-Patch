"""Trace hunter recipe series/type fields into retained native menu tables."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from audit_horizon_other_equipment import rows, ARMOR_FMT


def main():
    im = CodeImage()
    _, reader = decode_group(im, 0x19B0210)
    _, menu = decode_group(im, 0x279300)
    copy = im.instructions(0x279DE0)
    by = {i.address: i for i in reader + menu + copy}
    checks = {
        0x19B02FD: 'qword ptr [rax + 8]',
        0x19B0303: 'word ptr [rbp - 1], ax',
        0x19B032B: 'byte ptr [rbp + 1], al',
        0x19B0352: 'byte ptr [rbp + 2], al',
        0x279DF4: 'eax, word ptr [rdx + 8]',
        0x279DF8: 'word ptr [rcx + 0x14], ax',
        0x279DFC: 'eax, byte ptr [rdx + 0xa]',
        0x279E00: 'byte ptr [rcx + 0x13], al',
        0x279E03: 'eax, byte ptr [rdx + 0xb]',
        0x279E07: 'byte ptr [rcx + 0xc], al',
        0x2793C4: '0x278a00',
        0x2793CB: '0x279562',
        0x2793E4: 'eax, byte ptr [rax + 0xc]',
        0x2793F8: 'edx, word ptr [rsi + 0x14]',
        0x27942B: 'qword ptr [r15 + r8*8 + 0x119c0], rsi',
        0x279433: 'qword ptr [r15 + r8*8 + 0x119f0], rbx',
        0x279494: 'eax, byte ptr [rsi + 0x13]',
        0x279498: 'al, 1',
        0x27949C: 'qword ptr [r15 + rcx*8 + 0x2d828], rdi',
        0x2794A8: 'al, 2',
        0x2794AC: 'qword ptr [r15 + rcx*8 + 0x2efa8], rdi',
        0x2794B8: 'al, 3',
        0x2794BC: 'qword ptr [r15 + rcx*8 + 0x2efa8], rdi',
    }
    for address, operand in checks.items():
        assert by[address].op_str == operand, (hex(address), by[address].op_str)
    assert im.target(by[0x2793D5]) == 0x5012128
    assert im.target(by[0x279363]) == 0x5012150
    equipment = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    targets = [r for r in equipment['armor'] if r['platform'] == 'PS']
    recipes = json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))['recipes']
    _, ps = rows(P/'research/horizon-other-source/common/equip/armor.am_dat', ARMOR_FMT)
    _, pc = rows(P/'MHWI解包/chunk/common/equip/armor.am_dat', ARMOR_FMT)
    groups = {}
    for target in targets:
        record = ps[target['row']]
        assert record == pc[target['row']]
        assert record[3:6] == (target['set_layered_id'], target['type'], target['slot'])
        match = [r for r in recipes if r['file'] == 'armor.eq_crt'
                 and r['category'] == target['slot'] and r['equipment_index'] == target['index']]
        assert len(match) == 1 and match[0]['pc_record_identical']
        kind = record[4]
        assert kind in (0, 1, 2, 3)
        group = groups.setdefault(record[3], dict(series=record[3], type=kind,
            native_special_table={0: None, 1: '0x2d828', 2: '0x2efa8', 3: '0x2efa8'}[kind], records=[]))
        assert group['type'] == kind
        group['records'].append(dict(slot=record[5], equipment_index=record[0], recipe_row=match[0]['recipe_row']))
    assert len(groups) == 6
    for group in groups.values():
        assert {r['slot'] for r in group['records']} == set(range(5))
    report = dict(status='Static retained hunter series/type registration; final crafting still incomplete',
        exe_sha256=im.meta['exe_sha256'], groups=list(groups.values()),
        fields=dict(series=dict(serialized_offset=7, temporary_offset=8, native_offset=0x14),
                    type=dict(serialized_offset=9, temporary_offset=10, native_offset=0x13),
                    slot=dict(serialized_offset=10, temporary_offset=11, native_offset=0xC)),
        evidence=[dict(name=name, instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest())
            for name, code in [('reader', reader), ('copy', copy), ('menu', menu)]],
        finding='Native menu retains five-slot grouping and type1/type2/type3 tables after the shared armor exclusion predicate; existing narrow predicate hook covers this caller too.',
        limits=['Table roles follow type-conditioned stores; type names alone are not a complete menu or purchase contract.',
                'Native sorted-series lookup and nonzero registration gates remain unchanged.',
                'Payment, output ownership, layered equip/save and all callers remain separate work.',
                'No additional hook, game execution, dynamic validation or game-directory writes.'])
    (P/'research/horizon-other-hunter-set-recipe-path.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(groups=len(groups), armor_records=len(targets), types={k: sum(g['type']==k for g in groups.values()) for k in range(4)})))


if __name__ == '__main__':
    main()
