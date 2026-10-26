"""Apply six reviewed Arabic player-address fixes to a new frozen candidate."""
from pathlib import Path
import hashlib
import json
import re
import shutil
from merge_horizon_other_gmd import merge
from build_horizon_other_text_candidate import parse

P = Path(__file__).resolve().parents[1]
BASE = P/'staging/Horizon-complete-interaction-payload-20261021-c4'
OUT = P/'staging/Horizon-complete-language-payload-20261021-c5'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    source = P/'research/horizon-other-text-language-coverage.json'
    report = json.loads(source.read_text(encoding='utf-8-sig'))
    rows = [r for r in report['item6_consolidated_review_handoff_2026_10_20']['changes_for_review_only']
            if r.get('review_group') == 'arabic_address_6']
    assert len(rows) == 6 and len({r['target'] for r in rows}) == 6
    outputs = {}
    fixes = []
    for row in rows:
        before = (BASE/'nativePC'/row['target']).read_bytes()
        replacement = row['after_draft']
        # These six edits change player-address voice only. Keep native labels,
        # inline style/control tags and original line structure intact.
        assert re.findall(r'<[^>]*>', row['before']) == re.findall(r'<[^>]*>', replacement)
        assert row['before'].count('\r\n') == replacement.count('\r\n')
        assert not any(c in replacement for c in ['\0', '\ufffd'])
        changes = [{'key': row['key'], 'before': row['before'], 'after': replacement}]
        after = merge(before, changes)
        outputs[row['target']] = after
        fixes.append({'target': row['target'], 'changes': changes, 'reason': row['reason'],
                      'review': 'Impersonal player-address wording; character pronouns and reward giver retained',
                      'before_sha256': sha(before), 'after_sha256': sha(after)})
    if not OUT.exists():
        shutil.copytree(BASE, OUT)
        for relative, data in outputs.items():
            (OUT/'nativePC'/relative).write_bytes(data)
    else:
        # A prior validation failure may leave the completed output. Recheck
        # its exact inventory/bytes without changing or silently replacing it.
        expected = {f.relative_to(BASE/'nativePC').as_posix() for f in (BASE/'nativePC').rglob('*') if f.is_file()}
        actual = {f.relative_to(OUT/'nativePC').as_posix() for f in (OUT/'nativePC').rglob('*') if f.is_file()}
        assert expected == actual
        for relative, data in outputs.items():
            assert (OUT/'nativePC'/relative).read_bytes() == data
    preserved = 0
    for f in (BASE/'nativePC').rglob('*'):
        if f.is_file() and f.relative_to(BASE/'nativePC').as_posix() not in outputs:
            assert f.read_bytes() == (OUT/'nativePC'/f.relative_to(BASE/'nativePC')).read_bytes()
            preserved += 1
    # PANEL_171 belongs to the accepted Stormslinger baseline. This pass must
    # not apply the old other-module page-one draft over that implementation.
    panel171 = []
    for f in (BASE/'nativePC/common/text/steam').glob('panel_tutorial_*.gmd'):
        if f.name.startswith('panel_tutorial_cat_'):
            continue
        target = OUT/'nativePC'/f.relative_to(BASE/'nativePC')
        _, bv, bk = parse(f.read_bytes())
        _, av, ak = parse(target.read_bytes())
        assert bk == ak
        keys = [i for i, key in bk.items() if key.startswith(b'PANEL_171_')]
        assert keys and all(bv[i] == av[i] for i in keys)
        panel171.append(f.name)
    assert len(panel171) == 14
    result = {'status': 'six_arabic_voice_fixes_composed_static_only',
              'candidate': OUT.relative_to(P).as_posix(), 'source': source.relative_to(P).as_posix(),
              'source_sha256': sha(source.read_bytes()), 'fixes': fixes,
              'resource_files': preserved + len(outputs), 'unrelated_files_preserved': preserved,
              'stormslinger_panel171_languages_preserved': panel171,
              'limits': ['Arabic contextual shaping/RTL/line clipping not live verified',
                         'Not an external native-speaker linguistic certification'],
              'game_modified': False}
    (P/'research/horizon-complete-language-fixes-20261021.json').write_text(
        json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status', 'resource_files', 'unrelated_files_preserved']}))


if __name__ == '__main__':
    main()
