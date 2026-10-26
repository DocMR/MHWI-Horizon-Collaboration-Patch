"""Export certified original ABI/code fragments for the composable quest bridge."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage
from pc_unwind_groups import decode_group

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    fragments, reader = decode_group(image, 0x478A20)
    by_address = {i.address: i for i in reader}
    pins = {0x478A6D: 'lea rbx, [rbp + 0xb0]',
            0x478ACA: 'mov edx, 0x10', 0x478AD8: 'call 0x32a350',
            0x478AE5: 'call 0x32a330', 0x478AF1: 'mov eax, dword ptr [rbx + 8]',
            0x478AF4: 'mov edx, dword ptr [rbx + 0xc]',
            0x478B05: 'call 0x249b70', 0x478B0D: 'mov rax, qword ptr [rbx + 0x18]',
            0x478B19: 'mov qword ptr [rax + rcx*8], rdi'}
    for address, expected in pins.items():
        ins = by_address[address]
        assert ins.mnemonic+' '+ins.op_str == expected
    constructor = image.instructions(0x32A330)
    assert constructor and constructor[-2].op_str == 'rax, rcx'
    assert image.target(constructor[0]) == 0x2F229C8
    assert image.pointer(0x2F229C8) == 0x32A3B0
    destructor = image.instructions(0x32A3B0)
    assert destructor and any(i.op_str == 'dl, 1' for i in destructor)
    mapping = []
    offset = image.raw(0x324FAC0)
    while True:
        value = struct.unpack_from('<i', image.exe, offset+4*len(mapping))[0]
        if value == -1:
            break
        mapping.append(value)
        assert len(mapping) <= 512
    quests = json.loads((P/'research/horizon-other-quest-classification.json').read_text(encoding='utf-8'))['quests']
    assert all(q['quest_id'] not in mapping for q in quests)
    guards = []
    for address in (0x32A330, 0x32A350, 0x32A3B0, 0x249B70, 0x1355C40, 0x1355C50):
        instructions = image.instructions(address)
        assert instructions
        data = b''.join(i.bytes for i in instructions)
        guards.append({'rva': hex(address), 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                       'expected_hex': data.hex(), 'instructions': [image.format(i) for i in instructions]})
    report = {'exe_sha256': image.meta['exe_sha256'],
              'native_optional_mapping_count': len(mapping), 'catalog_ids_absent_from_native_mapping': True,
              'list_resource_reader': {'rva': '0x478a20', 'chained_fragments': len(fragments),
                                       'instructions': [image.format(i) for i in reader]},
              'code_blocks': guards,
              'resource_layout': {'name': '0xc', 'array': '0xb0', 'count': '0xb8',
                                  'capacity': '0xbc', 'entries': '0xc8', 'quest_id_after_mib_load': '0xe0'},
              'abi': {'allocate': '32A350: void*(size_t,unsigned alignment)',
                      'construct': '32A330: QuestNoNode*(void*); nonnull input always returned',
                      'destroy': 'node vtable slot0(QuestNoNode*,1), native scalar deleting destructor32A3B0',
                      'grow': '249B70: void(native_array*,uint32 capacity)',
                      'optional_count': '1355C50: int()', 'optional_at': '1355C40: int(void*,int)'},
              'integration': ['Use a single shared quest loader hook owner after original deserialization succeeds.',
                              'Bind only after shared bootstrap certifies original EXE/code and native allocator lifetime.',
                              'append_quest_resource_nodes deduplicates the six IDs and commits node pointers/count after preparation.',
                              'Do not retain the old two-quest optional-count hooks beside the six-quest hooks.',
                              'Open checks, star checks, source prerequisites and save mapping are not bypassed or implemented by this component.'],
              'limits': ['No native instruction or newly compiled library executed.',
                         'Six-task registration is partial until hooks/bootstrap/open/save integration are finished.']}
    (P/'research/horizon-other-quest-bridge-api.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    interface = {k: v for k, v in report.items() if k != 'list_resource_reader'}
    interface['code_blocks'] = [{k: v for k, v in r.items() if k != 'instructions'} for r in guards]
    (P/'staging/Horizon-other-module/merge/quest-bridge-interface.json').write_text(json.dumps(interface, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'list_reader_fragments': len(fragments), 'native_mapping_count': len(mapping),
                      'native_api_blocks': len(guards), 'six_ids_not_in_original_mapping': True}))


if __name__ == '__main__':
    main()
