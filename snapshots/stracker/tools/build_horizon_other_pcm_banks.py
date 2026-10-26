"""Rebuild isolated PCM bank candidates while preserving unrelated HIRC bytes."""
from pathlib import Path
import hashlib
import json
import struct
from convert_horizon_other_atrac9_media import bank_chunks, chunk

P = Path(__file__).resolve().parents[1]


def main():
    receipt = json.loads((P/'research/horizon-other-pcm-media-conversion.json').read_text(encoding='utf-8'))
    media = {(r['bank'], r['id']): r for r in receipt['media']}
    rows = []
    source_root = P/'research/horizon-other-effect-audio-source/sound/wwise/PS4'
    for source in sorted(source_root/name for name in {r['bank'] for r in receipt['media']}):
        original = source.read_bytes()
        parts = bank_chunks(original)
        index, payload, sizes = bytearray(), bytearray(), {}
        for offset in range(0, len(parts.get(b'DIDX', b'')), 12):
            identity, old_offset, old_size = struct.unpack_from('<3I', parts[b'DIDX'], offset)
            row = media[source.name, identity]
            converted = (P/row['path']).read_bytes()
            assert hashlib.sha256(converted).hexdigest() == row['sha256']
            assert hashlib.sha256(parts[b'DATA'][old_offset:old_offset+old_size]).hexdigest() == row['source_sha256']
            payload.extend(bytes((-len(payload)) % 16))
            index.extend(struct.pack('<3I', identity, len(payload), len(converted)))
            payload.extend(converted)
            sizes[identity] = (old_size, len(converted))
        patches, preserved_generators = [], []
        if b'HIRC' in parts:
            old_hirc = parts[b'HIRC']
            hirc = bytearray(old_hirc)
            cursor = 4
            for _ in range(struct.unpack_from('<I', hirc)[0]):
                kind, size = struct.unpack_from('<BI', hirc, cursor)
                start = cursor+5
                assert start+size <= len(hirc)
                if kind == 2:
                    identity, plugin = struct.unpack_from('<2I', hirc, start)
                    stream = hirc[start+8]
                    # Wwise Silence is a source generator, not ATRAC9 media.
                    # Keep its entire serialization rather than rewriting the
                    # plugin or interpreting its parameters as a DIDX record.
                    if plugin == 0x650002:
                        assert stream == 0
                        preserved_generators.append({'sound_id': identity, 'plugin_id': hex(plugin),
                            'sha256': hashlib.sha256(old_hirc[start:start+size]).hexdigest()})
                        cursor = start+size
                        continue
                    medium, old_size = struct.unpack_from('<2I', hirc, start+9)
                    assert plugin == 0xc0001 and stream == 0 and medium in sizes
                    assert old_size == sizes[medium][0]
                    struct.pack_into('<I', hirc, start+4, 0x10001)
                    struct.pack_into('<I', hirc, start+13, sizes[medium][1])
                    patches.append({'sound_id': identity, 'media_id': medium,
                                    'plugin_offset': start+4, 'size_offset': start+13})
                cursor = start+size
            assert cursor == len(hirc)
            allowed = {offset+i for patch in patches for offset in (patch['plugin_offset'], patch['size_offset']) for i in range(4)}
            assert all(i in allowed for i, (a, b) in enumerate(zip(old_hirc, hirc)) if a != b)
            parts[b'HIRC'] = bytes(hirc)
        parts[b'DIDX'], parts[b'DATA'] = bytes(index), bytes(payload)
        result = b''.join(chunk(tag, value) for tag, value in parts.items())
        parsed = bank_chunks(result)
        assert parsed[b'BKHD'] == bank_chunks(original)[b'BKHD']
        target = P/'research/horizon-other-pcm-bank-candidate'/source.name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(result)
        rows.append({'source': source.relative_to(P).as_posix(), 'source_sha256': hashlib.sha256(original).hexdigest(),
                     'path': target.relative_to(P).as_posix(), 'size': len(result),
                     'sha256': hashlib.sha256(result).hexdigest(), 'media': len(sizes), 'hirc_patches': patches,
                     'preserved_generators': preserved_generators,
                     'external_sound_objects_pending': b'HIRC' not in parsed})
    report = {'banks': rows, 'install_allowed': False,
              'limits': ['BKHD platform fields retained pending native acceptance certification.',
                         'Banks without HIRC need their external sound-object definitions located and composed.',
                         'No installation, audio playback, target-code execution or dynamic validation.']}
    (P/'research/horizon-other-pcm-bank-candidates.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'banks': len(rows), 'media': sum(r['media'] for r in rows),
                      'hirc_sound_patches': sum(len(r['hirc_patches']) for r in rows),
                      'external_graphs_pending': sum(r['external_sound_objects_pending'] for r in rows)}))


if __name__ == '__main__':
    main()
