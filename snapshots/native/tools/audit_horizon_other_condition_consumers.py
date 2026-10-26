"""Find static field consumers without treating matching offsets as semantics."""
from pathlib import Path
import json
import re
import struct
from analyze_hzd_code import CodeImage, X86_OP_MEM

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    rows, seen = [], set()
    patterns = [(0x4a, rb'\x0f[\xb7\xbf][\x40-\x7f]\x4a')]
    patterns += [(d, re.escape(struct.pack('<I', d))) for d in (0xfa, 0x29456, 0xe6)]
    for displacement, pattern in patterns:
        for match in re.finditer(pattern, image.code):
            address = image.base + match.start()
            owner = image.owner(address)
            if not owner or (owner[0], displacement) in seen:
                continue
            start, end = owner[:2]
            if not all(image.valid[start-image.base:end-image.base]):
                continue
            instructions = image.instructions(address)
            complete = bool(instructions)
            if not complete:
                instructions = list(image.md.disasm(image.code[start-image.base:end-image.base], start))
            hits = [k for k, instruction in enumerate(instructions)
                    if any(op.type == X86_OP_MEM and op.mem.disp == displacement and op.size == 2
                           for op in instruction.operands)]
            if not hits:
                continue
            seen.add((start, displacement))
            for index in hits:
                rows.append({'displacement': hex(displacement), 'function': hex(start),
                             'address': hex(instructions[index].address), 'whole_fragment_decoded': complete,
                             'context': [image.format(i) for i in instructions[max(0, index-8):index+20]]})
    excluded = {'0x197e4c0': 'Equipment text lookup: equipment manager5011F00, message resource lookup1B783B0, tail2244930.',
                '0x197e5e0': 'Equipment text lookup, not quest admission.',
                '0x1e45ae0': 'UI string formatting of manager fields; no rank comparison in captured context.'}
    report = {'exe_sha256': image.meta['exe_sha256'], 'candidates': rows,
              'excluded_semantics': excluded,
              'limits': ['Offset matches do not establish quest-object provenance.',
                         'Search covers selected direct WORD accesses, not pointer aliases, copied structures or every encoding.',
                         'Partial fragment decoding may encounter inline data; never use these candidates directly as patch sites.',
                         'No native code executed; full admission predicate remains unresolved.']}
    (P/'research/horizon-other-condition-consumer-audit.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'field_access_candidates': len(rows), 'semantic_exclusions': len(excluded)}))


if __name__ == '__main__':
    main()
