"""Certify serialized recipe fields through native loading and admission reads."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group
from capstone.x86 import X86_REG_RBP


def main():
    im = CodeImage()
    assert im.pointer(0x33E3CA8+0x50) == 0x19DD4D0
    fragments, reader = decode_group(im, 0x19DD4D0)
    rb = {i.address: i for i in reader}
    assert rb[0x19DD71C].op_str == 'rdx, [rbp - 0x28]'
    assert rb[0x19DD735].op_str == '0xb0e6c0'
    stores = [i for i in reader if i.mnemonic == 'mov' and i.operands[0].type == 3
              and i.operands[0].mem.base == X86_REG_RBP
              and -0x28 <= i.operands[0].mem.disp <= -4]
    widths = [1, 2, 2, 4, 4, 4, 4, 2, 1, 2, 1, 2, 1, 2, 1, 4]
    temporary = [0, 2, 4, 8, 12, 16, 20, 24, 32, 26, 33, 28, 34, 30, 35, 36]
    assert len(stores) == 16
    assert [i.operands[0].size for i in stores] == widths
    assert [i.operands[0].mem.disp+0x28 for i in stores] == temporary
    copy = im.instructions(0xB0E6C0)
    assert len(copy) == 33 and copy[-1].mnemonic == 'ret'
    field_map = {}
    for index in range(16):
        load, store = copy[2*index:2*index+2]
        src, dst = load.operands[1], store.operands[0]
        assert src.type == dst.type == 3 and src.size == dst.size
        field_map[src.mem.disp] = dict(native_offset=dst.mem.disp, width=src.size)
    offset, mapping = 0, []
    for store, width, temp in zip(stores, widths, temporary):
        assert field_map[temp]['width'] == width
        mapping.append(dict(serialized_offset=offset, width=width, temporary_offset=temp,
                            native_offset=field_map[temp]['native_offset'], reader_store=hex(store.address)))
        offset += width
    assert offset == 37
    _, admission = decode_group(im, 0x16B7740)
    ab = {i.address: i for i in admission}
    assert ab[0x16B77E0].op_str == 'ebx, word ptr [rbp + 0xc]'
    assert ab[0x16B7802].op_str == 'eax, dword ptr [rax + rcx*4 + 0xf6d98]'
    assert ab[0x16B7813].op_str == 'rbx, [rbp + 0x20]'
    assert ab[0x16B7838].op_str == 'eax, dword ptr [rax + rcx*4 + 0xf6d98]'
    recipes = json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))['recipes']
    layered = [r for r in recipes if r['file'] == 'armor.eq_crt' and r['equipment_index'] in
               (156, 805, 1420, 2032, 2644, 637, 1251, 1866, 2475, 3090)]
    assert len(layered) == 10 and all(r['pc_record_identical'] for r in layered)
    aloy_indices = (156, 805, 1420, 2032, 2644)
    for r in layered:
        if r['equipment_index'] == 156:
            assert r['unlock_item'] == 976
            assert [(m['item_id'], m['quantity']) for m in r['materials']] == [(976, 3), (927, 2)]
        elif r['equipment_index'] in aloy_indices:
            assert r['unlock_item'] == 0 and r['materials'] == []
        else:
            assert r['unlock_item'] == 2709
            assert [(m['item_id'], m['quantity']) for m in r['materials']] == [(2709, 5), (1321, 3), (1320, 3), (1853, 1)]
    report = dict(status='Static recipe scalar field flow and inherited acquisition gates',
        exe_sha256=im.meta['exe_sha256'], reader='0x19dd4d0', reader_fragments=fragments,
        reader_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in reader)).hexdigest(),
        copy='0xb0e6c0', serialized_record_bytes=offset, fields=mapping,
        native_admission=dict(function='0x16b7740', unlock_item_word='recipe+0xc',
            material_words='recipe+0x20,22,24,26', acquired_bitmap='character+0xf6d98',
            limits='Other original progression/story/bitmap gates and branch order remain required; acquisition is not inventory quantity or payment.'),
        layered_recipes=[dict(category=r['category'], equipment_index=r['equipment_index'],
            recipe_row=r['recipe_row'], unlock_item=r['unlock_item'],
            raw_gate_values=r['raw_gate_values'], materials=r['materials']) for r in layered],
        aloy_set_recipe_shape='Head recipe carries3x976+2x927; four other slot recipes have unlock0/materials empty. Do not force independent per-slot admission or duplicate material costs.',
        decision='Keep current original recipes and acquisition/progression admission; existing target-only exclusion adjustment must not replace this predicate.',
        limits=['Sequential field widths/temporary stores and copy offsets certified; native parser error paths are preserved.',
                'No claim that a material unlock alone satisfies every admission branch.',
                'Final crafting payment/output/layered equip/save still separate work.',
                'No original code execution or dynamic validation.'])
    (P/'research/horizon-other-recipe-field-flow.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(recipe_bytes=37, mapped_fields=16, target_layered_recipes=len(layered))))


if __name__ == '__main__':
    main()
