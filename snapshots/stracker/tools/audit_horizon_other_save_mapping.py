"""Inspect native quest mapping references and four serialized state bitmaps."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage
from scan_horizon_other_mapping_refs import mapping_references
from capstone.x86 import X86_REG_RBP, X86_REG_EBP

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    base = 0x324FAC0
    ids = []
    while True:
        value = struct.unpack_from('<i', image.exe, image.raw(base+4*len(ids)))[0]
        if value == -1:
            break
        ids.append(value)
        assert len(ids) < 512
    targets = {base+4*i for i in range(len(ids)+1)}
    references = []
    for address, target in mapping_references(image, targets):
        offset = address-image.base
        ins = next(image.md.disasm(image.code[offset:offset+15], address, count=1))
        assert image.target(ins) == target
        assert all(image.valid[offset:offset+ins.size])
        references.append({'rva': hex(address), 'target': hex(target), 'index_offset': (target-base)//4,
                           'instruction': image.format(ins), 'expected_hex': ins.bytes.hex(),
                           'owner': hex(image.owner(address)[0]) if image.owner(address) else None})
    absolute = []
    for name, _, _, size, raw_offset in image.sections:
        if name not in ('.rdata', '.data'):
            continue
        data = image.exe[raw_offset:raw_offset+size]
        for n, (pointer,) in enumerate(struct.iter_unpack('<Q', data[:len(data)//8*8])):
            if image.imagebase+base <= pointer <= image.imagebase+base+4*len(ids):
                absolute.append({'section': name, 'raw_offset': raw_offset+8*n,
                                 'target': hex(pointer-image.imagebase)})
    serialization = []
    for fn, loop, exit_, end, slot in [(0x1361D90, 0x1362D20, 0x1362D39, 0x1362DC5, 0x70),
                                      (0x136BEF0, 0x136CDD0, 0x136CDE9, 0x136CE75, 0x60)]:
        xs = image.instructions(fn)
        assert xs
        initial = [i for i in xs if loop-40 <= i.address < loop]
        assert any(i.mnemonic == 'mov' and i.op_str == 'r13d, 5' for i in initial)
        assert any(i.mnemonic == 'mov' and i.op_str == 'ebp, r13d' for i in initial)
        body = [i for i in xs if loop <= i.address < exit_]
        assert any(i.mnemonic == 'sub' and i.op_str == 'rbp, 1' for i in body)
        assert body[-1].mnemonic == 'jne' and body[-1].operands[0].imm == loop
        for i in xs:
            if exit_ <= i.address < end:
                _, written = i.regs_access()
                assert X86_REG_RBP not in written and X86_REG_EBP not in written
        fields = []
        for index, i in enumerate(xs):
            if not exit_ <= i.address < end or i.mnemonic != 'lea':
                continue
            for field in (0x104BD8, 0x104C20, 0x104C68, 0x104CB0):
                if i.operands[1].mem.disp != field:
                    continue
                following = xs[index+1:index+5]
                assert following[0].op_str == 'r8d, [rbp + 0x40]'
                assert any(j.mnemonic == 'call' and j.op_str == f'qword ptr [rax + {hex(slot)}]' for j in following)
                fields.append({'save_offset': hex(field), 'bytes': 64,
                               'instructions': [image.format(j) for j in [i]+following]})
        assert len(fields) == 4
        serialization.append({'function': hex(fn), 'direction': 'read' if slot == 0x70 else 'write', 'fields': fields})
    quests = json.loads((P/'research/horizon-other-quest-classification.json').read_text(encoding='utf-8'))['quests']
    assert all(q['quest_id'] not in ids for q in quests)
    extended = ids+[q['quest_id'] for q in quests]+[-1]
    assert len(extended)-1 <= 512
    candidate = {'schema_version': 1, 'module': 'horizon-other', 'exe_sha256': image.meta['exe_sha256'],
                 'original_mapping_rva': hex(base), 'original_count': len(ids),
                 'original_bytes_sha256': hashlib.sha256(struct.pack('<'+str(len(ids)+1)+'i', *(ids+[-1]))).hexdigest(),
                 'candidate_entries': extended, 'candidate_count': len(extended)-1,
                 'new_index_candidates': [{'quest_id': q['quest_id'], 'index': len(ids)+i} for i, q in enumerate(quests)],
                 'native_state_fields': {'open': '0x104BD8', 'completion': '0x104C20',
                                         'companion_state': '0x104C68',
                                         'late_join_sos_state': '0x104CB0',
                                         'completion_counter': '0x1484B4'},
                 'late_join_evidence': 'research/horizon-other-sos-notification.json',
                 'installation_allowed': False,
                 'reason': 'Serialized capacity is proven, but index ownership, all indirect consumers and prerequisite seeding still require certification.',
                 'integration_rule': 'If a certified extended mapping replaces native count/at consumers, remove the separate +6 count/at wrappers to avoid double enumeration.'}
    report = {**candidate, 'aligned_rdata_data_absolute_references': absolute,
              'mapping_range_rip_references': references, 'serialization': serialization,
              'limits': ['RIP scan covers implemented REX/non-REX LEA/MOV patterns and targets inside the mapping range, not arbitrary indirect address construction or all possible instruction encodings.',
                         'Aligned .rdata/.data pointer scan is not a universal pointer-provenance proof.',
                         'Four bitmap capacities and 512 counters do not establish free-index ownership or valid success/prerequisite semantics.',
                         'No in-game memory, save state or save file written.']}
    (P/'research/horizon-other-save-mapping-audit.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/quest-save-map-candidate.json').write_text(json.dumps(candidate, ensure_ascii=False, indent=2), encoding='utf-8')
    header = ['#pragma once', '#include <cstdint>', '#include <cstddef>',
              '// Candidate image only: native save-index ownership is NOT yet certified.',
              'namespace horizon_other { namespace generated {',
              f'inline constexpr std::size_t native_quest_count = {len(ids)};',
              'inline constexpr std::int32_t quest_mapping_candidate[] = {']
    for offset in range(0, len(extended), 12):
        header.append('    '+', '.join(str(value) for value in extended[offset:offset+12])+',')
    header.extend(['};', '} }', ''])
    (P/'staging/Horizon-other-module/src/generated_quest_save_mapping.h').write_text('\n'.join(header), encoding='utf-8')
    redirects=['// Generated exact RIP-relative mapping references.',
               'constexpr MappingReference kMappingReferences[] = {']
    for row in references:
        address=int(row['rva'],16)
        ins=next(image.md.disasm(image.code[address-image.base:address-image.base+15],address,count=1))
        assert ins.disp_size == 4 and ins.size <= 7
        raw=list(ins.bytes)+[0]*(7-ins.size)
        redirects.append('    {'+f'0x{address:X}, {ins.size}, {ins.disp_offset}, {row["index_offset"]}, '+
                         '{'+', '.join(f'0x{x:02X}' for x in raw)+'}},')
    redirects+=['};','']
    (P/'staging/Horizon-other-module/src/generated_quest_mapping_references.inc').write_text('\n'.join(redirects),encoding='utf-8')
    print(json.dumps({'native_count': len(ids), 'rip_references': len(references),
                      'interior_references': sum(r['index_offset'] != 0 for r in references),
                      'aligned_absolute_refs': len(absolute), 'serialized_bitmaps': 4,
                      'candidate_count': len(extended)-1, 'installation_allowed': False}))


if __name__ == '__main__':
    main()
