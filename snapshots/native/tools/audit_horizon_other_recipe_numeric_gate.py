"""Bind recipe serialized tail DWORD to its native population gate."""
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    _, reader = decode_group(im, 0x19DD4D0)
    by = {x.address: x for x in reader}
    for address, operand in {0x19DD6D5: 'qword ptr [rax + 0x10]',
                             0x19DD6DD: 'dword ptr [rbp - 4], eax',
                             0x19DD71C: 'rdx, [rbp - 0x28]',
                             0x19DD735: '0xb0e6c0'}.items():
        assert by[address].op_str == operand
    copy = im.instructions(0xB0E6C0)
    cb = {x.address: x for x in copy}
    assert cb[0xB0E72A].op_str == 'eax, dword ptr [rdx + 0x24]'
    assert cb[0xB0E72D].op_str == 'dword ptr [rcx + 0x2c], eax'
    population = {x.address: x for x in im.instructions(0x278CB0)}
    assert population[0x278D9D].op_str == 'ecx, dword ptr [rax + 0x2c]'
    assert population[0x278DA0].op_str == '0x18bc560'
    gate = {x.address: x for x in im.instructions(0x18BC560)}
    assert gate[0x18BC575].op_str == 'ebx, eax' and gate[0x18BC577].mnemonic == 'setbe'
    # Native reader's final scalar is serialized after the37-byte recipe's
    # first33 bytes. Temporary layout uses padding; it is not file layout.
    recipes = json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))
    values = []
    for row in recipes['recipes']:
        raw = bytes.fromhex(row['ps_raw_hex'])
        assert len(raw) == 37
        value = struct.unpack_from('<I', raw, 33)[0]
        assert value == row['native_numeric_gate_u32']
        values.append({'file': row['file'], 'recipe_row': row['recipe_row'], 'name': row['name'], 'value': value})
    report = {'exe_sha256': im.meta['exe_sha256'], 'serialized_offset': 33,
              'temporary_offset': '0x24', 'native_record_offset': '0x2c',
              'flow': 'W32 final scalar -> temporary+24 -> B0E6C0 record+2C ->278D9D ECX ->18BC560 unsigned comparison',
              'target_recipes': values,
              'limits': ['Numeric threshold is certified; its semantic name and current singleton value remain unproven.',
                         'Do not equate this field with four independent flags or remove the native comparison.',
                         'No target code executed or dynamic validation.']}
    _, upgrade_reader = decode_group(im, 0x19DDCE0)
    ub = {x.address: x for x in upgrade_reader}
    for address, operand in {0x19DDF16: 'qword ptr [rax + 0x10]',
                             0x19DDF1E: 'dword ptr [rbp - 0x10], eax',
                             0x19DDF5D: 'rdx, [rbp - 0x38]',
                             0x19DDF76: '0xb0eaf0'}.items():
        assert ub[address].op_str == operand
    upgrade_copy = {x.address: x for x in im.instructions(0xB0EAF0)}
    assert upgrade_copy[0xB0EB74].op_str == 'eax, dword ptr [rdx + 0x28]'
    assert upgrade_copy[0xB0EB77].op_str == 'dword ptr [rcx + 0x30], eax'
    assert population[0x278E0D].op_str == 'ecx, dword ptr [rax + 0x30]'
    assert population[0x278E10].op_str == '0x18bc560'
    report['upgrade_flow'] = {'serialized_offset': 37, 'temporary_offset': '0x28',
                              'record_offset': '0x30', 'copy': '0xb0eaf0',
                              'population_load': '0x278e0d', 'gate': '0x18bc560'}
    equipment = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    bow_recipes = equipment['bow_recipes']
    for row in bow_recipes:
        raw = bytes.fromhex(row['ps_raw_hex'])
        assert struct.unpack_from('<I', raw, 33 if row['file'].endswith('eq_crt') else 37)[0] == row['native_numeric_gate_u32']
    report['bow_recipe_numeric_gates'] = [{k: r[k] for k in ('file', 'row', 'native_numeric_gate_u32')} for r in bow_recipes]
    (P/'research/horizon-other-recipe-numeric-gate.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'target_recipes': len(values), 'numeric_gate_values': sorted({r['value'] for r in values})}))


if __name__ == '__main__':
    main()
