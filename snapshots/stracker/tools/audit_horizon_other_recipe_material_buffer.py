"""Separate original material-list writes from actual inventory transactions."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P


def main():
    im = CodeImage()
    # This is one fully decoded pdata fragment, not the whole parent function:
    # the parent's other fragments include an inline switch table.
    code = im.instructions(0x1F433FA)
    owner = im.owner(0x1F433FA)
    assert owner and owner[0] == 0x1F433C9 and code
    by = {x.address: x for x in code}
    checks = {
        0x1F433FA: '0x1671570',
        0x1F433FF: 'r15, rax',
        0x1F4340D: 'rbp, [rax + 0x28]',
        0x1F43411: 'r14, [rax + 0x20]',
        0x1F43415: 'rsi, [rbx + 0x2f10]',
        0x1F43420: 'edx, word ptr [r14]',
        0x1F43427: 'ebx, byte ptr [rbp]',
        0x1F4342B: '0x1187e50',
        0x1F43438: '0x11cea20',
        0x1F4343D: 'rsi, 0x10',
        0x1F43441: 'rbp, [rbp + 1]',
        0x1F43445: 'r14, 2',
        0x1F43452: '0x16b7740',
    }
    for address, operand in checks.items():
        assert by[address].op_str == operand
    leaves = {}
    for address in (0x1187E50, 0x11CEA20):
        c = im.instructions(address)
        assert c and c[-1].mnemonic == 'ret'
        leaves[hex(address)] = [im.format(x) for x in c]
    assert im.instructions(0x1187E50)[0].op_str == 'dword ptr [rcx + 8], edx'
    assert im.instructions(0x11CEA20)[0].op_str == 'dword ptr [rcx + 0xc], edx'
    # Verify the four-loop initialization in a separately bounded prefix.
    start, end = 0x1F43360, 0x1F433C9
    raw = im.code[start-im.base:end-im.base]
    assert all(im.valid[start-im.base:end-im.base])
    prefix = list(im.md.disasm(raw, start))
    assert sum(i.size for i in prefix) == len(raw)
    pb = {i.address: i for i in prefix}
    assert pb[0x1F43389].op_str == 'edi, 4'
    assert pb[0x1F43393].op_str == 'rbx, 0x2f10'
    assert pb[0x1F433A3].op_str == '0xeda580'
    caller = im.instructions(0x1F44FC0)
    cb = {i.address: i for i in caller}
    assert cb[0x1F44FD5].op_str == '0x1f43360'
    assert cb[0x1F44FFE].op_str == 'r8, 0x2f1'
    assert cb[0x1F45005].op_str == 'r8, 4'
    assert cb[0x1F4501A].op_str == '0x1f44d60'
    result = dict(status='Static four-material buffer construction; not inventory deduction',
        exe_sha256=im.meta['exe_sha256'],
        prefix=dict(start=hex(start), end=hex(end), sha256=hashlib.sha256(raw).hexdigest()),
        fragment=dict(start=hex(owner[0]), end=hex(owner[1]), instructions=len(code),
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest()),
        recipe_source='0x1671570 (original slot/native armor ID lookup)',
        native_recipe=dict(material_ids_offset='0x20', quantity_bytes_offset='0x28', slots=4),
        buffer=dict(base='UI receiver+0x2f10', stride='0x10', id_dword_offset=8, quantity_dword_offset=12),
        setters=leaves, display_consumer='0x1f44fc0 -> 0x1f44d60',
        finding='Recipe material IDs/quantities are copied into four local UI records; the two setters only write receiver fields. These calls must not be hooked as inventory spending.',
        limits=['One pdata fragment and bounded prefix certified; whole switch function not certified.',
                'Purchase confirmation, currency spending, inventory deduction and output registration remain separate.',
                'No native execution or dynamic tests.'])
    (P/'research/horizon-other-recipe-material-buffer.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(dict(fragment_instructions=len(code), material_slots=4, verified_setters=2)))


if __name__ == '__main__':
    main()
