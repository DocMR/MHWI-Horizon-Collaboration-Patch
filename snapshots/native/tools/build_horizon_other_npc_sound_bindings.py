"""Generate source guards for the PS-only NPC costume81 sound-path branch."""
import hashlib
import struct
from analyze_hzd_code import CodeImage, P
from analyze_ps_stormbolt_types import PsCodeImage
from pc_unwind_groups import decode_group


def main():
    pc, ps = CodeImage(), PsCodeImage()
    ps_body = ps.instructions(0x3A8D120)
    assert ps_body and ps.read(0x596CC14, 7) == b'_ot081\0'
    for table in (0x5830184, 0x58300C8):
        assert table+struct.unpack('<i', ps.read(table+(81-35)*4, 4))[0] == 0x3A8D3E1
    ps_by = {i.address: i for i in ps_body}
    assert ps_by[0x3A8D1F7].op_str == 'eax, -0x23'
    assert ps_by[0x3A8D2E8].op_str == 'eax, -0x23'
    fragments, body = decode_group(pc, 0x18E7B50)
    by = {i.address: i for i in body}
    for address, operand in {0x18E7B5D:'edx, 2', 0x18E7B82:'eax, [rdx - 0x62]',
        0x18E7B8E:'ebx, [rdx + 0x67]', 0x18E7C8F:'eax, [rdx - 0x59]',
        0x18E7C9B:'ebx, [rdx + 0x70]', 0x18E7BEA:'0x191a6c0',
        0x18E7D00:'0x191a6c0', 0x18E7BF4:'ebx, dword ptr [rax + 0x1c]',
        0x18E7D0A:'ebx, dword ptr [rax + 0x1c]'}.items():
        assert by[address].op_str == operand
    assert pc.exe[pc.raw(0x33BCAE0):pc.raw(0x33BCAE0)+7] == b'_ot036\0'
    spans = [(min(f[0] for f in fragments), max(f[1] for f in fragments))]
    for address in (0x5F9D40, 0x2C468C0, 0x24C380):
        fs, _ = decode_group(pc, address)
        spans.append((min(f[0] for f in fs), max(f[1] for f in fs)))
    # Bounded tail-call helper: early return for -1, otherwise original lookup.
    start = 0x191A6C0; end = start+34
    tail = list(pc.md.disasm(pc.code[start-pc.base:end-pc.base], start))
    assert sum(i.size for i in tail) == 34 and tail[-1].mnemonic == 'jmp'
    assert tail[-1].address+tail[-1].size == end
    spans.append((start, end))
    rows = []
    for start, end in spans:
        assert 0 < end-start <= 16384
        data = pc.code[start-pc.base:end-pc.base]
        assert len(data) == end-start and all(pc.valid[start-pc.base:end-pc.base])
        digest = ','.join(f'0x{x:02x}' for x in hashlib.sha256(data).digest())
        rows.append(f'    {{0x{start:X}, {end-start}, {{{digest}}}}}')
    output = P/'staging/Horizon-other-module/src/generated_npc_sound_owners.inc'
    output.write_text('const TreeOwnerSpec kNpcSoundOwners[] = {\n'+',\n'.join(rows)+'\n};\n', encoding='utf-8')
    print('NPC sound guards generated; PS costume81 selection in both branches; no execution.')


if __name__ == '__main__':
    main()
