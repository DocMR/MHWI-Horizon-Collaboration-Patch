"""Export exact inline Palico recipe exclusions for later shared cloning."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    fragments, code = decode_group(im, 0xBCD610)
    start, end = fragments[0][0], fragments[-1][1]
    raw = im.code[start-im.base:end-im.base]
    assert sum(x.size for x in code) == end-start == 646
    assert all(im.valid[start-im.base:end-im.base])
    by = {x.address: x for x in code}
    assert by[0xBCD690].op_str == 'r12d, 0xa6'
    assert by[0xBCD6AE].op_str == 'r13d, [r12 + 0x17]'
    for address, operand in {0xBCD6C0: 'ecx, dword ptr [rax + 0x2c]',
                             0xBCD6C3: '0x18bc560', 0xBCD6FB: 'ecx, word ptr [rax + 0x1a]',
                             0xBCD73A: 'eax, word ptr [rax + 0x10]'}.items():
        assert by[address].op_str == operand
    edits = []
    for address, expected in ((0xBCD703, '0f844b010000'), (0xBCD70D, '0f8441010000'),
                              (0xBCD719, 'e936010000'), (0xBCD742, '0f840c010000'),
                              (0xBCD74C, '0f8402010000'), (0xBCD756, '0f84f8000000')):
        assert by[address].bytes.hex() == expected and by[address].operands[0].imm == 0xBCD854
        # Last armor comparison originally has an unconditional skip after a
        # JNE to the normal path. Redirect it there explicitly instead of
        # falling into the alternate flag test at71E.
        replacement = 'eb41909090' if address == 0xBCD719 else '90'*by[address].size
        edits.append({'rva': hex(address), 'offset': address-start, 'expected_hex': expected,
                      'replacement_hex': replacement})
    armor = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    weapons = json.loads((P/'research/horizon-other-palico-weapon.json').read_text(encoding='utf-8'))
    targets = [r for r in armor['palico_armor'] if r['platform'] == 'PS']
    weapon_targets = [r for r in weapons['weapon_candidates'] if r['platform'] == 'PS']
    assert len(targets) == 6 and len(weapon_targets) == 3
    assert {r['native_equipment_id'] for r in targets} == {59, 166, 189}
    assert {r['native_equipment_id'] for r in weapon_targets} == {59, 166, 189}
    report = {'schema_version': 1, 'module': 'horizon-other', 'owner_rva': hex(start),
              'owner_bytes': len(raw), 'owner_sha256': hashlib.sha256(raw).hexdigest(),
              'exe_sha256': im.meta['exe_sha256'], 'fragments': [list(f) for f in fragments], 'edits': edits,
              'target_native_ids': [59, 166, 189],
              'armor_rows': [r['row'] for r in targets], 'weapon_rows': [r['row'] for r in weapon_targets],
              'state': 'Exact edits generated for compile-only buffer-edit component; no installed runtime hook',
              'limits': ['Threshold comparison and all downstream material/series/full-set processing are preserved.',
                         'Native OAM/OWP field/ID contracts are in palico-native-fields.json; final hook relocation and bootstrap remain pending.',
                         'No target code executed or dynamic validation.']}
    lines = ['// Generated exact contiguous native owner; apply before relocation.',
             'constexpr TreeOwnerSpec kPalicoOwner = {'+f'0x{start:X}, {len(raw)}, '+'{'+
             ', '.join(f'0x{x:02X}' for x in hashlib.sha256(raw).digest())+'}};',
             'constexpr PalicoEdit kPalicoEdits[] = {']
    for edit in edits:
        before, after = bytes.fromhex(edit['expected_hex']), bytes.fromhex(edit['replacement_hex'])
        assert len(before) == len(after)
        lines.append('    {'+f'{edit["offset"]}, {len(before)}, '+'{'+
                     ', '.join(f'0x{x:02X}' for x in before)+'}, {'+
                     ', '.join(f'0x{x:02X}' for x in after)+'}},')
    lines += ['};', '']
    (P/'staging/Horizon-other-module/src/generated_palico_recipe_edits.inc').write_text('\n'.join(lines), encoding='utf-8')
    for target in (P/'research/horizon-other-palico-exclusion-plan.json',
                   P/'staging/Horizon-other-module/merge/palico-exclusion-edits.json'):
        target.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'edits': len(edits), 'target_armor': len(targets), 'target_weapons': len(weapon_targets)}))


if __name__ == '__main__':
    main()
