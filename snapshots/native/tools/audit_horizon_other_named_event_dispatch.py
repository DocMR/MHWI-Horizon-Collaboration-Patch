"""Static disassembly certificates for PC named-event selection; no execution."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    methods, bodies = {}, {}
    for entry in (0x2300260, 0x2300750, 0x21E6C10, 0x24BDDC0, 0x24BDE30, 0x21757E0):
        if im.owner(entry):
            fragments, body = decode_group(im, entry)
        else:
            fragments, body = [], im.instructions(entry)
        assert body
        bodies[entry] = {i.address: i for i in body}
        methods[hex(entry)] = dict(fragments=[list(f) for f in fragments],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            instructions=[im.format(i) for i in body])
    checks = {
        0x2300260: {0x2300269:'r9d, edx', 0x2300274:'r8d, r8d', 0x2300277:'0x2300750'},
        0x2300750: {0x230076E:'edx, 0xffffffff', 0x2300784:'0x21e6c10',
            0x2300789:'r15d, eax', 0x23007FB:'r12d, -1', 0x23007FF:'0x230080a',
            0x2300801:'r12d, eax', 0x2300845:'0x24bddc0',
            0x230084F:'dword ptr [rax + 8], r15d', 0x2300853:'0x2300886',
            0x2300873:'0x22fef80', 0x2300892:'0x2300840',
            0x23008BA:'0x23007f0', 0x23008D6:'0x23007b0'},
        0x21E6C10: {0x21E6C34:'r8, rax', 0x21E6C39:'eax, 8',
            0x21E6C3C:'edx, dword ptr [r11 + r8*4]', 0x21E6C40:'edx, eax',
            0x21E6C47:'eax, edx'},
        0x24BDDC0: {0x24BDDC0:'dword ptr [rcx + 0xd0], edx',
            0x24BDDCD:'rax, 7', 0x24BDDD1:'rax, qword ptr [rcx + 0xc8]'},
        0x24BDE30: {0x24BDFB5:'rsi, 0xc8', 0x24BDFF1:'edx, 7',
            0x24BE001:'dword ptr [rsi + 8], r14d', 0x24BE01C:'qword ptr [rsi], rax',
            0x24BE030:'rdx, qword ptr [rsi]', 0x24BE038:'rdx, rdi',
            0x24BE03B:'r8d, 0x80', 0x24BE041:'0x21757e0',
            0x24BE055:'0x2175a10', 0x24BE08C:'qword ptr [rax + rbx + 0x68], rcx',
            0x24BE088:'rdi, -0x80', 0x24BE091:'rbx, -0x80',
            0x24BE099:'0x24be030'},
        0x21757E0: {0x21757EF:'rbx, r8', 0x21757F9:'rbp, rdx',
            0x2175806:'0x217587f', 0x217582D:'0x2c3db50',
            0x2175836:'rdx, [rdi + rbp]', 0x217583A:'rbx, rdi',
            0x2175843:'qword ptr [rax + 0x40]',
            0x2175883:'rcx, rbp', 0x2175889:'r8, rbx',
            0x217588C:'0x2c3db50', 0x2175891:'dword ptr [rsi + 0x18], ebx'},
    }
    for entry, expected in checks.items():
        for addr, operand in expected.items():
            assert bodies[entry][addr].op_str == operand
    assert im.target(bodies[0x21E6C10][0x21E6C1C]) == 0x34FFDA0
    table = struct.unpack_from('<256I', im.exe, im.raw(0x34FFDA0))
    for n, value in enumerate(table):
        crc = n
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
        assert value == crc
    report = dict(exe_sha256=im.meta['exe_sha256'], methods=methods,
        crc_table_sha256=hashlib.sha256(struct.pack('<256I', *table)).hexdigest(),
        facts=['2300260 passes category to R9D and null resource filter to 2300750.',
               'Name hash initial FFFFFFFF, reflected CRC32 table, return without final XOR.',
               'Category -1 bypasses equality filtering; category 0 in shield WWCT is eligible.',
               'Record+8 is compared to full 32-bit hash before 22FEF80 dispatch.',
               'Reader allocates count*128 array at resource+C8 with count at D0; accessor returns C8+index*128.',
               'Reader passes each 128-byte destination to stream read21757E0, then reads child path separately.',
               'Child resource pointer is written at record+68, leaving local ID+0, event ID+4 and name hash+8 untouched in this loop.',
               '21757E0 buffered read forwards bytes to caller destination; buffered and refill paths contain no event field remapping.',
               'Loops continue over records/categories/resources; no global uniqueness assumption.'],
        limits=['Loaded container membership, concrete underlying stream implementation and request lifetime remain separate.',
                'Does not establish actual equipment trigger timing or production startup.',
                'Static disassembly/data inspection only; no native code execution.'])
    (P/'research/horizon-other-named-event-dispatch.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(methods=len(methods), crc_table_entries=len(table), category_minus_one='all_categories')))


if __name__ == '__main__':
    main()
