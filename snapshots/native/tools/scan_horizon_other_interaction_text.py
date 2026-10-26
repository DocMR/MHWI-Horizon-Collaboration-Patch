"""Extract and scan source English non-quest text for remaining interactions."""
from pathlib import Path
import hashlib
import json
import re
import struct
from build_horizon_other_text_candidate import parse
from build_horizon_other_content_inventory import strings

P = Path(__file__).resolve().parents[1]


def main():
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    inventory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))['files']
    selected = [r for r in inventory if r['path'].startswith('common/text/')
                and '/quest/' not in r['path'] and r['path'].endswith('_eng.gmd')]
    out = P/'research/horizon-other-interaction-text-source'
    receipts, matches = [], []
    pattern = re.compile(r'Aloy|\bNora\b|Banuk|Frostclaw|Watcher|Bluegleam|Daemon|Horizon|Shield-Weaver|Thunderjaw|Firebreak', re.I)
    for name in sorted({r['archive'] for r in selected}):
        archive = ns['Archive'](Path(name))
        try:
            for row in [r for r in selected if r['archive'] == name]:
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                path = out/row['path']
                path.parent.mkdir(parents=True, exist_ok=True)
                if path.exists():
                    assert path.read_bytes() == data
                else:
                    path.write_bytes(data)
                if struct.unpack_from('<I', data, 20)[0] == 0:
                    # Unkeyed trace GMDs omit the 2048-byte hash buckets.
                    header = struct.unpack_from('<10I', data)
                    assert len(data) == 40+header[9]+1+header[8] and header[7] == 0
                    values = [text.encode('utf-8') for text in strings(path)]+[b'']
                    keys = {}
                else:
                    _, values, keys = parse(data)
                receipts.append(dict(**row, output=path.relative_to(P).as_posix(), sha256=hashlib.sha256(data).hexdigest()))
                for i, value in enumerate(values[:-1]):
                    text = value.decode('utf-8')
                    if pattern.search(text):
                        matches.append(dict(path=row['path'], index=i, key=keys[i].decode('utf-8') if i in keys else None, text=text))
        finally:
            archive.f.close()
    report = dict(status='Source-text candidates; mentions do not prove runtime ownership',
        scanned_files=len(receipts), source_files=receipts, matches=matches,
        limits=['English non-quest GMDs in the existing archive inventory only.',
                'Keywords can match unrelated watcher/daemon text and Stormslinger content; scope requires record/consumer evidence.',
                'No match is not proof of no other mechanics or content.',
                'Reference extraction and static text parsing only; no install or native mechanism execution.'])
    (P/'research/horizon-other-interaction-text-scan.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(files=len(receipts), matches=len(matches), matched_paths=sorted({m['path'] for m in matches}))))


if __name__ == '__main__':
    main()
