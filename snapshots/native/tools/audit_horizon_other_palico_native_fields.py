"""Certify OAM/OWP identity fields used by the inline recipe exclusions."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from audit_horizon_other_equipment import rows, CAT_ARMOR_FMT


def main():
    im = CodeImage()
    assert im.pointer(0x33E9DC8+0x50) == 0x1A0BD10
    assert im.pointer(0x33EA1D8+0x50) == 0x1A0DD80
    _, armor = decode_group(im, 0x1A0BD10)
    _, weapon = decode_group(im, 0x1A0DD80)
    ab, wb = {x.address: x for x in armor}, {x.address: x for x in weapon}
    for address, operand in {0x1A0BF77: 'qword ptr [rax + 8]',
                             0x1A0BF7A: 'word ptr [rbp - 0x10], ax',
                             0x1A0BFDD: 'rdx, [rbp - 0x38]',
                             0x1A0BFF6: '0x11e7320'}.items():
        assert ab[address].op_str == operand
    for address, operand in {0x1A0E027: 'qword ptr [rax + 8]',
                             0x1A0E02A: 'word ptr [rsp + 0x7a], ax',
                             0x1A0E094: 'rdx, [rsp + 0x58]',
                             0x1A0E0AE: '0x11ed0b0'}.items():
        assert wb[address].op_str == operand
    ac = {x.address: x for x in im.instructions(0x11E7320)}
    wc = {x.address: x for x in im.instructions(0x11ED0B0)}
    assert ac[0x11E7395].op_str == 'eax, word ptr [rdx + 0x28]'
    assert ac[0x11E7399].op_str == 'word ptr [rcx + 0x1a], ax'
    assert wc[0x11ED11D].op_str == 'eax, word ptr [rdx + 0x22]'
    assert wc[0x11ED121].op_str == 'word ptr [rcx + 0x10], ax'
    # Scope the ID-wide branch edits against the WHOLE source tables, rather
    # than only the rows already selected by their display names.
    _, all_armor = rows(P/'research/horizon-other-source/common/equip/otomoArmor.oam_dat', CAT_ARMOR_FMT)
    armor_rows = [i for i, row in enumerate(all_armor) if row[16] in (59, 166, 189)]
    source = P/'research/horizon-other-palico-weapon-ps.decrypted.bin'
    data = source.read_bytes()
    evidence = json.loads((P/'research/horizon-other-palico-weapon.json').read_text(encoding='utf-8'))
    ps = next(r for r in evidence['files'] if r['platform'] == 'PS')
    assert hashlib.sha256(data).hexdigest() == ps['plain_sha256']
    count = struct.unpack_from('<I', data, 6)[0]
    weapon_rows = [i for i in range(count) if struct.unpack_from('<H', data, 10+38*i+32)[0] in (59, 166, 189)]
    assert armor_rows == [61, 126, 149, 223, 288, 311]
    assert weapon_rows == [60, 130, 153]
    _, loop = decode_group(im, 0xBCD610)
    lb = {x.address: x for x in loop}
    assert lb[0xBCD6FB].op_str == 'ecx, word ptr [rax + 0x1a]'
    assert lb[0xBCD73A].op_str == 'eax, word ptr [rax + 0x10]'
    report = {'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': im.meta['exe_sha256'],
              'armor': {'serialized_offset': 36, 'temporary_offset': '0x28', 'native_offset': '0x1a',
                        'reader': '0x1a0bd10', 'copy': '0x11e7320', 'consumer': '0xbcd6fb', 'rows': armor_rows},
              'weapon': {'decrypted_serialized_offset': 32, 'temporary_offset': '0x22', 'native_offset': '0x10',
                         'reader': '0x1a0dd80', 'copy': '0x11ed0b0', 'consumer': '0xbcd73a', 'rows': weapon_rows},
              'target_native_ids': [59, 166, 189],
              'finding': 'Every source-table row with these native IDs is in this module scope; the six branch edits do not add other table identities.',
              'limits': ['Static scalar field/ID scope certification; not menu, crafting transaction or playback validation.',
                         'No target native code, new component or game execution.']}
    for path in (P/'research/horizon-other-palico-native-fields.json',
                 P/'staging/Horizon-other-module/merge/palico-native-fields.json'):
        path.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'armor_table_rows_checked': len(all_armor), 'weapon_table_rows_checked': count,
                      'target_armor_rows': len(armor_rows), 'target_weapon_rows': len(weapon_rows), 'unrelated_id_matches': 0}))


if __name__ == '__main__':
    main()
