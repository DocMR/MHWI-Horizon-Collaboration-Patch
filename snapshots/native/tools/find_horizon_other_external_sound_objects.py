"""Locate source media IDs in PC HIRC objects using bounded bank parsing."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]


def main():
    candidates = json.loads((P/'research/horizon-other-pcm-bank-candidates.json').read_text(encoding='utf-8'))
    external = {Path(r['source']).name for r in candidates['banks'] if r['external_sound_objects_pending']}
    converted = json.loads((P/'research/horizon-other-pcm-media-conversion.json').read_text(encoding='utf-8'))
    targets = {}
    for row in converted['media']:
        if row['bank'] in external:
            targets.setdefault(row['id'], []).append(row)
    matches, skipped, examined = [], [], 0
    base = P/'MHWI解包/chunk'
    for path in sorted((base/'sound/wwise/Windows').glob('*.nbnk')):
        examined += 1
        chunks = {}
        try:
            size = path.stat().st_size
            with path.open('rb') as stream:
                while stream.tell() < size:
                    header = stream.read(8)
                    assert len(header) == 8
                    tag, length = struct.unpack('<4sI', header)
                    assert stream.tell()+length <= size and tag not in chunks
                    if tag in (b'BKHD', b'HIRC'):
                        chunks[tag] = stream.read(length)
                    else:
                        chunks[tag] = None
                        stream.seek(length, 1)
            if b'HIRC' not in chunks:
                continue
            version = struct.unpack_from('<I', chunks[b'BKHD'])[0]
            if version != 120:
                skipped.append({'path': path.relative_to(base).as_posix(), 'reason': f'version {version}'})
                continue
            hirc = chunks[b'HIRC']
            cursor = 4
            for _ in range(struct.unpack_from('<I', hirc)[0]):
                kind, length = struct.unpack_from('<BI', hirc, cursor)
                start = cursor+5
                assert start+length <= len(hirc)
                if kind == 2 and length >= 17:
                    identity, plugin = struct.unpack_from('<2I', hirc, start)
                    stream_type = hirc[start+8]
                    media_id, media_size = struct.unpack_from('<2I', hirc, start+9)
                    if media_id in targets:
                        matches.append({'path': path.relative_to(base).as_posix(), 'hirc_offset': start,
                                        'sound_id': identity, 'plugin': hex(plugin), 'stream_type': stream_type,
                                        'media_id': media_id, 'media_size': media_size,
                                        'source_banks': [r['bank'] for r in targets[media_id]],
                                        'hirc_sha256': hashlib.sha256(hirc).hexdigest()})
                cursor = start+length
            assert cursor == len(hirc)
        except (AssertionError, struct.error) as error:
            skipped.append({'path': path.relative_to(base).as_posix(), 'reason': type(error).__name__})
    found = {r['media_id'] for r in matches}
    report = {'examined': examined, 'target_media_ids': len(targets), 'matches': matches,
              'unresolved_media_ids': sorted(set(targets)-found), 'skipped': skipped,
              'limits': ['Media-ID matches are candidates; bank ownership/loading and source semantics still need certification.',
                         'Only Wwise120 Sound object layout inspected; no banks changed or target code executed.']}
    (P/'research/horizon-other-external-sound-object-candidates.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'examined': examined, 'target_media': len(targets), 'matches': len(matches),
                      'unresolved': len(report['unresolved_media_ids']), 'skipped': len(skipped)}))


if __name__ == '__main__':
    main()
