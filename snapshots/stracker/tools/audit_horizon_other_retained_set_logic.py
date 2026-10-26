"""Identify retained native set matching; do not remove it as a platform gate."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P


def main():
    im = CodeImage()
    start, end = 0x1672160, 0x1672175
    raw = im.code[start-im.base:end-im.base]
    assert all(im.valid[start-im.base:end-im.base])
    wrapper = list(im.md.disasm(raw, start))
    assert sum(x.size for x in wrapper) == end-start
    assert wrapper[0].op_str == 'edx, ecx'
    assert im.target(wrapper[1]) == 0x5011F00
    assert wrapper[2].op_str == 'rcx, 0x127e8' and wrapper[-1].op_str == '0x2798a0'
    lookup = im.instructions(0x2798A0)
    assert lookup and lookup[1].op_str == 'rdx, [rax + rax*8]'
    assert lookup[2].op_str == 'rdx, 4' and lookup[3].op_str == 'rax, [rcx + 0x119a8]'
    matcher, consumer = im.instructions(0xEE2BD0), im.instructions(0xEE30C0)
    assert matcher and consumer
    mb, cb = {x.address: x for x in matcher}, {x.address: x for x in consumer}
    pins = {0xEE2BDD: 'ecx, 0xd7', 0xEE2BE2: '0x1672160',
            0xEE2C2C: 'ecx, 0xf2', 0xEE2C31: '0x1672160'}
    for address, operand in pins.items():
        assert mb[address].op_str == operand
    for address, operand in {0xEE3127: '0xee2bd0', 0xEE31C5: 'r8d, 0x9a',
                             0xEE31CE: 'r8d, 0xd7', 0xEE31D7: 'r8d, 0xf2',
                             0xEE31E0: 'eax, [r8 - 0x1d9]',
                             0xEE3342: 'bl, bl'}.items():
        assert cb[address].op_str == operand
    assert any(x.mnemonic == 'mov' and x.op_str == 'ecx, 0x1d9' for x in matcher)
    report = {'exe_sha256': im.meta['exe_sha256'],
              'set_lookup': {'entry': hex(start), 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                             'manager': '0x5011f00', 'member': '0x127e8',
                             'lookup': '0x2798a0', 'record_stride': 144, 'record_array_offset': '0x119a8'},
              'set_matcher': '0xee2bd0', 'consumer': '0xee30c0',
              'target_series_present': [154, 215, 242, 473],
              'finding': 'Native PC retains explicit target set matching and a consumer restriction branch. Do not classify matching constants alone as platform exclusion.',
              'instructions': {'wrapper': [im.format(x) for x in wrapper],
                               'lookup': [im.format(x) for x in lookup],
                               'matcher': [im.format(x) for x in matcher],
                               'consumer': [im.format(x) for x in consumer]},
              'limits': ['Matching/consumer logic alone does not prove crafting availability or complete appearance behavior.',
                         'Consumer flag purpose and callers remain to certify; no branch is changed by this audit.',
                         'No native code executed or dynamic validation.']}
    (P/'research/horizon-other-retained-set-logic.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Retained PC target-set matcher and consumer certified; no exclusion edits inferred')


if __name__ == '__main__':
    main()
