"""Bind source condition codes to native rank requirement labels, statically."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage
from build_horizon_other_text_candidate import parse

P = Path(__file__).resolve().parents[1]


def main():
    image = CodeImage()
    start, end = 0x193AD50, 0x193AD94
    raw = image.code[start-image.base:end-image.base]
    assert all(image.valid[start-image.base:end-image.base])
    code = list(image.md.disasm(raw, start))
    assert sum(i.size for i in code) == end-start and code[-1].mnemonic == 'ret'
    by = {i.address: i for i in code}
    pins = {0x193AD52: 'ecx, 1', 0x193AD58: 'eax, 0x3a', 0x193AD5D: 'eax, 0x16',
            0x193AD62: 'ecx, 0x1d2', 0x193AD74: 'edx, 0x607',
            0x193AD79: 'eax, 0x30', 0x193AD7C: 'ecx, 0x630'}
    for address, expected in pins.items():
        assert by[address].op_str == expected
    assert [i.operands[0].imm for i in code if i.mnemonic == 'jmp'] == [0x18BBDB0, 0x18BBDB0]
    # Case4 of native text getter resolves the common text resource at5012DE0.
    target = struct.unpack_from('<I', image.code, 0x18BBEE8-image.base+4*4)[0]
    assert target == 0x18BBE17
    branch = next(image.md.disasm(image.code[target-image.base:target-image.base+7], target))
    assert image.target(branch) == 0x5012DE0
    setup = {i.address: i for i in image.instructions(0x1B783C0)}
    assert image.target(setup[0x1B7AEDF]) == 0x3402808
    assert setup[0x1B7AEE6].op_str == 'qword ptr [rsp + 0x48], rax'
    path_offset = image.raw(0x3402808)
    assert image.exe[path_offset:path_offset+28].startswith(b'common\\text\\cm_facility_jpn\0')
    source = P/'MHWI解包/chunk/common/text/cm_facility_eng.gmd'
    data = source.read_bytes()
    _, values, keys = parse(data)
    specs = [(0, 'none', 0, 466, 'None'),
             (6, 'hunter', 6, 472, 'HR 6 or higher'),
             (11, 'hunter', 11, 477, 'HR 11 or higher'),
             (20, 'hunter', 50, 486, 'HR 50 or higher'),
             (45, 'master', 24, 1588, 'MR 24 or higher')]
    labels = []
    for condition, kind, minimum, index, expected in specs:
        assert values[index].decode('utf-8') == expected
        assert index == (0x1D2+condition if condition < 22 else 0x607+condition)
        labels.append({'condition': condition, 'kind': kind, 'minimum': minimum, 'text_index': index,
                       'text_key': keys[index].decode('utf-8'), 'label': expected})
    report = {'exe_sha256': image.meta['exe_sha256'], 'label_function': hex(start),
              'label_function_bytes': len(raw), 'label_function_sha256': hashlib.sha256(raw).hexdigest(),
              'text_file': str(source.relative_to(P)), 'text_sha256': hashlib.sha256(data).hexdigest(),
              'labels': labels, 'instructions': [image.format(i) for i in code],
              'limits': ['This proves native displayed rank lower bounds, not the full admission predicate.',
                         'Other story/ownership/prerequisite rules must still be evaluated.',
                         'No target code or newly implemented rank checker executed.']}
    (P/'research/horizon-other-rank-condition-labels.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    out = ['// Generated native condition labels; lower bounds only, no quest unlock grant.']
    for row in labels:
        out.append(f'    {{{row["condition"]}, RankKind::{row["kind"]}, {row["minimum"]}}},')
    (P/'staging/Horizon-other-module/src/generated_rank_conditions.inc').write_text('\n'.join(out)+'\n', encoding='utf-8')
    print(json.dumps({'labels': labels}, ensure_ascii=False))


if __name__ == '__main__':
    main()
