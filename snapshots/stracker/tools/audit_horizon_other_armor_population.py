"""Certify armor-manager population and retain unidentified gates intact."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P


def main():
    im = CodeImage()
    population = im.instructions(0x278CB0)
    assert population
    by = {x.address: x for x in population}
    for address, target in ((0x278D77, 0x5012150), (0x278DC1, 0x5012150),
                            (0x278DDF, 0x5012160), (0x278E22, 0x5012160)):
        assert im.target(by[address]) == target
    for address in (0x278D95, 0x278E05, 0x278E50):
        assert by[address].op_str == '0x462700'
    for address in (0x278DA0, 0x278E10, 0x278E5B):
        assert by[address].op_str == '0x18bc560'
    assert by[0x278DAF].op_str == '0x278a00'
    assert by[0x278FC6].op_str == 'r15d, 0x2ec'
    gate = im.instructions(0x18BC560)
    gb = {x.address: x for x in gate}
    assert im.target(gb[0x18BC568]) == 0x51C4148
    assert gb[0x18BC572].op_str == 'qword ptr [rax + 0x68]'
    assert gb[0x18BC575].op_str == 'ebx, eax' and gb[0x18BC577].mnemonic == 'setbe'
    report = {'exe_sha256': im.meta['exe_sha256'], 'population': '0x278cb0',
              'population_sha256': hashlib.sha256(b''.join(x.bytes for x in population)).hexdigest(),
              'record_resources': ['0x5012150', '0x5012160'],
              'record_getter': '0x462700', 'retained_gate': '0x18bc560',
              'gate_contract': 'Unsigned incoming ECX <= global51C4148 object virtual68 return; semantic purpose unproven.',
              'preserved_special_record_predicate': '0x278a00',
              'vector_loop_compare': 748,
              'finding': 'Population has resource and shared numeric gates before native set construction; no bypass inferred.',
              'instructions': {'population': [im.format(x) for x in population],
                               'numeric_gate': [im.format(x) for x in gate]},
              'limits': ['Resource-global serialized table identities and gate value meaning remain to bind.',
                         'Constructor loop coverage does not prove every target row reaches every menu.',
                         'No target code executed; no unlock or gate branch modified.']}
    (P/'research/horizon-other-armor-population.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print('Armor population resource/gate path certified; original gate preserved')


if __name__ == '__main__':
    main()
