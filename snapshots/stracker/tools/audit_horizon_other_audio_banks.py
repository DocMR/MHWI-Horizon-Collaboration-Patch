"""Bound PS Wwise bank chunks and embedded media; never decode target code."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]


def main():
    banks = []
    for path in sorted((P/'research/horizon-other-effect-audio-source').rglob('*.nbnk')):
        data = path.read_bytes()
        cursor, chunks = 0, {}
        while cursor < len(data):
            assert cursor+8 <= len(data)
            tag, size = struct.unpack_from('<4sI', data, cursor)
            assert tag not in chunks and cursor+8+size <= len(data)
            chunks[tag] = data[cursor+8:cursor+8+size]
            cursor += 8+size
        assert cursor == len(data) and b'BKHD' in chunks
        media = []
        index = chunks.get(b'DIDX', b'')
        assert len(index) % 12 == 0
        payload = chunks.get(b'DATA', b'')
        for offset in range(0, len(index), 12):
            identity, start, size = struct.unpack_from('<3I', index, offset)
            assert start+size <= len(payload)
            raw = payload[start:start+size]
            row = {'id': identity, 'offset': start, 'size': size, 'sha256': hashlib.sha256(raw).hexdigest()}
            assert raw[:4] == b'RIFF' and raw[8:12] == b'WAVE'
            row['riff_declared_bytes'] = struct.unpack_from('<I', raw, 4)[0]+8
            row['riff_size_delta'] = len(raw)-row['riff_declared_bytes']
            position, riff = 12, []
            while position < len(raw):
                assert position+8 <= len(raw)
                kind, length = struct.unpack_from('<4sI', raw, position)
                assert position+8+length <= len(raw)
                value = raw[position+8:position+8+length]
                riff.append({'tag': kind.decode('ascii'), 'size': length})
                if kind == b'fmt ':
                    assert length >= 16
                    codec, channels, rate, average, align, bits = struct.unpack_from('<HHIIHH', value)
                    row.update(codec=hex(codec), channels=channels, sample_rate=rate,
                               block_align=align, bits=bits, fmt_hex=value.hex())
                position += 8+length+(length & 1)
            assert position == len(raw)
            row['chunks'] = riff
            media.append(row)
        banks.append({'path': path.relative_to(P).as_posix(), 'sha256': hashlib.sha256(data).hexdigest(),
                      'bkhd_hex': chunks[b'BKHD'].hex(),
                      'chunks': {key.decode('ascii'): len(value) for key, value in chunks.items()}, 'media': media})
    report = {'banks': banks, 'limits': ['Embedded media only; streamed media and HIRC graph not certified.',
                                         'No audio playback, native code execution or game launch.']}
    (P/'research/horizon-other-audio-bank-layout.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'banks': len(banks), 'embedded_media': sum(len(r['media']) for r in banks),
                      'codecs': sorted({r['codec'] for b in banks for r in b['media']})}))


if __name__ == '__main__':
    main()
