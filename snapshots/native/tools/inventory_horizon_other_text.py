"""Find locally available Horizon text evidence without changing game files."""
from pathlib import Path
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(r'Horizon|Aloy|Banuk|Shield.Weaver|Watcher|Frozen Wilds|Firebreak|Nora|地平线|亚罗伊|阿洛伊|织盾|守望者|巴努克|诺拉|冰尘雪野', re.I)


def scan():
    rows, errors, count = [], [], 0
    for platform, directory in [('PS', ROOT / 'PS雷暴资源/nativePS4/common/text'),
                                ('PS-reference', ROOT / 'research/horizon-other-source/common/text'),
                                ('PC', ROOT / 'MHWI解包/chunk/common/text')]:
        for path in sorted(directory.rglob('*.gmd')):
            # English and simplified Chinese give an initial identity inventory.
            if not path.stem.endswith(('_eng', '_chS')):
                continue
            count += 1
            data = path.read_bytes()
            try:
                if len(data) < 40 or data[:4] != b'GMD\0':
                    raise ValueError('Unsupported GMD header')
                strings_count = struct.unpack_from('<I', data, 24)[0]
                size = struct.unpack_from('<I', data, 32)[0]
                if not 0 < size <= len(data) - 40:
                    raise ValueError('Invalid string pool size')
                strings = data[-size:].split(b'\0')
                if len(strings) != strings_count + 1 or strings[-1] != b'':
                    raise ValueError('String count mismatch')
                matches = []
                for index, value in enumerate(strings[:-1]):
                    text = value.decode('utf-8')
                    if PATTERN.search(text):
                        matches.append({'string_index': index, 'text': text})
                if matches:
                    rows.append({'platform': platform, 'path': str(path.relative_to(ROOT)),
                                 'sha256': hashlib.sha256(data).hexdigest(), 'matches': matches})
            except (ValueError, UnicodeError, struct.error) as error:
                errors.append({'path': str(path.relative_to(ROOT)), 'error': str(error)})
    report = {'scope': 'Locally available English/Chinese GMD text candidates only',
              'limits': ['Text mentions are candidates, not verified item/quest IDs or working features.',
                         'Missing PS text files and unnamed dependencies are outside this scan.'],
              'files_scanned': count, 'files': rows, 'errors': errors}
    output = ROOT / 'research/horizon-other-text-inventory.json'
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'files_scanned': count, 'matched_files': len(rows),
                      'matches': sum(len(r['matches']) for r in rows), 'errors': len(errors)}))
    for row in rows:
        for match in row['matches']:
            if row['platform'].startswith('PS'):
                print(row['path'], match['string_index'], match['text'].replace('\n', ' ')[:180])


if __name__ == '__main__':
    scan()
