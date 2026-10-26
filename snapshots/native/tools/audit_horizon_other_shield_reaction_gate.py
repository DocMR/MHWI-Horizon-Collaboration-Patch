"""Compare native reaction-local gates using static bytes, without target execution."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from analyze_ps_stormbolt_types import PsCodeImage
from capstone.x86 import X86_OP_IMM


def main():
    pc, ps = CodeImage(), PsCodeImage()
    a, z = 0x1217AC0, 0x1217B41
    raw = pc.code[a-pc.base:z-pc.base]
    assert all(pc.valid[a-pc.base:z-pc.base])
    body = list(pc.md.disasm(raw, a))
    assert sum(i.size for i in body) == len(raw)
    b = {i.address: i for i in body}
    expected = {
        0x1217AF8: 'r11, qword ptr [r10 + 0x7d20]',
        0x1217AFF: 'byte ptr [r11 + 0x660], 0',
        0x1217B09: 'byte ptr [r11 + 0x661], 0',
        0x1217B11: '0x1217b37',
        0x1217B13: 'ecx, dword ptr [r10 + 0x8950]',
        0x1217B2B: 'eax, 0x17',
        0x1217B32: 'r15b, 1',
        0x1217B3E: 'r15b, r15b',
    }
    for address, operand in expected.items():
        assert b[address].op_str == operand, pc.format(b[address])
    # This pdata range ends in selector/target tables. Decode the actual code
    # through RET, and validate every direct and table branch against boundaries.
    end, table, selectors, pdata_end = 0x1217DD9, 0x1217DDC, 0x1217E08, 0x1217E47
    native = pc.code[a-pc.base:pdata_end-pc.base]
    assert all(pc.valid[a-pc.base:pdata_end-pc.base])
    complete = list(pc.md.disasm(pc.code[a-pc.base:end-pc.base], a))
    assert sum(i.size for i in complete) == end-a
    cb = {i.address: i for i in complete}
    assert complete[-1].mnemonic == 'ret'
    assert not any('ymm' in i.op_str for i in complete)
    assert pc.code[0x1217B3E-pc.base:0x1217B44-pc.base].hex() == '4532ff8b5b10'
    assert pc.code[end-pc.base:table-pc.base].hex() == '0f1f00'
    targets = struct.unpack('<11I', pc.code[table-pc.base:selectors-pc.base])
    cases = list(pc.code[selectors-pc.base:pdata_end-pc.base])
    assert len(cases) == 63 and max(cases) < len(targets)
    assert all(t in cb for t in targets)
    for i in complete:
        if i.mnemonic.startswith('j') and i.operands[0].type == X86_OP_IMM:
            assert i.operands[0].imm in cb, pc.format(i)
    assert cb[0x1217BE0].op_str == 'eax, 0x3e'
    assert cb[0x1217BF0].op_str == 'eax, byte ptr [r8 + rax + 0x1217e08]'
    assert cb[0x1217BF9].op_str == 'edx, dword ptr [r8 + rax*4 + 0x1217ddc]'
    consumers = [i.address for i in complete if i.mnemonic == 'test' and i.op_str == 'r15b, r15b']
    assert consumers == [0x1217CA0, 0x1217CB3, 0x1217CF8]
    assert cb[0x1217DBD].op_str == 'dword ptr [rax + 0x10], ebx'
    assert cb[0x1217DC0].op_str == 'byte ptr [rax + 0x1f], bpl'
    owner = ps.owner(0x2CE08A2)
    full = ps.instructions(owner[0])
    assert len(full) == 2976
    pb = {i.address: i for i in full}
    for address, operand in {
        0x2CE0607: 'byte ptr [r14 + 0x660], 0',
        0x2CE0611: 'byte ptr [r14 + 0x661], 0',
        0x2CE0619: '0x2ce08a2',
        0x2CE08C0: 'rcx, dword ptr [rcx + 0x141c]',
        0x2CE08E2: 'word ptr [rcx + 0x24], 0',
        0x2CE08FB: 'ecx, dword ptr [rdx + 0xa8c]',
        0x2CE0901: '0x2ce061f',
        0x2CE0907: '0x2ce0628',
        0x2CE0628: 'r10d, r10d',
    }.items():
        assert pb[address].op_str == operand, ps.format(pb[address])
    windows = [(0x2CE05FC, 0x2CE0643), (0x2CE08A2, 0x2CE090C)]
    report = {
        'status': 'native_local_predicate_difference_certified_hook_not_installed',
        'pc_exe_sha256': pc.meta['exe_sha256'],
        'pc_window': {'start': hex(a), 'end': hex(z),
            'sha256': hashlib.sha256(raw).hexdigest(),
            'instructions': [pc.format(i) for i in body]},
        'pc_full_owner': {'start': hex(a), 'code_end': hex(end),
            'pdata_end': hex(pdata_end), 'guard_bytes': len(native),
            'sha256': hashlib.sha256(native).hexdigest(),
            'instruction_count': len(complete),
            'instructions': [pc.format(i) for i in complete],
            'target_table': hex(table), 'selector_table': hex(selectors),
            'targets': [hex(t) for t in targets],
            'cases': [{'input': n, 'target': hex(targets[c])} for n, c in enumerate(cases)],
            'local_result_consumers': [hex(v) for v in consumers],
            'result_writes': ['output argument+10 receives EBX category', 'output argument+1F receives BPL flag']},
        'ps_full_owner': {'bounds': [hex(v) for v in owner],
            'instruction_count': len(full),
            'sha256': hashlib.sha256(ps.read(owner[0], owner[1]-owner[0])).hexdigest()},
        'ps_windows': [[ps.format(i) for i in full if lo <= i.address < hi]
                       for lo, hi in windows],
        'pc_local_result': 'R15b = (status660 != 0 OR status661 != 0) AND actor8950.bit31 AND NOT actor8948.bit23',
        'ps_local_result': 'R10b = (status660 != 0 OR status661 != 0 OR eligiblePowerCellNotExhausted) AND actor8700.bit31 AND NOT actor86F8.bit23',
        'ps_extra_gate': 'Only if both status bytes zero: status+8 actor nonnull, actor60.bit0, selected skill209 record ordinal nonzero, signed record24 > 0, count7A8 != resourceA8C. Failure goes to zero-result block.',
        'candidate_patch_boundary': 'PC 1217B11 JE currently selects zero-result when both bytes are zero. A future bridge must supply the missing condition and preserve the existing actor flag gates; replacing whole native function return is not equivalent.',
        'limits': ['Full PS owner and PC code/inline-table separation certified. Target table entries and direct branches all land on decoded code boundaries.',
            'Register-local predicate pairing does not yet prove all consumers or final reaction outcomes.',
            'No runtime hook or native execution. No game-directory writes.'],
    }
    (P/'research/horizon-other-shield-reaction-gate.json').write_text(
        json.dumps(report, indent=2), encoding='utf-8')
    digest = hashlib.sha256(native).digest()
    generated = '// Generated full reaction owner including native inline tables.\n'
    generated += 'constexpr TreeOwnerSpec kShieldReactionOwner = {' + f'0x{a:X}, {len(native)}, '
    generated += '{' + ', '.join(f'0x{x:02X}' for x in digest) + '}};\n'
    (P/'staging/Horizon-other-module/src/generated_shield_reaction_owner.inc').write_text(generated, encoding='utf-8')
    print(f'PC full owner certified: {len(complete)} instructions, {len(targets)} targets, {len(cases)} selectors, {len(native)} guarded bytes; three local consumers.')


if __name__ == '__main__':
    main()
