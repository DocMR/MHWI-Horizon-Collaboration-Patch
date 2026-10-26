"""Export exact bow-only native tree edits for final composition with Stormslinger."""
from pathlib import Path
import hashlib
import json
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]
EDITS = [(0x13732F0, 0x1373456, '0f867e010000', 0x13735DA, 'IDs102/103 child exclusion'),
         (0x13732F0, 0x137345F, '0f8475010000', 0x13735DA, 'ID254 child exclusion'),
         (0x13738B0, 0x13738E2, '763d', 0x1373921, 'IDs102/103 exclusion predicate'),
         (0x13738B0, 0x13738EC, '7433', 0x1373921, 'ID254 exclusion predicate')]


def main():
    image = CodeImage()
    owners = {owner: image.instructions(owner) for owner, *_ in EDITS}
    assert all(owners.values())
    result = []
    for owner, rva, expected_hex, destination, label in EDITS:
        instructions = {i.address: i for i in owners[owner]}
        instruction = instructions[rva]
        assert instruction.bytes.hex() == expected_hex
        assert instruction.mnemonic in ('jbe', 'je')
        assert instruction.operands[0].imm == destination
        # Store the full original owner digest for safe composition, not just a short opcode.
        code = b''.join(i.bytes for i in owners[owner])
        result.append({'owner_rva': hex(owner), 'rva': hex(rva),
                       'offset_in_owner': rva-owner, 'expected_hex': expected_hex,
                       'replacement_hex': '90' * instruction.size, 'reason': label,
                       'original_owner_bytes': len(code), 'original_owner_sha256': hashlib.sha256(code).hexdigest()})
    plan = {'schema_version': 1, 'module': 'horizon-other',
            'exe_sha256': image.meta['exe_sha256'], 'edits': result,
            'composition': {'shared_owners': ['0x13732f0', '0x13738b0'],
                            'rule': 'Compose these four non-overlapping bow edits with Stormslinger edits against the same original owner before generating a single clone/hook.',
                            'forbidden': 'Do not install two competing whole-function clones or hook owners; do not overwrite the Stormslinger branch edits.'},
            'scope': 'Remove only explicit category11 bow ID102/103/254 exclusions; preserve source lookups, DLC/recipe gates and category13 Stormslinger branches.',
            'state': 'Static native instruction plan; not installed; no dynamic tests',
            'remaining': ['Final merge chat must compose owner edits with the Stormslinger clone implementation.',
                          'Other menu, crafting transaction and equipment resource paths remain to implement/certify.']}
    output = P/'staging/Horizon-other-module/merge/weapon-tree-edits.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(plan, indent=2), encoding='utf-8')
    report = {**plan, 'instructions': {hex(owner): [image.format(i) for i in xs] for owner, xs in owners.items()}}
    (P/'research/horizon-other-tree-exclusion-audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    lines = ['// Generated from certified native owners. Do not edit by hand.',
             'constexpr TreeOwnerSpec kTreeOwners[] = {']
    for owner, xs in owners.items():
        raw = b''.join(i.bytes for i in xs)
        digest = hashlib.sha256(raw).digest()
        lines.append('    {'+f'0x{owner:X}, {len(raw)}, '+'{'+', '.join(f'0x{x:02X}' for x in digest)+'}},')
    lines += ['};', 'constexpr TreeEdit kTreeEdits[] = {']
    for row in result:
        expected = bytes.fromhex(row['expected_hex'])
        lines.append('    {'+f'{row["owner_rva"]}, {row["offset_in_owner"]}, {len(expected)}, '+'{'+
                     ', '.join(f'0x{x:02X}' for x in expected)+'}},')
    lines += ['};', '']
    (P/'staging/Horizon-other-module/src/generated_bow_tree_edits.inc').write_text('\n'.join(lines), encoding='utf-8')
    print('4 bow-only branch edits exported; both complete native owners and exact branches statically verified.')


if __name__ == '__main__':
    main()
