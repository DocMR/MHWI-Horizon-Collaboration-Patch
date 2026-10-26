"""Read PS/PC equipment identities and retained recipes; never install changes."""
from pathlib import Path
import json
import struct
from build_horizon_other_content_inventory import strings

P = Path(__file__).resolve().parents[1]
PS = P / 'research/horizon-other-source'
PC = P / 'MHWI解包/chunk'
ARMOR_FMT = '<IHBHBBHHHHBBI5b4B' + 'HB' * 5 + 'IHHHB'
CAT_ARMOR_FMT = '<IHBBIBHIII5bIHHH'
RECIPE_FMT = '<BHHiIII' + 'HB' * 4 + 'I'
UPGRADE_FMT = '<BHHiII' + 'HB' * 4 + '4HI'


def rows(path, fmt):
    data = path.read_bytes()
    count = struct.unpack_from('<I', data, 6)[0]
    size = struct.calcsize(fmt)
    assert len(data) == 10 + count * size, (str(path), len(data), count, size)
    return data, [struct.unpack_from(fmt, data, 10 + i * size) for i in range(count)]


def main():
    armors, palicos, recipes = [], [], []
    for platform, root in [('PS', PS), ('PC', PC)]:
        text_root = root / ('common/text/ps4' if platform == 'PS' else 'common/text/steam')
        _, records = rows(root / 'common/equip/armor.am_dat', ARMOR_FMT)
        text = strings(text_root / 'armor_eng.gmd')
        for row, record in enumerate(records):
            assert record[-3] < len(text) and record[-2] < len(text)
            name = text[record[-3]]
            if (platform == 'PS' and name.startswith(('Aloy', 'Shield-Weaver', 'Banuk')) or
                    platform == 'PC' and row in {r['row'] for r in armors if r['platform'] == 'PS'}):
                armors.append({'platform': platform, 'row': row, 'index': record[0],
                               'native_equipment_id': record[-4],
                               'set_layered_id': record[3], 'type': record[4], 'slot': record[5],
                               'model_ids': list(record[7:9]), 'set_skill': list(record[22:24]),
                               'name_index': record[-3], 'description_index': record[-2], 'name': name,
                               'raw_hex': struct.pack(ARMOR_FMT, *record).hex()})
        _, records = rows(root / 'common/equip/otomoArmor.oam_dat', CAT_ARMOR_FMT)
        text = strings(text_root / 'ot_armor_eng.gmd')
        for row, record in enumerate(records):
            assert record[-2] < len(text) and record[-1] < len(text)
            name = text[record[-2]]
            if (platform == 'PS' and name.startswith(('Felyne Watcher', 'Frostclaw')) or
                    platform == 'PC' and row in {r['row'] for r in palicos if r['platform'] == 'PS'}):
                palicos.append({'platform': platform, 'row': row, 'index': record[0],
                                'native_equipment_id': record[16],
                                'set_id': record[1], 'slot': record[2], 'full_set': record[3],
                                'model_id': record[7], 'variant': record[9], 'name_index': record[-2],
                                'description_index': record[-1], 'name': name,
                                'raw_hex': struct.pack(CAT_ARMOR_FMT, *record).hex()})
    bow_ids = {85, 86, 237}
    for filename, fmt in [('weapon.eq_crt', RECIPE_FMT), ('weapon.eq_cus', UPGRADE_FMT)]:
        ps_data, ps_rows = rows(PS / 'common/equip' / filename, fmt)
        pc_data, pc_rows = rows(PC / 'common/equip' / filename, fmt)
        assert len(ps_rows) == len(pc_rows)
        size = struct.calcsize(fmt)
        for i, record in enumerate(ps_rows):
            if record[0] != 11 or record[1] not in bow_ids:
                continue
            start = 7 if filename.endswith('eq_crt') else 6
            row = {'file': filename, 'row': i, 'category': record[0], 'equipment_index': record[1],
                   'native_numeric_gate_u32': record[-1],
                   'unlock_item': record[2], 'raw_gate_values': list(record[3:start]),
                   'materials': [{'item_id': record[j], 'quantity': record[j + 1]} for j in range(start, start + 8, 2)],
                   'pc_row_identical': record == pc_rows[i],
                   'ps_raw_hex': ps_data[10+i*size:10+(i+1)*size].hex(),
                   'pc_raw_hex': pc_data[10+i*size:10+(i+1)*size].hex()}
            if filename.endswith('eq_cus'):
                edges = list(record[14:18])
                row['forward_recipe_rows'] = edges
                row['forward_targets'] = [{'recipe_row': e, 'category': ps_rows[e][0], 'equipment_index': ps_rows[e][1]} for e in edges if e]
            recipes.append(row)
    result = {'status': 'Static identity and serialized recipe audit; no game runtime validation',
              'schema_sources': ['https://raw.githubusercontent.com/Synthlight/MHW-Editor/master/010%20Templates/am_dat.bt',
                                 'https://raw.githubusercontent.com/Synthlight/MHW-Editor/master/010%20Templates/oam_dat.bt'],
              'armor_record_bytes': struct.calcsize(ARMOR_FMT), 'palico_armor_record_bytes': struct.calcsize(CAT_ARMOR_FMT),
              'armor': armors, 'palico_armor': palicos, 'bow_recipes': recipes,
              'limits': ['Record field labels follow external schemas; runtime consumers still require verification.',
                         'Recipe gate fields are retained as raw values pending consumer certification.',
                         'Bow category11 and equipment row matching are consistent with existing weapon type numbering; live crafting remains unverified.']}
    for row in armors:
        if row['platform'] == 'PC':
            ps = next(r for r in armors if r['platform'] == 'PS' and r['row'] == row['row'])
            row['identical_to_ps'] = row['raw_hex'] == ps['raw_hex']
    target = P / 'research/horizon-other-equipment-audit.json'
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    for row in armors:
        if row['platform'] == 'PS':
            print('ARMOR', row['row'], row['name'], 'models', row['model_ids'], 'set', row['set_layered_id'])
    print('PALICO', json.dumps(palicos, ensure_ascii=False))
    print('RECIPES', json.dumps(recipes, ensure_ascii=False))


if __name__ == '__main__':
    main()
