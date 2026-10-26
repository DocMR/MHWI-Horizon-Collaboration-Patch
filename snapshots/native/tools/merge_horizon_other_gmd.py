"""Single-file three-way text merger for the final integration chat.

This utility transforms GMD files only. It does not load or test game code.
"""
from pathlib import Path
import argparse
import json
import struct
from build_horizon_other_text_candidate import parse


def merge(data, changes):
    offset, before, keys = parse(data)
    by_key = {}
    for index, key in keys.items():
        if key in by_key:
            raise ValueError(f'Duplicate target GMD key: {key!r}')
        by_key[key] = index
    after = before.copy()
    seen, conflicts, changed = set(), [], []
    for entry in changes:
        key = entry['key'].encode('utf-8')
        if key in seen:
            raise ValueError(f'Duplicate delta key: {key!r}')
        seen.add(key)
        if key not in by_key:
            conflicts.append({'key': entry['key'], 'reason': 'missing target key'})
            continue
        index = by_key[key]
        expected = entry['before'].encode('utf-8')
        replacement = entry['after'].encode('utf-8')
        if b'\0' in expected or b'\0' in replacement:
            raise ValueError('Embedded NUL in delta text')
        if before[index] not in (expected, replacement):
            conflicts.append({'key': entry['key'], 'reason': 'target differs from both base and replacement',
                              'current': before[index].decode('utf-8')})
            continue
        after[index] = replacement
        changed.append(index)
    if conflicts:
        raise ValueError(json.dumps({'conflicts': conflicts}, ensure_ascii=False))
    result = bytearray(data[:offset] + b'\0'.join(after))
    struct.pack_into('<I', result, 32, len(result)-offset)
    actual_offset, actual, actual_keys = parse(result)
    assert actual_offset == offset and actual_keys == keys and actual == after
    assert result[:32] == data[:32] and result[36:offset] == data[36:offset]
    assert all(actual[i] == before[i] for i in range(len(before)) if i not in changed)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--delta', type=Path, required=True)
    parser.add_argument('--target', required=True, help='Exact relative target from delta files')
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.input.resolve() == args.output.resolve():
        parser.error('Use a separate output file for review')
    manifest = json.loads(args.delta.read_text(encoding='utf-8'))
    if manifest.get('schema_version') != 1 or manifest.get('module') != 'horizon-other':
        parser.error('Unsupported delta manifest')
    matches = [row for row in manifest['files'] if row['target'] == args.target]
    if len(matches) != 1:
        parser.error('Target must have exactly one delta definition')
    result = merge(args.input.read_bytes(), matches[0]['changes'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print(f'Wrote merged GMD for review: {args.output}')


if __name__ == '__main__':
    main()
