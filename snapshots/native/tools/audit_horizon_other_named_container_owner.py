"""Certify native named-container resource wrapper storage and teardown calls."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    methods, bodies = {}, {}
    for entry in (0x22FDA80, 0x22FD620, 0x22FF5A0, 0x2222330, 0x2225C70,
                  0x130C860, 0x11947B0):
        fragments, body = decode_group(im, entry)
        bodies[entry] = {i.address: i for i in body}
        methods[hex(entry)] = dict(fragments=[list(f) for f in fragments],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            instructions=[im.format(i) for i in body])
    checks = {
        0x22FDA80: {0x22FDAB7:'rbx, qword ptr [rdx + 8]',
            0x22FDB2C:'qword ptr [rdi + 8], rbx', 0x22FDB36:'0x2216e40',
            0x22FDB60:'qword ptr [rax + rcx*8], rdi',
            0x22FDB67:'dword ptr [rsi + 0x10]'},
        0x22FD620: {0x22FD639:'rcx, qword ptr [rcx + 8]',
            0x22FD642:'0x22174d0', 0x22FD647:'qword ptr [rbx + 8], 0'},
        0x22FF5A0: {0x22FF5D5:'qword ptr [rax]',
            0x22FF5FF:'qword ptr [rbx + 0x20], 0',
            0x22FF607:'qword ptr [rbx + 0x10], 0',
            0x22FF64A:'0x22fda80'},
        0x2222330: {0x2222336:'byte ptr [rcx + 0x30], 0',
            0x2222347:'rsi, rdx', 0x2222355:'rdi, [rcx + 8]',
            0x2222368:'dword ptr [rsi + 0x5c]'},
        0x2225C70: {0x2225CA1:'dword ptr [rbx + 0x5c], 1',
            0x2225CA5:'0x2225cfa', 0x2225CE1:'0x2225d40',
            0x2225CEB:'qword ptr [rdx + rax*8 + 0x8070], 0',
            0x2225D13:'sil, sil', 0x2225D1B:'edx, 1',
            0x2225D23:'qword ptr [rax]'},
        0x130C860: {0x130C88B:'0x22174d0', 0x130C8A7:'0x22174d0',
            0x130C8AC:'qword ptr [rdi + 0x298], rbx',
            0x130C8BB:'0x2216e40', 0x130C8C0:'rcx, [rdi + 0x60]',
            0x130C8C4:'rdx, rbx', 0x130C8C7:'0x22ff5a0'},
        0x11947B0: {0x11947C4:'rcx, qword ptr [rcx + 0xdb0]',
            0x11947CB:'rcx, qword ptr [rcx + 0xa18]', 0x11947D2:'0x130c710'},
    }
    for entry, expected in checks.items():
        for address, operand in expected.items():
            assert bodies[entry][address].op_str == operand
    assert im.target(bodies[0x22FDA80][0x22FDB25]) == 0x35138A0
    assert im.pointer(0x35138A0) == 0x22FD620
    assert im.pointer(0x33FC610+0x68) == 0x2225C70
    getter = im.instructions(0x130C710)
    assert len(getter) == 2 and getter[0].op_str == 'rax, [rcx + 0x60]' and getter[1].mnemonic == 'ret'
    methods['0x130c710'] = dict(instructions=[im.format(i) for i in getter],
        sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in getter)).hexdigest())
    # These independently grounded call targets are tail-only leaves without pdata.
    for start, size, expected in [
        (0x2216E40, 15, ['mov', 'mov', 'jmp']),
        (0x22174D0, 17, ['mov', 'mov', 'mov', 'jmp'])]:
        offset = start-im.base
        raw = im.code[offset:offset+size]
        assert all(im.valid[offset:offset+size])
        ins = list(im.md.disasm(raw, start))
        assert sum(i.size for i in ins) == size and [i.mnemonic for i in ins] == expected
        assert ins[0].op_str == 'rdx, rcx' and im.target(ins[1]) == 0x5123A00
        assert ins[-1].op_str == ('0x2222330' if start == 0x2216E40 else 'qword ptr [rax + 0x68]')
        methods[hex(start)] = dict(bounds='exact tail leaf ending at jump; next byte INT3',
            sha256=hashlib.sha256(raw).hexdigest(), instructions=[im.format(i) for i in ins])
        assert im.code[offset+size] == 0xCC
    report = dict(exe_sha256=im.meta['exe_sha256'], methods=methods,
        facts=['22FDA80 rejects duplicate resource pointers already held in wrapper+8.',
               'New wrapper35138A0 stores resource at+8, calls2216E40, then enters container+20 array/count+10.',
               'The named dispatcher2300750 reads that same wrapper+8 resource.',
               'Wrapper destructor22FD620 calls22174D0 for nonnull resource and clears+8.',
               '2216E40 forwards resource to manager5123A00/2222330; full body increments resource+5C under native conditional locking.',
               'sMhResource vtable33FC610 virtual68 resolves to2225C70: decrement resource+5C, zero triggers native cache removal and virtual destructor outside conditional lock.',
               'Replacement22FF5A0 destroys old wrappers before clearing the array and adding new resource.'],
        role_sound_route=['130C710 returns inline container sound+60.',
                          '130C860 stores resource in sound+298 with native reference management, then passes it to sound+60 via22FF5A0.',
                          'Native caller11947B0 follows receiver+DB0 owner, owner+A18 sound, then130C710.',
                          'This matches the adapter owner+A18/containerAt route, not proof of which WWCT was loaded.'],
        limits=['Release mapping is certified for static manager vtable33FC610, not arbitrary derived overrides or live instance type.',
                'Cache fallback2225D40 and concrete resource destructor body not audited here.',
                'Does not establish actual equipment container provenance or its loading call site.',
                'Replacement destroys prior wrappers; host must not assume borrowed resource survives replacement.',
                'No native execution, dynamic test, or game-directory write.'])
    (P/'research/horizon-other-named-container-owner.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(methods=len(methods), wrapper_vtable='35138A0', destructor='22FD620')))


if __name__ == '__main__':
    main()
