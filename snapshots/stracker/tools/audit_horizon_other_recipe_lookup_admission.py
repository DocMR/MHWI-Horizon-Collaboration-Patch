"""Certify recipe lookup receivers and retained admission consumers, statically."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    owners = {}
    by = {}
    for address in (0x1671570, 0x1672480, 0x16B7740):
        fragments, code = decode_group(im, address)
        by.update({i.address: i for i in code})
        owners[hex(address)] = dict(fragments=fragments, instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest())
    for address in (0x2798A0, 0x2798D0):
        code = im.instructions(address)
        assert code and code[-1].mnemonic == 'ret'
        by.update({i.address: i for i in code})
        owners[hex(address)] = dict(instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest())
    checks = {
        0x2798AA: 'rax, [rcx + 0x119a8]',
        0x2798D2: 'rax, qword ptr [rcx + rax*8 + 0x2efa8]',
        0x1671599: 'rcx, 0x127e8',
        0x16715A0: '0x2798a0',
        0x16715A8: 'rax, qword ptr [rax + rsi*8 + 0x18]',
        0x16715B2: 'eax, word ptr [rax + 0x1e]',
        0x16715D6: 'rax, qword ptr [rcx + rsi*8 + 0x48]',
        0x16724B9: 'rcx, 0x127e8',
        0x16724C0: '0x2798d0',
        0x16724D1: 'rdi, [rax + 0x18]',
        0x16724E8: 'r9d, byte ptr [r8 + 0x13]',
        0x16724FF: 'r9b, 2',
        0x1672532: 'r8b, 3',
        0x167256A: 'rcx, qword ptr [rdi + 0x30]',
        0x167256E: '0x16b7740',
        0x16725AF: 'rcx, qword ptr [rbx]',
        0x16725B2: '0x16b7740',
        0x16B7802: 'eax, dword ptr [rax + rcx*4 + 0xf6d98]',
        0x16B7838: 'eax, dword ptr [rax + rcx*4 + 0xf6d98]',
        0x16B7894: 'eax, dword ptr [rax + rcx*4 + 0x102428]',
        0x16B77A5: '0x1356800',
    }
    for address, operand in checks.items():
        assert by[address].op_str == operand, (hex(address), by[address].op_str)
    for address in (0x1671590, 0x16724B0):
        assert im.target(by[address]) == 0x5011F00
    for address in (0x1672494, 0x16B7793, 0x16B77EB):
        assert im.target(by[address]) == 0x5013950
    # Both are the SAME series-layout offsets relative to the registered base.
    assert 0x119A8 + 0x18 == 0x119C0
    assert 0x119A8 + 0x48 == 0x119F0
    report = dict(status='Static recipe identity and admission chain; no payment/output certification',
        exe_sha256=im.meta['exe_sha256'], owners=owners,
        receiver=dict(manager_global='0x5011f00', embedded_armor_manager='0x127e8'),
        lookup=dict(function='0x1671570', series_getter='0x2798a0',
                    equipment_array='series+0x18', recipe_array='series+0x48',
                    match='slot and native armor ID+1E; return matching original recipe pointer'),
        layered_consumer=dict(function='0x1672480', getter='0x2798d0',
            table='0x2efa8', types=[2, 3], admission='0x16b7740',
            preserved_gates=['nonnull entries', 'native character+16C and equipment+28 bit conditions',
                             'original series194/195 special branch', 'native admission predicate']),
        admission_reads=dict(item_acquisition_bitmap='character+0xf6d98',
                             other_bitmap='character+0x102428',
                             progression_query='0x1356800'),
        finding='Confirmed receiver provenance and paired equipment/recipe arrays connect registered hunter recipes to existing lookups and type2/3 admission. Admission reads acquisition/progression state; it is not a spend transaction.',
        limits=['Recipe-native field correspondence and full payment/output/save remain to certify.',
                'Equal raw offsets in unrelated UI classes do not identify this manager; such matches are excluded.',
                'Static reads do not establish complete availability for all current player progression.',
                'No native execution or dynamic validation.'])
    (P/'research/horizon-other-recipe-lookup-admission.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(owners=len(owners), instruction_checks=len(checks), receiver_globals_checked=5)))


if __name__ == '__main__':
    main()
