"""Generate original code, relocation and relocated CHAININFO metadata only."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P, X86_OP_IMM
from pc_unwind_groups import decode_group


def align(value):
    return (value+15) & ~15


def main():
    im = CodeImage()
    reports, output = [], ['// Generated native clone assets; no executable code is run by this generator.']
    for number, start in enumerate((0x13732F0, 0x13738B0, 0xBCD610)):
        fragments, instructions = decode_group(im, start)
        end = fragments[-1][1]
        assert fragments[0][0] == start
        assert sum(i.size for i in instructions) == end-start
        assert all(a[1] == b[0] for a, b in zip(fragments, fragments[1:]))
        cursor = start
        for ins in instructions:
            assert ins.address == cursor
            cursor += ins.size
        raw = im.code[start-im.base:end-im.base]
        assert all(im.valid[start-im.base:end-im.base])
        relocations = []
        for ins in instructions:
            target = im.target(ins)
            if target is not None:
                assert ins.disp_size == 4
                relocations.append((ins.address-start+ins.disp_offset, ins.address-start+ins.size, target))
            elif (ins.mnemonic == 'call' or ins.mnemonic.startswith('j')) and ins.operands and ins.operands[0].type == X86_OP_IMM:
                target = ins.operands[0].imm
                if start <= target < end:
                    continue
                assert ins.imm_size == 4, 'External short branch needs a different clone strategy'
                relocations.append((ins.address-start+ins.imm_offset, ins.address-start+ins.size, target))
        for offset, next_offset, target in relocations:
            assert 0 <= offset <= next_offset-4 and next_offset <= len(raw)
            assert not start <= target < end, 'Internal RIP reference needs clone-relative handling'
        module = P/'staging/Horizon-other-module'
        edits = json.loads((module/'merge/weapon-tree-edits.json').read_text(encoding='utf-8'))['edits']
        palico = json.loads((module/'merge/palico-exclusion-edits.json').read_text(encoding='utf-8'))
        edits += [dict(e, owner_rva=palico['owner_rva']) for e in palico['edits']]
        for edit in edits:
            if int(edit['owner_rva'], 16) != start:
                continue
            a = int(edit['rva'], 16)-start
            z = a+len(bytes.fromhex(edit['expected_hex']))
            assert raw[a:z] == bytes.fromhex(edit['expected_hex'])
            assert all(z <= off or a >= off+4 for off, _, _ in relocations), 'Relocation would overwrite module edit'
        payload = bytearray(raw)
        unwind_map, unwind_rows = {}, []
        for fragment in fragments:
            rva = fragment[2]
            if rva in unwind_map:
                continue
            at = im.raw(rva)
            header = im.exe[at:at+4]
            version, flags = header[0] & 7, header[0] >> 3
            assert version == 1 and flags in (0, 4), 'Handler metadata needs a separate strategy'
            size = 4+((header[2]+1)//2)*4
            size += 12 if flags == 4 else 0
            offset = align(len(payload))
            payload.extend(bytes(offset-len(payload)))
            unwind_map[rva] = offset
            payload.extend(im.exe[at:at+size])
            unwind_rows.append((rva, offset, size, flags))
        for rva, offset, size, flags in unwind_rows:
            if flags != 4:
                continue
            chain = struct.unpack_from('<3I', payload, offset+size-12)
            assert chain in fragments and chain[2] in unwind_map
            struct.pack_into('<3I', payload, offset+size-12,
                             chain[0]-start, chain[1]-start, unwind_map[chain[2]])
        table_offset = align(len(payload))
        payload.extend(bytes(table_offset-len(payload)))
        for a, z, unwind in fragments:
            payload.extend(struct.pack('<3I', a-start, z-start, unwind_map[unwind]))
        assert len(payload) <= 4096
        label = f'kClone{number}'
        output.append(f'constexpr std::uint8_t {label}Bytes[] = '+'{')
        output += [', '.join(f'0x{b:02X}' for b in payload[o:o+16])+',' for o in range(0, len(payload), 16)]
        output += ['};', f'constexpr NativeRelocation {label}Relocations[] = '+'{']
        output += ['    {'+f'{off}, {nxt}, 0x{target:X}'+'},' for off, nxt, target in relocations]
        output += ['};']
        reports.append({'rva': start, 'code_bytes': len(raw), 'payload_bytes': len(payload),
                        'original_sha256': hashlib.sha256(raw).hexdigest(),
                        'module_edit_relocation_overlap': False,
                        'relocations': relocations, 'runtime_function_table_offset': table_offset,
                        'runtime_function_count': len(fragments), 'unwind_fragments': [list(f) for f in fragments]})
    output += ['constexpr NativeCloneSpec kNativeClones[] = {']
    for n, row in enumerate(reports):
        output.append('    {'+f'0x{row["rva"]:X}, {row["code_bytes"]}, kClone{n}Bytes, sizeof(kClone{n}Bytes), '+
                      f'kClone{n}Relocations, sizeof(kClone{n}Relocations)/sizeof(kClone{n}Relocations[0]), '+
                      f'{row["runtime_function_table_offset"]}, {row["runtime_function_count"]}'+'},')
    output += ['};', '']
    module = P/'staging/Horizon-other-module'
    (module/'src/generated_native_clone_specs.inc').write_text('\n'.join(output), encoding='utf-8')
    report = {'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': im.meta['exe_sha256'],
              'clones': reports,
              'limits': ['Metadata and static relocation descriptions only; no native code executed.',
                         'Host must attest the game build and exact original code before preparation.',
                         'No exception handler unwind records supported; three-fragment CHAININFO is relocated explicitly.']}
    for target in (P/'research/horizon-other-native-clone-specs.json', module/'merge/native-clone-specs.json'):
        target.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'clones': len(reports), 'unwind_fragments': sum(r['runtime_function_count'] for r in reports)}))


if __name__ == '__main__':
    main()
