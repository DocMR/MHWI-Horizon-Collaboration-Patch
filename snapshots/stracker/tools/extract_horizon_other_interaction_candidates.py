"""Extract the five identified interaction text families without installation."""
from pathlib import Path
import hashlib
import json
import re

P = Path(__file__).resolve().parents[1]
TABLES = {'l_delivery', 'GC_Pose', 'GC_Title_1', 'panel_tutorial', 'panel_tutorial_cat'}


def main():
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    inventory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))['files']
    selected = []
    for row in inventory:
        match = re.fullmatch(r'common/text/ps4/(.+)_([^/_]+)\.gmd', row['path'])
        if match and match[1] in TABLES:
            selected.append(row)
    assert len(selected) == 50
    receipts = []
    for name in sorted({r['archive'] for r in selected}):
        archive = ns['Archive'](Path(name))
        try:
            for row in [r for r in selected if r['archive'] == name]:
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                target = P/'research/horizon-other-source'/row['path']
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                receipts.append(dict(**row, sha256=hashlib.sha256(data).hexdigest(), output=target.relative_to(P).as_posix()))
        finally:
            archive.f.close()
    (P/'research/horizon-other-interaction-source-receipt.json').write_text(
        json.dumps(dict(files=receipts, status='Source references only; extract does not activate interactions'), ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(files=len(receipts), tables=len(TABLES))))


if __name__ == '__main__':
    main()
