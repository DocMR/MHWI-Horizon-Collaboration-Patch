"""Refresh native notice guards from authenticated retained PC code only."""
import hashlib
from analyze_hzd_code import CodeImage,P

def main():
    im=CodeImage()
    spans=[(0x24c740,0xb40),(0x1ae2050,0x1be),(0x1acf140,169),(0x1ad5430,35),
           (0x1d8e120,0x4bd),(0x18c48a0,58),
           (0x2343240,7),(0x2340980,7),(0x23537a0,175),
           (0x234f3c0,25),(0x234f3e0,25),(0x234f7d0,25),(0x234f7f0,25),
           (0x2345170,11),(0x2343bc0,7),
           (0x23445f0,25),(0x2344610,25),(0x2342cd0,45)]
    lines=['// Authenticated original PC spans; generated from saved valid code.',
           'inline const TreeOwnerSpec kNoticeNativeOwners[] = {']
    for a,n in spans:
        data=im.code[a-im.base:a-im.base+n]
        assert len(data)==n and 0 not in im.valid[a-im.base:a-im.base+n]
        digest=hashlib.sha256(data).digest()
        lines.append('{0x%X,%d,{%s}},'%(a,n,','.join('0x%02X'%b for b in digest)))
        (P/f'research/horizon-complete-notice-native-span-{a:x}.bin').write_bytes(data)
    lines.append('};')
    (P/'staging/Horizon-complete-integration-20261021/src/generated_notice_native_owners.inc').write_text('\n'.join(lines)+'\n',encoding='ascii')
    print(f'Prepared {len(spans)} guards and matching isolated native fixture spans.')

if __name__=='__main__':main()
