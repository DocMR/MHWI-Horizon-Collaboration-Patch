"""Certify quest classification data flow from saved instructions, without execution."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    parser = {i.address: i for i in image.instructions(0x1A10170)}
    # Existing resource parser consumes two DWORDs from serialized offset6,
    # then a single byte from offset14, then the rank DWORD from offset15.
    expected = {0x1A10277: 'ff5010', 0x1A10288: 'ff5010',
                0x1A102B4: '0fb60401', 0x1A102BC: '88442468',
                0x1A102CA: 'ff5010', 0x1A102CD: '8944246c',
                0x1A11BD3: '488d8fb0000000', 0x1A11BE1: '488d542460',
                0x1A11BE6: 'e875b98fff',
                0x1A10434: 'ff5008', 0x1A10437: '668945b0',
                0x1A10445: 'ff5008', 0x1A1044C: '668945b2'}
    for address, expected_hex in expected.items():
        assert parser[address].bytes.hex() == expected_hex
    a, z = 0x130D560, next(x for x in image.starts if x > 0x130D560)
    assert 0 not in image.valid[a-image.base:z-image.base]
    copy = list(image.md.disasm(image.code[a-image.base:z-image.base], a))
    assert sum(i.size for i in copy) == z-a
    copy_by = {i.address: i for i in copy}
    for address, expected_hex in {0x130D56B: '0fb64208', 0x130D56F: '884134',
                                  0x130D572: '8b420c', 0x130D575: '894138',
                                  0x130D604: '0fb74250', 0x130D608: '6689414a',
                                  0x130D60C: '0fb74252', 0x130D610: '6689414c'}.items():
        assert copy_by[address].bytes.hex() == expected_hex
    # This pdata owner contains 316 bytes of code followed by a 48-byte jump table.
    # Decoding the table as instructions is invalid, not evidence of missing export bytes.
    start, table, end = 0xF2C320, 0xF2C45C, 0xF2C48C
    assert image.owner(start)[:2] == (start, end)
    assert 0 not in image.valid[start-image.base:end-image.base]
    instructions = list(image.md.disasm(image.code[start-image.base:table-image.base], start))
    assert sum(i.size for i in instructions) == table-start
    by_address = {i.address: i for i in instructions}
    targets = list(struct.unpack('<12I', image.code[table-image.base:end-image.base]))
    assert all(target in by_address for target in targets)
    assert targets[1] == 0xF2C3F5  # Optional category uses the native star comparison.
    for address, expected_hex in {0xF2C385: 'e8e62c4200', 0xF2C3C9: 'e8e28da400',
                                  0xF2C3D3: 'e8689ea400', 0xF2C3F5: '3bc5'}.items():
        assert by_address[address].bytes.hex() == expected_hex
    master = image.instructions(0x1976110)
    category = image.instructions(0x19751B0)
    star = image.instructions(0x1976240)
    assert master and category and star
    star_by = {i.address: i for i in star}
    assert star_by[0x19762DB].bytes.hex() == '0fbe4034'
    data = json.loads((P/'research/horizon-other-six-quest-resources.json').read_text(encoding='utf-8'))
    rows = []
    for quest in data['quests']:
        plain = (P/f'research/horizon-other-quest-source/decoded/questData_{quest["quest_id"]}.bin').read_bytes()
        rows.append({'quest_id': quest['quest_id'], 'star': plain[14],
                     'source_rank_dword': struct.unpack_from('<I', plain, 15)[0],
                     'source_words_52_54': list(struct.unpack_from('<2H', plain, 0x52)),
                     'reward_id': quest['rem_ids'][0], 'optional_category': 1,
                     'resource_name': f'quest\\questData_{quest["quest_id"]}'})
    # This gate is called before the category/star test. The retained export
    # has an external entry jump; decoding the complete span does not certify
    # original native behavior. Do not bypass it or publish its bytes as an owner.
    gate_rva = 0x5F83B0
    gate_offset = gate_rva-image.base
    gate_entry = image.code[gate_offset:gate_offset+16]
    assert 0 not in image.valid[gate_offset:gate_offset+16]
    entry_target = (gate_rva+5+struct.unpack_from('<i', gate_entry, 1)[0]
                    if gate_entry[0] == 0xE9 else None)
    gate_start, gate_end = 0x5F83B6, 0x5F8463
    gate_body = image.code[gate_start-image.base:gate_end-image.base]
    assert 0 not in image.valid[gate_start-image.base:gate_end-image.base]
    gate_instructions = list(image.md.disasm(gate_body, gate_start))
    assert sum(i.size for i in gate_instructions) == len(gate_body)
    gate_by = {i.address: i for i in gate_instructions}
    assert image.target(gate_by[0x5F83B8]) == 0x2FC5A60
    assert image.target(gate_by[0x5F83C5]) == 0x2FC6078
    table_bytes = image.exe[image.raw(0x2FC5A60):image.raw(0x2FC6078)]
    assert len(table_bytes) == 78*20
    special_rows = list(struct.iter_unpack('<5I', table_bytes))
    owned_ids = {q['quest_id'] for q in rows}
    assert not owned_ids.intersection(row[0] for row in special_rows)
    selector = {i.address: i for i in image.instructions(0x1353450)}
    substitute = {i.address: i for i in image.instructions(0x1355D00)}
    for addr, encoded in {0x1353450:'81fa81020000', 0x1353458:'81fadf030000',
                          0x1353460:'81fa69060000'}.items():
        assert selector[addr].bytes.hex() == encoded
    for addr, encoded in {0x1355D53:'81e925030000', 0x1355D5B:'83e901',
                          0x1355D60:'81e99b000000', 0x1355D68:'83f90a',
                          0x1355DB6:'8bc3'}.items():
        assert substitute[addr].bytes.hex() == encoded
    fixed_ids = {641, 991, 1641, 805, 806, 961, 971}
    assert not owned_ids.intersection(fixed_ids)
    result = {'exe_sha256': image.meta['exe_sha256'], 'quests': rows,
              'optional_menu_policy': 'Six owned quests are permanent optional quests; keep native admission, result and persistence.',
              'additional_menu_gate': {'call_rva': '0xf2c3b8', 'callee_rva': hex(gate_rva),
                  'saved_entry_hex': gate_entry.hex(),
                  'saved_entry_jump_target_rva': entry_target,
                  'original_body_certified': False,
                  'intact_body': {'start_rva': hex(gate_start), 'end_rva': hex(gate_end),
                      'sha256': hashlib.sha256(gate_body).hexdigest(),
                      'instructions': [image.format(i) for i in gate_instructions]},
                  'special_table': {'rva': '0x2fc5a60', 'count': 78,
                      'sha256': hashlib.sha256(table_bytes).hexdigest(),
                      'quest_ids': [r[0] for r in special_rows], 'owned_matches': []},
                  'other_fixed_ids': sorted(fixed_ids),
                  'action': 'Preserve this original predicate. Intact body has no owned special IDs; recover original entry only if a change becomes necessary. Do not force success.'},
              'star_flow': 'MIB+14 -> parser temporary+8 -> resource body+34 -> resource+E4 -> star getter1976240 signed byte read',
              'rank_flow': 'MIB+15 DWORD -> parser temporary+C -> resource body+38 -> resource+E8',
              'condition_word_flow': 'MIB+52/+54 WORDs -> temporary+50/+52 -> resource body+4A/+4C -> resource+FA/+FC; displayed rank labels certified separately',
              'star_filter': {'code_bytes': table-start, 'inline_table_bytes': end-table, 'category_targets': [hex(t) for t in targets],
                              'availability_gate_call': '0xf2c385 -> 0x134f070',
                              'category_call': '0xf2c3c9 -> 0x19751b0', 'star_call': '0xf2c3d3 -> 0x1976240'},
              'native_instructions': {'star_filter': [image.format(i) for i in instructions],
                                      'master': [image.format(i) for i in master],
                                      'category': [image.format(i) for i in category],
                                      'star_getter': [image.format(i) for i in star]},
              'limits': ['No game code or surrogate mechanism executed.',
                         'Catalog does not grant prerequisite, open or completion bits.',
                         'Displayed rank lower bounds are certified in horizon-other-rank-condition-labels.json; full native admission remains pending.',
                         'Native availability reads the mapping/open bitmap; six-task mapping and real prerequisites remain incomplete.']}
    (P/'research/horizon-other-quest-classification.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    out = P/'staging/Horizon-other-module/src/generated_quest_catalog.inc'
    out.parent.mkdir(parents=True, exist_ok=True)
    content = '// Generated from source MIBs and certified native field mapping.\n'
    for row in rows:
        content += '    {' + f'{row["quest_id"]}, {row["star"]}, {row["source_rank_dword"]}, {row["reward_id"]}, '
        content += json.dumps(row['resource_name']) + ', '
        content += ', '.join(str(value) for value in row['source_words_52_54']) + '},\n'
    out.write_text(content, encoding='utf-8')
    print(json.dumps({'quests': rows, 'star_filter_code_bytes': table-start, 'jump_table_bytes': end-table}, ensure_ascii=False))


if __name__ == '__main__':
    main()
