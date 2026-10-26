"""Map target equipment rows to serialized recipes and material candidates."""
from pathlib import Path
import json
import struct
from audit_horizon_other_equipment import rows, RECIPE_FMT
from build_horizon_other_content_inventory import strings

P = Path(__file__).resolve().parents[1]
PS = P/'research/horizon-other-source'
PC = P/'MHWI解包/chunk'


def main():
    equipment = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    weapons = json.loads((P/'research/horizon-other-palico-weapon.json').read_text(encoding='utf-8'))
    item_text = strings(PS/'common/text/ps4/item_eng.gmd')
    targets = []
    for entry in equipment['armor']:
        if entry['platform'] == 'PS':
            targets.append({'file': 'armor.eq_crt', 'category': entry['slot'],
                            'equipment_index': entry['index'], 'name': entry['name']})
    for entry in equipment['palico_armor']:
        if entry['platform'] == 'PS':
            targets.append({'file': 'ot_equip.eq_crt', 'category': entry['slot']+1,
                            'equipment_index': entry['index'], 'name': entry['name']})
    for entry in weapons['weapon_candidates']:
        if entry['platform'] == 'PS':
            targets.append({'file': 'ot_equip.eq_crt', 'category': 0,
                            'equipment_index': entry['row'], 'name': entry['name']})
    recipes, missing = [], []
    for filename in ('armor.eq_crt', 'ot_equip.eq_crt'):
        _, ps_rows = rows(PS/'common/equip'/filename, RECIPE_FMT)
        _, pc_rows = rows(PC/'common/equip'/filename, RECIPE_FMT)
        for target in [r for r in targets if r['file'] == filename]:
            matches = [(i, r) for i, r in enumerate(ps_rows) if r[:2] == (target['category'], target['equipment_index'])]
            if not matches:
                missing.append(target)
            for index, record in matches:
                materials = []
                for j in (7, 9, 11, 13):
                    item, quantity = record[j:j+2]
                    if quantity:
                        materials.append({'item_id': item, 'quantity': quantity,
                                          'text_index_candidate': 2*item,
                                          'name_candidate': item_text[2*item] if 2*item < len(item_text) else None})
                pc_matches = [(i, r) for i, r in enumerate(pc_rows) if r[:2] == record[:2]]
                recipes.append({**target, 'recipe_row': index, 'unlock_item': record[2],
                                'native_numeric_gate_u32': record[15],
                                'raw_gate_values': list(record[3:7]), 'materials': materials,
                                'pc_matching_rows': [i for i, _ in pc_matches],
                                'pc_record_identical': len(pc_matches) == 1 and pc_matches[0][1] == record,
                                'ps_raw_hex': struct.pack(RECIPE_FMT, *record).hex()})
    report = {'status': 'Static recipe associations; native menu availability not certified',
              'recipes': recipes, 'without_direct_recipe': missing,
              'limits': ['Category association uses armor slot / Palico weapon0 head1 body2 and matching table indices; consumer confirmation remains required.',
                         'Material text at twice item ID is a candidate association, not yet a certified native text lookup.',
                         'No direct recipe does not mean content is absent; layered delivery and full-set paths remain to trace.']}
    (P/'research/horizon-other-armor-palico-recipes.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'target_equipment': len(targets), 'recipes': len(recipes),
                      'pc_identical': sum(r['pc_record_identical'] for r in recipes),
                      'without_direct_recipe': [r['name'] for r in missing]}, ensure_ascii=False))


if __name__ == '__main__':
    main()
