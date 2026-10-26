"""Certify native inventory quantity branches without executing the mechanism."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    methods, indexed = {}, {}
    for entry in (0x1895930,0x1895750):
        fragments, body = decode_group(im,entry)
        indexed[entry] = {i.address:i for i in body}
        methods[hex(entry)] = dict(fragments=[list(f) for f in fragments],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            instructions=[im.format(i) for i in body])
    expected = {0x18958A4:'ebx, dword ptr [rcx + 0xc]',
        0x18958D7:'r14d',0x18958DA:'ebx, r14d',
        0x18958DD:'r14d, dword ptr [rcx + 0xc]',0x18958E1:'0x18958ba',
        0x18958BA:'edx, ebx',0x18958BC:'0x11cea20',
        0x18958E3:'edx, edx',0x18958E5:'0x11cea20',0x18958EA:'eax, ebx'}
    for address, operand in expected.items():
        assert indexed[0x1895750][address].op_str == operand
    for address, operand in {0x1895B60:'r14d',
        0x1895B63:'r12d, dword ptr [rax + 0xc]',0x1895B67:'r14d, r12d',
        0x1895B6A:'0x1895b95',0x1895B6C:'r12d, r14d',
        0x1895B78:'0x11cea20',0x1895B7D:'eax, eax',
        0x1895C35:'0x11cea20',0x1895C6E:'r12d, r14d',
        0x1895C71:'eax, r12d'}.items():
        assert indexed[0x1895930][address].op_str == operand
    setter = im.instructions(0x11CEA20)
    assert len(setter) == 7
    sx = {i.address:i for i in setter}
    assert sx[0x11CEA20].op_str == 'dword ptr [rcx + 0xc], edx'
    assert sx[0x11CEA27].op_str == 'r8b, 1'
    assert sx[0x11CEA2D].op_str == 'dword ptr [rcx + 8], edx'
    methods['0x11cea20'] = dict(instructions=[im.format(i) for i in setter],
        sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in setter)).hexdigest())
    report = dict(exe_sha256=im.meta['exe_sha256'],methods=methods,
        storage1_negative_branch=['Load existing quantity from record+C.',
            'Negate negative input delta to required positive amount; subtract from existing quantity.',
            'If enough, write remaining quantity via11CEA20 and return0.',
            'If insufficient, write0 via11CEA20 and return signed shortage.'],
        storage0_negative_branch=['For located record, compare required positive amount with existing record+C.',
            'Strictly less: write remaining quantity through11CEA20 and return0.',
            'Greater/equal: follows native slot compaction/clear route, then returns existing minus required.',
            '11CEA20 writes record+C and clears item ID+8 when new quantity0 and clear flag1.'],
        limits=['Storage selection/type lookup and every compaction detail still require provenance.',
                'Storage0 item metadata flag20 may bypass mutation; retain this native exception.',
                'Not proof of target recipe admission or finished equipment output.',
                'No native mechanism simulation, execution or dynamic test.'])
    (P/'research/horizon-other-inventory-quantity.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(full_functions=3,storage1_negative_branch_certified=True,storage0_quantity_writes_certified=True)))


if __name__=='__main__':
    main()
