"""Find bounded native PC PCM examples via streamed RIFF header inspection."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]


def main():
    rows, inspected = [], 0
    expected = struct.pack('<HHIIHHHHI',0xFFFE,1,48000,96000,2,16,0,0,0x4101)
    root = P/'MHWI解包/chunk/sound/wwise/Windows'
    for path in sorted(root.glob('*.nbnk')):
        inspected += 1
        with path.open('rb') as f:
            index, data_start = None, None
            size = path.stat().st_size
            while f.tell()+8 <= size:
                tag, length = struct.unpack('<4sI',f.read(8))
                start = f.tell()
                assert start+length <= size
                if tag == b'DIDX':
                    index = f.read(length)
                if tag == b'DATA':
                    data_start = start
                f.seek(start+length)
            if index is None or data_start is None:
                continue
            assert len(index)%12 == 0
            for pos in range(0,len(index),12):
                media, offset, length = struct.unpack_from('<3I',index,pos)
                f.seek(data_start+offset)
                prefix = f.read(min(length,4096))
                if prefix[:4] != b'RIFF' or prefix[8:12] != b'WAVE':
                    continue
                cursor = 12
                while cursor+8 <= len(prefix):
                    tag, count = struct.unpack_from('<4sI',prefix,cursor)
                    if tag == b'fmt ':
                        fmt = prefix[cursor+8:cursor+8+count]
                        if fmt == expected:
                            rows.append(dict(bank=str(path.relative_to(P)), media=media,
                                media_bytes=length, fmt_hex=fmt.hex(), fmt_sha256=hashlib.sha256(fmt).hexdigest(),
                                bank_bytes=size, media_file_offset=data_start+offset))
                        break
                    cursor += 8+count+(count&1)
                if rows and rows[-1]['bank'] == str(path.relative_to(P)):
                    break
        if len(rows) >= 12:
            break
    report = dict(banks_inspected=inspected, examples=rows, expected_fmt_hex=expected.hex(),
        scope='Up to12 distinct existing PC banks with exact24-byte mono48k PCM fmt match.',
        limits=['Header identity only; HIRC codec binding, complete media validity and native playback remain separate.',
                'Bounded sample search, not a complete codec inventory. No audio decoded or played.'])
    (P/'research/horizon-other-pc-pcm-baselines.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(banks_inspected=inspected, exact_fmt_examples=len(rows))))


if __name__=='__main__':
    main()
