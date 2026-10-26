"""Restore both page-one keys of native tutorial group123 in all14 languages."""
from pathlib import Path
import hashlib
import json
import re
import shutil
from build_horizon_other_text_candidate import parse
from merge_horizon_other_gmd import merge

P = Path(__file__).resolve().parents[1]
BASE = P/'staging/Horizon-complete-presentation-payload-20261021-c6'
OUT = P/'staging/Horizon-complete-tutorial-payload-20261021-c7'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def values(data):
    _, strings, keys = parse(data)
    return {key.decode(): strings[index].decode('utf-8') for index, key in keys.items()}


def main():
    if OUT.exists():
        raise ValueError('Preserve frozen tutorial candidate')
    review_path = P/'research/horizon-other-text-language-coverage.json'
    review = json.loads(review_path.read_text(encoding='utf-8-sig'))
    drafts = {r['language']: r for r in review['tutorial_page1_project_drafts_2026_10_20']['files']}
    revisions = {(r['language'], r['key']): r['after_draft']
                 for r in review['continued_language_and_pc_icon_review_2026_10_20']['draft_revisions']}
    assert set(drafts) == {'ara', 'pol', 'ptB', 'rus'}
    outputs, receipts = {}, []
    wanted = ['PANEL_171_HEAD', 'PANEL_171_TEXT']
    for language in ['ara','chS','chT','eng','fre','ger','ita','jpn','kor','pol','ptB','rus','spa','spL']:
        relative = f'common/text/steam/panel_tutorial_{language}.gmd'
        before = (BASE/'nativePC'/relative).read_bytes()
        old = values(before)
        assert all(old[key] == 'HARDUMMY' for key in wanted)
        if language in drafts:
            row = drafts[language]
            for dependency in row['term_dependencies']:
                assert sha((P/dependency['path']).read_bytes()) == dependency['sha256']
            selected = {r['key']: revisions.get((language, r['key']), r['proposed_text'])
                        for r in row['changes_for_review']}
            origin = 'Retained project translation; proper-name revisions reviewed'
            source_sha = sha(review_path.read_bytes())
        else:
            source = P/f'research/horizon-other-source/common/text/ps4/panel_tutorial_{language}.gmd'
            source_bytes = source.read_bytes()
            ps = values(source_bytes)
            selected = {key: ps[key] for key in wanted}
            origin = 'Retained original PS tutorial page'
            source_sha = sha(source_bytes)
        assert set(selected) == set(wanted)
        assert all(text and text != 'HARDUMMY' and '\0' not in text and '\ufffd' not in text for text in selected.values())
        assert '<ICON PL_R2>' in selected['PANEL_171_TEXT']
        # PL_R2 is an existing native PC input token, not a hard-coded PS key.
        native = values((P/f'MHWI解包/chunk/common/text/steam/panel_tutorial_{language}.gmd').read_bytes())
        assert any('<ICON PL_R2>' in text and text != 'HARDUMMY' for text in native.values())
        for text in selected.values():
            assert len(re.findall(r'<STYL\s[^>]+>', text)) == text.count('</STYL>')
        changes = [{'key': key, 'before': old[key], 'after': selected[key]} for key in wanted]
        after = merge(before, changes)
        final = values(after)
        assert all(final[key] not in ('', 'HARDUMMY') for key in [*wanted,'PANEL_172_HEAD','PANEL_172_TEXT'])
        assert all(final[key] == text for key, text in old.items() if key not in wanted)
        outputs[relative] = after
        receipts.append({'path': relative, 'language': language, 'origin': origin,
                         'source_sha256': source_sha, 'changes': changes,
                         'before_sha256': sha(before), 'output_sha256': sha(after)})
    shutil.copytree(BASE, OUT)
    for relative, data in outputs.items():
        (OUT/'nativePC'/relative).write_bytes(data)
    for f in (BASE/'nativePC').rglob('*'):
        if f.is_file() and f.relative_to(BASE/'nativePC').as_posix() not in outputs:
            assert f.read_bytes() == (OUT/'nativePC'/f.relative_to(BASE/'nativePC')).read_bytes()
    result = {'status': 'native_group123_both_pages_restored_in_14_languages_static_only',
              'candidate': OUT.relative_to(P).as_posix(), 'resource_files': 686,
              'files': receipts, 'native_pc_input_token_retained': '<ICON PL_R2>',
              'correction': 'C4/C5 preserved PANEL171 byte values, but those values were still PC HARDUMMY; the standalone baseline did not ship this tutorial table.',
              'limits': ['Actual group request, repeat-read flag, controller/keyboard icons and page layout remain live pending',
                         'Four languages are project translations; no external linguistic certification'],
              'game_modified': False}
    (P/'research/horizon-complete-tutorial-page-one-20261021.json').write_text(
        json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status', 'resource_files']}))


if __name__ == '__main__':
    main()
