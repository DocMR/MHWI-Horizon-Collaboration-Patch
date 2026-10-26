"""Certify delivery217 resource, scalar field copy and retained exclusion gate."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    descriptors = json.loads((P/'research/equip-descriptor-separate-snapshots.json').read_text(encoding='utf-8'))['tables'][0]
    descriptor = next(r for r in descriptors['entries'] if r['index'] == 217)
    assert descriptor['path'] == 'common\\facility\\l_delivery'
    assert int(descriptor['type'], 16) == 0x4F857D8
    assert int(descriptor['result_global'], 16) == 0x5012608
    assert im.pointer(0x31DE368+0x50) == 0xF6D8C0
    owners, by = {}, {}
    for address in (0xF6D8C0, 0xF180A0, 0xF16E00):
        fragments, code = decode_group(im, address)
        owners[hex(address)] = dict(fragments=fragments, instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest())
        by.update({i.address: i for i in code})
    checks = {
        0xF6D900: 'ecx, 0x10d',
        0xF6DA3B: 'rdx, [rbp - 0x38]',
        0xF6DA54: '0xbcc5e0',
        0xF180B6: 'ecx, dword ptr [rcx + 8]',
        0xF180C4: '0xf181b7',
        0xF180D0: 'edx, 0x26',
        0xF18105: 'eax, dword ptr [rax + rcx*4 + 0x102428]',
        0xF16E3D: '0xf180a0',
        0xF16E4D: 'edi, dword ptr [rdi + 0x20]',
    }
    for address, operand in checks.items():
        assert by[address].op_str == operand
    assert im.target(by[0xF180B9]) == 0x31CD970
    assert im.target(by[0xF16E0F]) == 0x5012608
    copy = im.instructions(0xBCC5E0)
    assert len(copy) == 23 and copy[-1].mnemonic == 'ret'
    native_offsets = (8, 0x24, 0x20, 0x28, 0x30, 0x2C, 0xC, 0x10, 0x14, 0x18, 0x1C)
    for index, destination in enumerate(native_offsets):
        load, store = copy[2*index:2*index+2]
        assert load.mnemonic == store.mnemonic == 'mov'
        assert load.operands[1].mem.disp == 4*index
        assert store.operands[0].mem.disp == destination
        assert load.operands[1].size == store.operands[0].size == 4
    table = P/'research/horizon-other-source/common/facility/l_delivery.stmp'
    raw = table.read_bytes()
    assert raw == (P/'MHWI解包/chunk/common/facility/l_delivery.stmp').read_bytes()
    assert struct.unpack_from('<IHI', raw) == (0x18091001, 0x10D, 107)
    records = [struct.unpack_from('<11I', raw, 10+44*i) for i in range(107)]
    assert all(row[0] == i for i, row in enumerate(records))
    row = records[57]
    assert row == (57, 1269, 1235, 4, 0, 0, 5000, 976, 927, 3, 2)
    excluded = struct.unpack_from('<38I', im.exe, im.raw(0x31CD970))
    assert excluded.count(57) == 1
    report = dict(status='Static delivery field flow and PC exclusion; platform difference not yet certified',
        exe_sha256=im.meta['exe_sha256'], descriptor=descriptor,
        resource=dict(name='rLeadersDelivery', instance_vtable='0x31de368', reader='0xf6d8c0'),
        owners=owners, copy=dict(function='0xbcc5e0', native_offsets=list(native_offsets),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in copy)).hexdigest()),
        target=dict(serialized_row=57, serialized_id=57, serialized_dwords=list(row),
                    native_fields={hex(destination): value for destination, value in zip(native_offsets, row)}),
        pc_exclusion=dict(function='0xf180a0', table='0x31cd970', ids=list(excluded),
            matched_branch='0xf180c4 -> original false path0xf181b7',
            known_consumer='0xf16e00 ->0xf16e3d, then conditional original bitmap read'),
        limits=['PC delivery ID57 is rejected before later native bitmap gates; this does not prove PS accepts it.',
                'PS counterpart and current full-set/layered acquisition alternatives must be compared before altering obsolete delivery gates.',
                'Serialized ID equals row across this source table, but native text lookup/quest identity is not inferred solely from that equality.',
                'Remaining raw DWORD meanings and material/cost/payment/completion are not certified here.',
                'No gate changed, no native execution, no dynamic test or game-directory write.'])
    (P/'research/horizon-other-delivery-native-fields.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(delivery_records=len(records), copied_fields=11, pc_excludes_target=True,
                         predicate_instructions=owners['0xf180a0']['instructions'])))


if __name__ == '__main__':
    main()
