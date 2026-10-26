"""Inventory validated direct setter callers in saved PC code, without execution."""
import hashlib
import json
import re
import struct
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im = CodeImage()
    rows = []
    for match in re.finditer(b'\xe8', im.code):
        offset = match.start()
        address = offset+im.base
        if offset+5 > len(im.code) or address+5+struct.unpack_from('<i', im.code, offset+1)[0] != 0x130C860:
            continue
        if not all(im.valid[offset:offset+5]) or not im.owner(address):
            continue
        try:
            fragments, body = decode_group(im, address)
        except AssertionError:
            continue
        if not any(i.address == address and i.mnemonic == 'call' and i.op_str == '0x130c860' for i in body):
            continue
        rows.append(dict(call=hex(address), owner=hex(body[0].address),
            fragments=[list(f) for f in fragments],
            sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in body)).hexdigest(),
            has_owner_a18=any('+ 0xa18]' in i.op_str for i in body),
            context=[im.format(i) for i in body if address-100 <= i.address <= address+24]))
    report = dict(exe_sha256=im.meta['exe_sha256'], setter='0x130c860', callers=rows,
        limits=['Only validated direct E8 callers with complete saved unwind groups.',
                'No claim about indirect calls, missing code or total whole-game coverage.',
                'A18 field matches are candidates, not concrete receiver type proof.'])
    (P/'research/horizon-other-sound-resource-callers.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(callers=len(rows), a18_candidates=[r['call'] for r in rows if r['has_owner_a18']])))


if __name__ == '__main__':
    main()
