"""Convert the two scoped NPC dialogue AKPK payloads; never execute game code."""
from pathlib import Path
import hashlib
import json
import struct
from convert_horizon_other_atrac9_media import bank_chunks, chunk, convert_atrac9_media

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-effect-audio-source/sound/wwise/PS4'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC/sound/wwise/Windows'


def package_entries(data):
    magic, header_size, version, languages, banks, streams, external = struct.unpack_from('<4s6I', data)
    assert magic == b'AKPK' and version == 1
    assert (languages, banks, external) == (16, 4, 4)
    assert header_size == 20+languages+banks+streams+external
    bank_at = 28+languages
    assert struct.unpack_from('<I', data, bank_at)[0] == 0
    stream_at = bank_at+banks
    count = struct.unpack_from('<I', data, stream_at)[0]
    assert streams == 4+20*count and count == 31
    assert struct.unpack_from('<I', data, stream_at+streams)[0] == 0
    rows, ids, end = [], set(), header_size+8
    for n in range(count):
        at = stream_at+4+20*n
        identity, block, size, offset, language = struct.unpack_from('<5I', data, at)
        assert block == 1 and language == 0 and identity not in ids
        assert offset >= end and offset+size <= len(data)
        assert not any(data[end:offset])
        ids.add(identity)
        rows.append((at, identity, offset, size))
        end = offset+size
    assert end == len(data)
    return header_size+8, rows


def main():
    files, media, sound_rows = [], [], []
    for costume in ('036', '081'):
        name = f'npc_otm{costume}_talk'
        original = (SOURCE/(name+'.npck')).read_bytes()
        header_end, entries = package_entries(original)
        result = bytearray(original[:header_end])
        for at, identity, offset, size in entries:
            row = convert_atrac9_media(original[offset:offset+size], name+'.npck', identity)
            converted = (P/row['path']).read_bytes()
            result.extend(bytes((-len(result)) % 16))
            struct.pack_into('<II', result, at+8, len(converted), len(result))
            result.extend(converted)
            media.append(row)
        _, rebuilt = package_entries(result)
        assert [r[1] for r in entries] == [r[1] for r in rebuilt]
        for (_, identity, offset, size), row in zip(rebuilt, media[-len(entries):]):
            assert identity == row['id'] and hashlib.sha256(result[offset:offset+size]).hexdigest() == row['sha256']

        source_bank = (SOURCE/(name+'.nbnk')).read_bytes()
        parts = bank_chunks(source_bank)
        assert set(parts) == {b'BKHD', b'HIRC'}
        assert struct.unpack_from('<I', parts[b'BKHD'])[0] == 120
        before = parts[b'HIRC']; hirc = bytearray(before); cursor = 4
        referenced, allowed = set(), set()
        for _ in range(struct.unpack_from('<I', hirc)[0]):
            kind, size = struct.unpack_from('<BI', hirc, cursor)
            start = cursor+5
            assert start+size <= len(hirc)
            if kind == 2:
                identity, plugin = struct.unpack_from('<II', hirc, start)
                medium = struct.unpack_from('<I', hirc, start+9)[0]
                assert plugin == 0xc0001 and hirc[start+8] == 2 and medium in {r[1] for r in entries}
                # Version120 stream type2 is Streaming. Its stored buffer-size
                # field is retained, not replaced with the whole package size.
                struct.pack_into('<I', hirc, start+4, 0x10001)
                allowed.update(range(start+4, start+8)); referenced.add(medium)
                sound_rows.append({'bank': name, 'sound_id': identity, 'media_id': medium})
            cursor = start+size
        assert cursor == len(hirc) and referenced == {r[1] for r in entries}
        assert all(i in allowed for i, (a, b) in enumerate(zip(before, hirc)) if a != b)
        parts[b'HIRC'] = bytes(hirc)
        bank = b''.join(chunk(tag, value) for tag, value in parts.items())
        assert bank_chunks(bank)[b'BKHD'] == bank_chunks(source_bank)[b'BKHD']
        OUTPUT.mkdir(parents=True, exist_ok=True)
        for extension, data, source in (('.npck', bytes(result), original), ('.nbnk', bank, source_bank)):
            target = OUTPUT/(name+extension)
            if target.exists():
                assert target.read_bytes() == data
            else:
                target.write_bytes(data)
            files.append({'path': target.relative_to(OUTPUT.parents[2]).as_posix(),
                          'size': len(data), 'sha256': hashlib.sha256(data).hexdigest(),
                          'source_sha256': hashlib.sha256(source).hexdigest()})
    report = {'files': files, 'media': media, 'sound_references': sound_rows,
              'install_allowed': False, 'dynamic_validation_performed': False,
              'limits': ['Scoped AKPK1 containers only; original IDs, language table and stream types retained.',
                         'PCM stream parser acceptance and actual loading/triggering remain unverified.']}
    (P/'research/horizon-other-npc-stream-conversion.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'files': len(files), 'converted_media': len(media), 'sound_references': len(sound_rows)}))


if __name__ == '__main__':
    main()
