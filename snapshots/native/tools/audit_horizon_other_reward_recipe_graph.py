"""Join retained recipe inputs to six source REM tables and PC item records.

Static data only. No game loading, recipe execution, or install operation.
"""
from pathlib import Path
import hashlib
import json
import struct

from build_horizon_other_content_inventory import strings

P = Path(__file__).resolve().parents[1]


def main():
    def read(name):
        return json.loads((P / 'research' / name).read_text(encoding='utf-8'))

    quests = read('horizon-other-six-quest-resources.json')['quests']
    recipes = read('horizon-other-armor-palico-recipes.json')['recipes']
    recipes += read('horizon-other-equipment-audit.json')['bow_recipes']
    item_path = P / 'MHWI解包/chunk/common/item/itemData.itm'
    data = item_path.read_bytes()
    ps_item_path = P/'research/horizon-other-source/common/item/itemData.itm'
    ps_data = ps_item_path.read_bytes()
    assert ps_data == data, 'Revisit item-parameter differences before claiming native inheritance'
    fmt = '<IBIBbbHIIBBII'
    size = struct.calcsize(fmt)
    count = struct.unpack_from('<I', data, 6)[0]
    assert size == 32 and len(data) == 10 + count * size
    items = {}
    for row in range(count):
        offset = 10 + row * size
        values = struct.unpack_from(fmt, data, offset)
        assert values[0] not in items
        items[values[0]] = dict(row=row, offset=offset,
                              raw_hex=data[offset:offset+size].hex())

    reward_sources = {}
    for quest in quests:
        active_rem = [r for r in quest['rem_ids'] if r]
        assert len(active_rem) == 1
        rem_id = active_rem[0]
        raw = (P / f'research/horizon-other-quest-source/quest/rem/remData_{rem_id}.rem').read_bytes()
        assert len(raw) == 114 and struct.unpack_from('<IH', raw) == (0x18091001, 0x40)
        decoded = []
        for slot in range(16):
            item = struct.unpack_from('<I', raw, 18 + slot * 4)[0]
            quantity, weight = raw[82 + slot], raw[98 + slot]
            if quantity and weight:
                decoded.append(dict(slot=slot, item_id=item, quantity=quantity, weight=weight))
                reward_sources.setdefault(item, []).append(dict(quest_id=quest['quest_id'],
                    rem_id=rem_id, slot=slot, quantity=quantity, weight=weight))
        assert decoded == quest['rewards']

    consumers = {}
    for recipe in recipes:
        identity = dict(file=recipe['file'], category=recipe['category'],
                        equipment_index=recipe['equipment_index'],
                        row=recipe.get('recipe_row', recipe.get('row')))
        assert recipe.get('pc_record_identical', recipe.get('pc_row_identical'))
        consumers.setdefault(recipe['unlock_item'], []).append(dict(**identity, role='unlock'))
        for material in recipe['materials']:
            if material['quantity']:
                consumers.setdefault(material['item_id'], []).append(dict(**identity,
                    role='material', quantity=material['quantity']))

    text = strings(P / 'research/horizon-other-source/common/text/ps4/item_eng.gmd')
    graph = []
    for item in sorted(set(consumers) | set(reward_sources)):
        assert item in items, item
        graph.append(dict(item_id=item, pc_record=items[item],
            name_candidate=text[2*item] if 2*item < len(text) else None,
            rewards=reward_sources.get(item, []), recipe_consumers=consumers.get(item, [])))
    exclusive = (926, 927, 976, 2707, 2708, 2709)
    assert all(reward_sources.get(i) for i in exclusive)
    result = dict(status='Static reward/recipe graph; native acquisition and crafting not certified',
        quest_count=len(quests), recipe_count=len(recipes), item_count=len(graph),
        pc_item_table_sha256=hashlib.sha256(data).hexdigest(),
        ps_item_table_sha256=hashlib.sha256(ps_data).hexdigest(), pc_ps_item_table_identical=True,
        collaboration_item_ids=list(exclusive), items=graph,
        limits=['PS/PC itemData whole-table identity is now verified; inventory acquisition and native item consumers still require separate evidence.',
                'Weights are serialized REM weights, not certified final drop probabilities or guaranteed counts.',
                'No edge from these six REM tables does not imply unobtainability: ordinary rewards, gathering and delivery remain native paths.',
                'Item text at twice item ID remains a candidate association.',
                'Direct recipe coverage excludes unresolved layered delivery and full-set routes; no claim of complete crafting coverage.'])
    target = P / 'research/horizon-other-reward-recipe-graph.json'
    target.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(quests=len(quests), recipes=len(recipes), items=len(graph),
        collaboration=[dict(item_id=i, rewards=len(reward_sources[i]),
                            consumers=len(consumers.get(i, []))) for i in exclusive])))


if __name__ == '__main__':
    main()
