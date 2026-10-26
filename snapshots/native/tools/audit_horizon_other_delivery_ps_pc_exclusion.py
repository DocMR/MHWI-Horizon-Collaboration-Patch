"""Compare actual PS/PC native delivery exclusion constants and callers."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from analyze_ps_stormbolt_types import PsCodeImage
from pc_unwind_groups import decode_group


def main():
    ps, pc = PsCodeImage(), CodeImage()
    codes = {}
    for address in (0x273D170, 0x2A8FC30):
        code = ps.instructions(address)
        assert code
        codes[address] = code
    # Loader owner roots are recovered by unwind; don't guess their starts.
    loader = ps.instructions(0x3EB73B1)
    assert loader
    by = {i.address: i for code in [*codes.values(), loader] for i in code}
    for address, operand in {
        0x3EB73C0: 'rsi, qword ptr [r12 + rbx*2]',
        0x3EB74AC: 'qword ptr [rbx + r13], r14',
        0x3EB7542: 'rbx, 8',
        0x3EB7546: 'rbx, 0xe78',
        0x273D194: 'r15d, dword ptr [rdi + 8]',
        0x273D19B: 'eax, [r15 - 0x21]',
        0x273D19F: 'eax, 0x27',
        0x273D1A2: '0x273d1b8',
        0x273D1AE: 'rcx, rax',
        0x273D1B2: '0x273d26f',
        0x2A8FC87: 'rax, qword ptr [rax + 0xc8]',
        0x2A8FC8E: 'rbx, qword ptr [rax + r12*8]',
        0x2A8FCA2: 'rdi, rbx',
        0x2A8FCA5: '0x273d170',
    }.items():
        assert by[address].op_str == operand, (hex(address), by[address].op_str)
    from capstone.x86 import X86_REG_RIP
    def target(i):
        return next(i.address+i.size+o.mem.disp for o in i.operands if o.type == 3 and o.mem.base == X86_REG_RIP)
    assert target(by[0x3EB73B1]) == 0x6F01FC0
    assert target(by[0x3EB73B8]) == 0x6F01148
    global_address = 0x6F01148+217*8
    assert global_address == target(by[0x2A8FC70]) == 0x6F01810
    descriptor = struct.unpack('<2Q', ps.read(0x67B5660+217*16, 16))
    assert descriptor == (0x6DF4538, 0x5983C54)
    assert ps.read(descriptor[1], 27) == b'common\\facility\\l_delivery\0'
    mask = by[0x273D1A4].operands[1].imm
    assert mask == 0x6FFFFFFFFF
    ps_ids = [33+bit for bit in range(39) if mask & (1 << bit)]
    pc_ids = list(struct.unpack_from('<38I', pc.exe, pc.raw(0x31CD970)))
    assert ps_ids == pc_ids and 57 in ps_ids
    assert by[0x273D26F].mnemonic == 'xor' and by[0x273D26F].op_str == 'eax, eax'
    _, pc_code = decode_group(pc, 0xF180A0)
    report = dict(status='PS/PC native delivery exclusion sets identical; retain original rejection',
        pc_exe_sha256=pc.meta['exe_sha256'], ps_eboot_sha256=hashlib.sha256(ps.data).hexdigest(),
        ps_resource=dict(descriptor_base='0x67b5660', descriptor_copy='0x6f01fc0',
            result_base='0x6f01148', index=217, global_rva=hex(global_address), type='0x6df4538'),
        ps_predicate=dict(function='0x273d170', id_native_offset=8, subtract=33,
                          unsigned_range=39, mask=hex(mask), reject='0x273d1b2 ->0x273d26f'),
        excluded_ids=ps_ids, pc_predicate='0xf180a0', pc_table='0x31cd970', target_id=57,
        ps_owners=[dict(start=hex(code[0].address), instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest())
            for code in [codes[0x273D170], codes[0x2A8FC30], loader]],
        pc_owner_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in pc_code)).hexdigest(),
        decision='Do not remove delivery57 exclusion as a platform-port difference; current source PS rejects it too. Continue current armor recipe/unlock path.',
        limits=['Does not certify every delivery gate or all alternative layered acquisition routes.',
                'Earlier raw consecutive integer segment was not the actual PS exclusion representation.',
                'Static instruction/data comparison only; no original mechanism executed.'])
    (P/'research/horizon-other-delivery-ps-pc-exclusion.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(exclusion_sets_identical=True, excluded_count=len(ps_ids), target_excluded_on_both=True)))


if __name__ == '__main__':
    main()
