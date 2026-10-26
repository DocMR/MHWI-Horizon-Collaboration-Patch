"""Certify player constructor path literal and existing PC sound list contents."""
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    fragments, body = decode_group(im, 0x2031B40)
    by = {i.address:i for i in body}
    assert im.target(by[0x2032498]) == 0x34A7158
    assert by[0x203249F].op_str == 'qword ptr [rbx + 0x5f68], rax'
    offset = im.raw(0x34A7158)
    literal = im.exe[offset:im.exe.index(b'\0',offset)].decode('ascii')
    assert literal == 'pl\\common\\sound\\pl_snd_ct_list'
    path = P/'MHWI解包/chunk/pl/common/sound/pl_snd_ct_list.wwctl'
    raw = path.read_bytes()
    assert struct.unpack_from('<4I',raw) == (0x4C544357,1,2,0)
    strings = raw[16:].split(b'\0')
    assert strings[-1] == b''
    names = [s.decode('ascii').replace('\\','/') for s in strings[:-1]]
    assert names == ['sound/pl/hm_cmn_snd_ct','sound/pl/pl_cmn_snd_ct']
    common = P/'MHWI解包/chunk/sound/pl/pl_cmn_snd_ct.wwct'
    assert b'sound\\pl\\snd_equip_body068_snd_ev\0' in common.read_bytes()
    methods = {}
    checks = {
        0x213F40: {0x213F63:'r9d, 0xb8'},
        0x23DDA40: {0x23DDA9D:'qword ptr [rbx], rax'},
        0x23DE020: {0x23DE052:'dword ptr [rsp + 0x40], 0x4c544357',
            0x23DE060:'dword ptr [rsp + 0x44], 1',
            0x23DE0F5:'edx, [rsi*8]', 0x23DE114:'dword ptr [rdi + 0xb0], esi',
            0x23DE126:'qword ptr [rdi + 0xa8], rax',
            0x23DE163:'0x2175a10', 0x23DE191:'qword ptr [rax + 0x58]',
            0x23DE1A3:'qword ptr [rbx + rax], rcx', 0x23DE1A7:'rbx, 8'},
        0x130C7E0: {0x130C82C:'qword ptr [rdi + 0x2a0], rbx',
            0x130C83B:'0x2216e40', 0x130C840:'rcx, [rdi + 0x60]',
            0x130C847:'0x22ff650'},
        0x22FF650: {0x22FF698:'qword ptr [rax]',
            0x22FF6C1:'qword ptr [rdi + 0x20], rbx',
            0x22FF6C5:'qword ptr [rdi + 0x10], rbx',
            0x22FF6F5:'0x47f870', 0x22FF6FF:'rdx, qword ptr [rax]',
            0x22FF70A:'0x22fda80', 0x22FF71B:'0x22ff6f0'},
    }
    for entry, expected in checks.items():
        fs, xs = decode_group(im,entry)
        indexed = {i.address:i for i in xs}
        for address, operand in expected.items():
            assert indexed[address].op_str == operand
        methods[hex(entry)] = dict(fragments=[list(f) for f in fs],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in xs)).hexdigest(),
            instructions=[im.format(i) for i in xs])
    assert im.pointer(0x3522818+8) == 0x23DDA40
    assert im.pointer(0x3522850+0x50) == 0x23DE020
    registration = {i.address:i for i in decode_group(im,0x213F40)[1]}
    assert im.target(registration[0x213F5C]) == 0x51C79D8
    assert im.target(registration[0x213F51]) == 0x3522838
    assert im.target({i.address:i for i in decode_group(im,0x23DDA40)[1]}[0x23DDA96]) == 0x3522850
    reader = {i.address:i for i in decode_group(im,0x23DE020)[1]}
    assert im.target(reader[0x23DE187]) == 0x51C46F8
    access = im.instructions(0x47F870)
    assert any(i.op_str == 'rax, [rax + rcx*8]' for i in access)
    assert any(i.op_str == 'rax, qword ptr [rax + 0xa8]' for i in access)
    methods['0x47f870'] = dict(instructions=[im.format(i) for i in access])
    report = dict(exe_sha256=im.meta['exe_sha256'], constructor='0x2031b40',
        fragments=[list(f) for f in fragments],
        code_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
        path_literal=literal, field='receiver+5F68', list_entries=names,
        list_assignment_methods=methods,
        list_assignment='130C7E0 holds list at sound+2A0;22FF650 clears previous wrappers, then adds each nonnull list resource through22FDA80 to sound+60.',
        reader='rWwiseContainerList DTI51C79D8, instance VT3522850, reader23DE020 verifies WCTL v1, allocates count*8 and loads each path as WWCT DTI51C46F8 into list+A8.',
        files=[dict(path=str(f.relative_to(P)),sha256=hashlib.sha256(f.read_bytes()).hexdigest()) for f in (path,common)],
        limits=['Static default load chain certified; asynchronous readiness, path overrides and live object timing remain separate.',
                'Existing common WWCT contains the shield event resource name; actual load timing not established.',
                'No native execution, dynamic test or game-directory writes.'])
    (P/'research/horizon-other-player-sound-list.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(entries=len(names),shield_event_in_pc_common=True)))


if __name__=='__main__':
    main()
