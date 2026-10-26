"""Generate guards for scoped Horizon GCOD native admission; no native execution."""
import hashlib
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group

def main():
    im=CodeImage()
    root=P/'research/horizon-other-source/common/guildcard'
    for filename, ids in [('gc_data_title1.gcod',list(range(782,787))+list(range(1500,1505))),
                           ('gc_data_pose.gcod',[13])]:
        data=(root/filename).read_bytes()
        assert len(data)==10+struct.unpack_from('<I',data,6)[0]*20
        # Index, serialized ID and native reordered fields must agree.
        for identity in ids:
            record=struct.pack('<5I',identity,12,10 if identity>=1500 else 1,0,2)
            assert data.count(record)==1
            assert data[10+(identity-1)*20:10+identity*20]==record
    table_count=struct.unpack_from('<I',im.exe,im.raw(0x3ECA048))[0]
    assert table_count==39
    table=[struct.unpack_from('<5I',im.exe,im.raw(0x3ECA100)+n*20) for n in range(table_count)]
    assert {r[1] for r in table if r[0]==1}=={67101,67102,67607,67803,67804}
    assert {r[1] for r in table if r[0]==10}=={67803,67804}
    lines=['const TreeOwnerSpec kGuildcardAdmissionOwners[] = {']
    for address in (0xEE6390,0xEE6310,0x462700,0xEE27A0,0x1976240,0x30F150,0x134C120):
        xs=im.instructions(address);assert xs
        data=b''.join(i.bytes for i in xs)
        assert xs[0].address==address
        if address in (0xEE6390,0xEE6310):
            by={i.address:i for i in xs}
            assert im.target(by[address+4])==(0x5012A28 if address==0xEE6390 else 0x5012A40)
            assert by[address+0x22].op_str=='eax, dword ptr [rax + 0x18]'
            assert by[address+0x27].op_str=='eax, 1'
        if address==0x134C120:
            by={i.address:i for i in xs}
            assert by[0x134C128].op_str=='edx, 0xc'
            assert by[0x134C12B].op_str=='0x134c149'
            assert by[0x134C13A].op_str=='eax, dword ptr [rcx + rax*4 + 0x16de68]'
        digest=','.join(f'0x{x:02x}' for x in hashlib.sha256(data).digest())
        lines.append(f'    {{0x{address:X}, {len(data)}, {{{digest}}}}},')
    star={i.address:i for i in im.instructions(0x1976240)}
    for address,operand in {0x197625A:'qword ptr [rdi + 0xcea8], rbx',
                           0x1976270:'rcx, qword ptr [rdi + rax*8 + 0xaea8]',
                           0x1976278:'0x30f150', 0x197627D:'dword ptr [rax + 0x30], esi'}.items():
        assert star[address].op_str==operand
    assert im.instructions(0x30F150)[0].op_str=='rax, [rcx + 0xb0]'
    condition={i.address:i for i in im.instructions(0xEE27A0)}
    assert condition[0xEE27B2].op_str=='edx, -1'
    assert condition[0xEE27B7].op_str=='dword ptr [rcx + 8], edx'
    # Actual existing GUI consumer: uGUIGuildCard virtual2D0 invokes the
    # settings setup, which requests all conditions before populating its view.
    assert im.pointer(0x3213CB8+0x2D0)==0x109FC90
    menu={i.address:i for i in im.instructions(0x109FC90)}
    setup={i.address:i for i in im.instructions(0x60E6D0)}
    update={i.address:i for i in im.instructions(0xEE3400)}
    assert menu[0x109FEC3].op_str=='0x60e6d0'
    assert setup[0x60E6F4].bytes.hex()=='b9ffffffff'
    assert setup[0x60E6F9].op_str=='0xee3400'
    for address,target in {0xEE345D:'0xee27a0',0xEE3477:'0x1368400',
                           0xEE35B0:'0xee27a0',0xEE35CA:'0x13682e0'}.items():
        assert update[address].op_str==target
    # Keep the existing native character fields and stream pair. Do not create
    # a separate file or a direct save-bit writer for the ported card rewards.
    for root,fields in [(0x1368400,{0x16DBE0,0x16DCE0}),
                        (0x13682E0,{0x16DE68,0x16DE80})]:
        _,body=decode_group(im,root)
        used={op.mem.disp for i in body for op in i.operands if op.type==3}
        assert fields.issubset(used)
    for root,slot in [(0x1361D90,'0x70'),(0x136BEF0,'0x60')]:
        _,body=decode_group(im,root)
        for field,size in [(0x16DBE0,0x100),(0x16DCE0,0x100),
                           (0x16DE68,0x10),(0x16DE80,0x10)]:
            candidates=[n for n,i in enumerate(body) if i.mnemonic=='lea' and
                        any(op.type==3 and op.mem.disp==field for op in i.operands)]
            assert candidates
            assert any(any(j.mnemonic=='mov' and j.op_str==f'r8d, {size:#x}' for j in body[n:n+5]) and
                       any(j.mnemonic=='call' and j.op_str==f'qword ptr [rax + {slot}]' for j in body[n:n+5])
                       for n in candidates)
    lines.append('};')
    (P/'staging/Horizon-other-module/src/generated_guildcard_admission_owners.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print('Native guildcard GUI2D0 -> setup -> update(-1) certified; scoped GCOD admission guards generated: ten titles and pose13; owned permanent-optional conditions adapted; original writers retained.')

if __name__=='__main__':
    main()
