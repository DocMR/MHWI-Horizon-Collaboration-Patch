"""Certify armor recipe exclusion identity and generate a narrow postfilter."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    _, reader = decode_group(im, 0x19B0210)
    by = {x.address: x for x in reader}
    assert by[0x19B06C4].op_str == 'qword ptr [rax + 8]'
    assert by[0x19B06C7].op_str == 'word ptr [rbp + 0x33], ax'
    assert by[0x19B0757].op_str == 'rdx, [rbp - 9]'
    assert by[0x19B0770].op_str == '0x279de0'
    copy = {x.address: x for x in im.instructions(0x279DE0)}
    assert copy[0x279ED4].op_str == 'eax, word ptr [rdx + 0x3c]'
    assert copy[0x279ED8].op_str == 'word ptr [rcx + 0x1e], ax'
    code = im.instructions(0x278A00)
    cb = {x.address: x for x in code}
    for address, operand in {0x278A12: 'edx, word ptr [rdx + 0xa]',
                             0x278A22: 'rax, rax', 0x278A2B: 'ecx, word ptr [rax + 0x1e]',
                             0x278A2F: 'eax, byte ptr [rbx + 8]',
                             0x278A77: 'ecx, 0xe1', 0x278ABD: 'ecx, 0xe1', 0x278A5F: 'ecx, 0xe3',
                             0x278A87: 'eax, [rcx - 0x12a]', 0x278ACD: 'eax, [rcx - 0x12a]',
                             0x278A92: 'ecx, 0x20b', 0x278AD8: 'ecx, 0x20b',
                             0x278A9A: 'ecx, 0x214', 0x278AE0: 'ecx, 0x214',
                             0x278AA2: 'eax, [rcx - 0x2cd]', 0x278AE8: 'eax, [rcx - 0x2cd]'}.items():
        assert cb[address].op_str == operand
    population = {x.address: x for x in im.instructions(0x278CB0)}
    assert population[0x278DAF].op_str == '0x278a00'
    assert population[0x278DB6].mnemonic == 'jne' and population[0x278DB6].op_str == '0x278dc1'
    equipment = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    rows = [r for r in equipment['armor'] if r['platform'] == 'PS']
    assert len(rows) == 30
    assert {r['native_equipment_id'] for r in rows} == {225, 227, 298, 299, 523, 532, 718}
    assert all(0 <= r['slot'] <= 4 for r in rows)
    assert len({(r['slot'], r['index']) for r in rows}) == len(rows)
    generated = ['// Generated target armor recipe identities; not a general crafting grant.']
    for row in rows:
        generated.append('    {'+f'{row["slot"]}, {row["index"]}, {row["native_equipment_id"]}'+'},')
    module = P/'staging/Horizon-other-module'
    (module/'src/generated_armor_recipe_exclusions.inc').write_text('\n'.join(generated)+'\n', encoding='utf-8')
    raw = b''.join(x.bytes for x in code)
    (module/'src/generated_armor_predicate_owner.inc').write_text(
        '// Generated certified original predicate owner.\nconstexpr TreeOwnerSpec kArmorPredicateOwner = {'+
        f'0x278A00, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in hashlib.sha256(raw).digest())+'}};\n', encoding='utf-8')
    report = {'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': im.meta['exe_sha256'],
              'native_predicate': '0x278a00', 'owner_bytes': len(raw),
              'owner_sha256': hashlib.sha256(raw).hexdigest(),
              'operation': 'Postprocess only this predicate result for exact target category/serialized-index pairs.',
              'recipe_layout': {'category_byte': 8, 'serialized_armor_row_u16': 10},
              'armor_id_flow': 'Serialized armor.am_dat+53 WORD -> temp+3C -> native armor record+1E; not series ID.',
              'targets': [{k: r[k] for k in ('slot', 'index', 'native_equipment_id', 'name')} for r in rows],
              'instructions': [im.format(x) for x in code],
              'limits': ['Install only on278A00; never substitute this result for complete crafting admission.',
                         'Call original first; preserve its result for null/unrelated recipes.',
                         'Native threshold, materials, story, unlock checks and transactions remain separate.',
                         'This component installs no hook and has not been dynamically executed.']}
    for output in (P/'research/horizon-other-armor-exclusions.json', module/'merge/armor-exclusions.json'):
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'target_armor_recipes': len(rows), 'native_ids': sorted({r['native_equipment_id'] for r in rows})}))


if __name__ == '__main__':
    main()
