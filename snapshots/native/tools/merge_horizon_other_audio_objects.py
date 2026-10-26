"""Apply reviewed stable-Sound-ID bank edits to a separate output file."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from convert_horizon_other_atrac9_media import bank_chunks, chunk


def sound_objects(hirc):
    cursor, objects = 4, {}
    for _ in range(struct.unpack_from('<I', hirc)[0]):
        kind, size = struct.unpack_from('<BI', hirc, cursor)
        start = cursor+5
        assert start+size <= len(hirc)
        if kind == 2:
            identity = struct.unpack_from('<I', hirc, start)[0]
            assert identity not in objects
            objects[identity] = (start, hirc[start:start+size])
        cursor = start+size
    assert cursor == len(hirc)
    return objects


def merge(data, changes):
    parts = bank_chunks(data)
    assert struct.unpack_from('<I', parts[b'BKHD'])[0] == 120
    original = parts[b'HIRC']
    objects = sound_objects(original)
    hirc = bytearray(original)
    edited = set()
    for row in changes:
        assert row['sound_id'] not in edited
        edited.add(row['sound_id'])
        if row['sound_id'] not in objects:
            raise ValueError(f"Missing Sound object {row['sound_id']}")
        offset, payload = objects[row['sound_id']]
        digest = hashlib.sha256(payload).hexdigest()
        if digest == row['after_sha256']:
            continue
        if digest != row['before_sha256']:
            raise ValueError(f"Conflicting Sound object {row['sound_id']}")
        for edit in row['edits']:
            start = offset+edit['offset']
            before, after = bytes.fromhex(edit['before']), bytes.fromhex(edit['after'])
            assert len(before) == len(after) and hirc[start:start+len(before)] == before
            hirc[start:start+len(after)] = after
        assert hashlib.sha256(hirc[offset:offset+len(payload)]).hexdigest() == row['after_sha256']
    parts[b'HIRC'] = bytes(hirc)
    result = b''.join(chunk(tag, payload) for tag, payload in parts.items())
    actual = bank_chunks(result)
    assert all(actual[tag] == payload for tag, payload in bank_chunks(data).items() if tag != b'HIRC')
    final = sound_objects(actual[b'HIRC'])
    assert all(final[identity][1] == payload for identity, (_, payload) in objects.items() if identity not in edited)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--delta', type=Path, required=True)
    parser.add_argument('--module', choices=['horizon-other','stormslinger'], default='horizon-other')
    parser.add_argument('--target', required=True)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error('Use a separate output path')
    manifest = json.loads(args.delta.read_text(encoding='utf-8'))
    assert manifest['schema_version'] == 1 and manifest['module'] == args.module
    rows = [r for r in manifest['files'] if r['target'] == args.target]
    assert len(rows) == 1
    result = merge(args.input.read_bytes(), rows[0]['changes'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)


if __name__ == '__main__':
    main()
