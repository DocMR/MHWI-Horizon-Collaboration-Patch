"""Bind native descriptor globals and the shared recipe threshold receiver."""
import json
import hashlib
from analyze_hzd_code import CodeImage, P


def main():
    im = CodeImage()
    snapshots = json.loads((P/'research/equip-descriptor-separate-snapshots.json').read_text(encoding='utf-8'))
    table = snapshots['tables'][0]
    assert table['result_base'] == '0x5011f40' and table['descriptor_base'] == '0x500f4d0'
    bindings = []
    for index, global_rva, kind, path, suffix, vt, getter in [
        (61, 0x5012128, 0x50084D8, 'common\\equip\\armor', 'am_dat', None, None),
        (66, 0x5012150, 0x5009C40, 'common\\equip\\armor', 'eq_crt', 0x33E3CA8, 0x19DD4C0),
        (68, 0x5012160, 0x5009C78, 'common\\equip\\equip_custom', 'eq_cus', 0x33E3D58, 0x19DDCD0)]:
        rows = [r for r in table['entries'] if r['index'] == index]
        assert len(rows) == 1
        row = rows[0]
        assert int(row['result_global'], 16) == global_rva == 0x5011F40+index*8
        assert int(row['type'], 16) == kind and row['path'] == path
        if vt:
            assert im.pointer(vt+0x30) == getter
            code = im.instructions(getter)
            assert len(code) == 2 and code[-1].mnemonic == 'ret'
            offset = im.raw(im.target(code[0]))
            assert im.exe[offset:offset+len(suffix)+1] == suffix.encode()+b'\0'
        bindings.append({'index': index, 'global': hex(global_rva), 'type': hex(kind),
                         'path': path.replace('\\', '/')+'.'+suffix})
    ctor = {x.address: x for x in im.instructions(0x225BE20)}
    assert im.target(ctor[0x225BE4F]) == 0x35072F8
    assert ctor[0x225BE56].op_str == 'qword ptr [rdi], rax'
    assert im.target(ctor[0x225C3D1]) == 0x51C4148
    assert ctor[0x225C3D1].op_str.startswith('qword ptr')
    assert ctor[0x225C513].op_str == 'dword ptr [rdi + 0xaf5ec], 0x100'
    assert im.pointer(0x35072F8+0x68) == 0x225F1B0
    getter = im.instructions(0x225F1B0)
    assert len(getter) == 2 and getter[0].op_str == 'eax, dword ptr [rcx + 0xaf5ec]'
    # Game ctor overrides the base vtable after the base ctor publishes the
    # same this pointer. Its whole pdata owner has inline data; certify only
    # the fully retained, contiguous constructor prefix, not that whole owner.
    start, end = 0x1AEC7D0, 0x1AEC80C
    assert im.owner(start)[0] == start
    assert all(im.valid[start-im.base:end-im.base])
    prefix = list(im.md.disasm(im.code[start-im.base:end-im.base], start))
    assert sum(x.size for x in prefix) == end-start
    pb = {x.address: x for x in prefix}
    assert pb[0x1AEC7F0].op_str == 'rbx, rcx'
    assert pb[0x1AEC7F3].op_str == '0x225be20'
    assert im.target(pb[0x1AEC802]) == 0x33FA9B8
    assert pb[0x1AEC809].op_str == 'qword ptr [rbx], rax'
    assert im.pointer(0x33FA9B8+0x68) == 0x1AF2700
    actual = im.instructions(0x1AF2700)
    assert len(actual) == 2 and actual[0].op_str == 'eax, 0x15200000' and actual[-1].mnemonic == 'ret'
    threshold = 0x15200000
    recipes = json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))['recipes']
    bow = json.loads((P/'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))['bow_recipes']
    checks = [{'file': row.get('file', 'armor.eq_crt'),
               'row': row.get('recipe_row', row.get('row')),
               'value': row['native_numeric_gate_u32'],
               'threshold_met': row['native_numeric_gate_u32'] <= threshold} for row in recipes+bow]
    assert all(row['threshold_met'] for row in checks)
    report = {'exe_sha256': im.meta['exe_sha256'], 'bindings': bindings,
              'initializer_sha256': hashlib.sha256(b''.join(x.bytes for x in im.instructions(0x1B783C0))).hexdigest(),
              'threshold_receiver': {'singleton': '0x51c4148', 'base_constructor': '0x225be20',
                                     'base_vtable': '0x35072f8', 'virtual_slot': '0x68',
                                     'base_getter': '0x225f1b0', 'base_field': '0xaf5ec', 'base_default': 256,
                                     'game_constructor': '0x1aec7d0', 'game_vtable': '0x33fa9b8',
                                     'game_getter': '0x1af2700', 'game_threshold': threshold},
              'game_constructor_prefix': [im.format(x) for x in prefix],
              'target_numeric_gate_checks': checks,
              'finding': 'Game vtable overrides the base getter with constant15200000; all inspected target numeric gates pass. Preserve the native comparison.',
              'limits': ['Base AF5EC default does not govern the certified game virtual dispatch.',
                         'Resource bindings identify retained tables; they do not prove every recipe is available.',
                         'Snapshot provenance uses complete initializer static constant dataflow, not target execution.',
                         'No native code or dynamic validation executed.']}
    (P/'research/horizon-other-recipe-resource-bindings.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/recipe-native-gates.json').write_text(json.dumps({
        'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': im.meta['exe_sha256'],
        'operation': 'preserve_native_gate', 'gate': '0x18bc560',
        'receiver_contract': report['threshold_receiver'], 'checks': checks,
        'limits': report['limits']}, indent=2), encoding='utf-8')
    print(json.dumps({'game_threshold': hex(threshold), 'target_checks': len(checks), 'all_met': True, 'gate_edits': 0}))


if __name__ == '__main__':
    main()
