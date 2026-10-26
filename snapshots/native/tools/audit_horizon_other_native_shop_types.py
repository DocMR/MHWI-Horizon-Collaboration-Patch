"""Anchor crafting UI research in registered native types, not nearby addresses."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    entries = [
        ('uGUIEquipArmorShop', 0x3469AD0, 0x1FE790, 0x5029F10, 0x3469B30,
         0x1DBFF20, 0x1DC0500, 0x3469B50, 0x1DC0520, 0x1DC0531),
        ('uGUIEquipCraftShop', 0x346C548, 0x1FE940, 0x502A000, 0x346C4B0,
         0x1DEC360, 0x1DECE60, 0x346C578, 0x1DECE8E, 0x1DECE95),
        ('uGUIWishList', 0x34905C8, 0x201D40, 0x502C190, 0x34905A8,
         0x1F3F8B0, 0x1F403B0, 0x34905F0, 0x1F403CD, 0x1F403D4),
    ]
    results = []
    for name, text, reg, dti, dtivt, placement, ctor, vt, load, store in entries:
        off = im.raw(text)
        assert im.exe[off:off+len(name)+1] == name.encode()+b'\0'
        registration = im.instructions(reg)
        assert registration and any(im.target(i) == text for i in registration)
        assert any(im.target(i) == dti for i in registration)
        assert any(im.target(i) == dtivt for i in registration)
        assert im.pointer(dtivt+0x10) == placement
        raw = im.code[placement-im.base:placement-im.base+16]
        assert all(im.valid[placement-im.base:placement-im.base+16])
        pc = list(im.md.disasm(raw, placement))
        assert sum(i.size for i in pc) == 16 and pc[-1].mnemonic == 'ret'
        assert any(i.mnemonic == 'jmp' and i.op_str == hex(ctor) for i in pc)
        construction = im.instructions(ctor)
        by = {i.address: i for i in construction}
        assert im.target(by[load]) == vt
        assert by[store].op_str == 'qword ptr [rbx], rax'
        methods = []
        for slot in (0x250, 0x288, 0x2D0):
            target = im.pointer(vt+slot)
            c = im.instructions(target)
            thunk = None
            if not c and im.owner(target) is None:
                raw_thunk = im.code[target-im.base:target-im.base+5]
                ins = list(im.md.disasm(raw_thunk, target))
                if (all(im.valid[target-im.base:target-im.base+5])
                        and len(ins) == 1 and ins[0].mnemonic == 'jmp'
                        and ins[0].operands[0].type == 2):
                    destination = ins[0].operands[0].imm
                    fragments, resolved = decode_group(im, destination)
                    thunk = dict(bytes_hex=raw_thunk.hex(), destination=hex(destination),
                                 fragments=fragments, full_group_instructions=len(resolved),
                                 sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in resolved)).hexdigest())
            methods.append(dict(slot=hex(slot), target=hex(target), tail_bridge=thunk,
                pdata_fragment=im.owner(target), fragment_decoded=bool(c),
                direct_calls=[dict(site=hex(i.address), target=hex(i.operands[0].imm))
                              for i in c if i.mnemonic == 'call' and i.operands[0].type == 2]))
        results.append(dict(name=name, name_rva=hex(text), registration=hex(reg),
            dti=hex(dti), dti_vtable=hex(dtivt), placement=hex(placement),
            ctor=hex(ctor), instance_vtable=hex(vt), methods=methods,
            registration_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in registration)).hexdigest(),
            placement_sha256=hashlib.sha256(raw).hexdigest()))
    _, setup = decode_group(im, 0x1DF4290)
    sb = {i.address: i for i in setup}
    assert im.target(sb[0x1DF438C]) == 0x1DF02D0
    _, callback = decode_group(im, 0x1DF02D0)
    cb = {i.address: i for i in callback}
    assert cb[0x1DF034D].op_str == 'dword ptr [rbx + 0x45e8], 3'
    assert cb[0x1DF0367].op_str == 'dword ptr [rbx + 0x45e8], 4'
    assert cb[0x1DF0371].op_str == 'dword ptr [rbx + 0x45f0], edi'
    _, deferred = decode_group(im, 0x1DFA530)
    db = {i.address: i for i in deferred}
    assert db[0x1DFA88A].op_str == 'eax, dword ptr [rbx + 0x45e8]'
    assert db[0x1DFA8E9].op_str == '0x1df5400'
    assert db[0x1DFA8D6].op_str == '0x1dfb900'
    report = dict(status='Static registered native shop type anchors',
        exe_sha256=im.meta['exe_sha256'], types=results,
        deferred_actions=dict(setup='0x1df4290', callback='0x1df02d0',
            action_field='0x45e8', selected_index_field='0x45f0',
            dispatcher='0x1dfa530', actions={'3': '0x1df5400', '4': '0x1dfb900'},
            finding='Callback records action and selection; later native dispatcher invokes handlers. This does not establish action3/4 as crafting payment.'),
        next_roots=['ArmorShop setup callbacks, with slot288 bridge resolved to 0x1dd2ae0',
                    'CraftShop deferred operations and their upstream native requests; slot288 bridge resolves to 0x1dfa530'],
        limits=['Virtual slot semantics are not inferred solely from their ordinal.',
                'Nearby code addresses do not alone prove a method belongs to a registered type.',
                'GUI setup calls are not equipment inventory mutations merely because they write state.',
                'No payment or crafting completion claim; incomplete fragments remain marked.',
                'No target execution, game installation or dynamic test.'])
    (P/'research/horizon-other-native-shop-types.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(registered_types=len(results), method_slots=9)))


if __name__ == '__main__':
    main()
