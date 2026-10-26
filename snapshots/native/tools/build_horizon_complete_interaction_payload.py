"""Merge retained interaction text and approved permanent-reward descriptions."""
from pathlib import Path
import copy
import hashlib
import json
import shutil
from merge_horizon_other_gmd import merge

P = Path(__file__).resolve().parents[1]
M = P/'staging/Horizon-complete-integration-20261021'
BASE = P/'staging/Horizon-complete-mechanism-payload-20261021-c3'
OUT = P/'staging/Horizon-complete-interaction-payload-20261021-c4'
TABLES = {'GC_Pose', 'GC_Title_1', 'panel_tutorial', 'panel_tutorial_cat', 'skill', 'skill_pt'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if OUT.exists():
        raise ValueError('Preserve frozen interaction candidate')
    original = json.loads((M/'merge/text-deltas.json').read_text(encoding='utf-8-sig'))
    policy_path = P/'research/horizon-other-interaction-translations.json'
    policy_bytes = policy_path.read_bytes()
    policy = json.loads(policy_bytes)['reward_policy_overrides']
    selected = []
    outputs = {}
    overrides = 0
    unchanged = 0
    for original_row in original['files']:
        filename = original_row['target'].rsplit('/', 1)[-1]
        table, language = filename[:-4].rsplit('_', 1)
        if table not in TABLES:
            continue
        row = copy.deepcopy(original_row)
        native = (P/'MHWI解包/chunk'/row['target']).read_bytes()
        assert sha(native) == row['base_sha256']
        by_key = {change['key']: change for change in row['changes']}
        for key, text in policy.get(language, {}).get(table, {}).items():
            # Existing reviewed deltas already select all eleven reward docs.
            assert key in by_key and text and '\0' not in text
            by_key[key]['after'] = text
            overrides += 1
        current = BASE/'nativePC'/row['target']
        before = current.read_bytes() if current.exists() else native
        after = merge(before, row['changes'])
        assert merge(after, row['changes']) == after
        selected.append(row)
        if after != before:
            outputs[row['target']] = after
        else:
            unchanged += 1
    assert len(selected) == 84 and overrides == 154 and unchanged == 28
    shutil.copytree(BASE, OUT)
    for relative, raw in outputs.items():
        target = OUT/'nativePC'/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
    for source in (BASE/'nativePC').rglob('*'):
        if source.is_file():
            relative = source.relative_to(BASE/'nativePC').as_posix()
            if relative not in outputs:
                assert source.read_bytes() == (OUT/'nativePC'/relative).read_bytes()
    delta_path = M/'merge/complete-interaction-text-deltas-c4.json'
    assert not delta_path.exists()
    delta_path.write_text(json.dumps({'schema_version': 1, 'module': 'horizon-other',
                                     'policy': 'Stable-key merge; permanent collaboration reward descriptions',
                                     'reward_policy_source': policy_path.relative_to(P).as_posix(),
                                     'reward_policy_sha256': sha(policy_bytes),
                                     'files': selected}, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    files = [{'path': f.relative_to(OUT).as_posix(), 'bytes': f.stat().st_size,
              'sha256': sha(f.read_bytes())}
             for f in sorted((OUT/'nativePC').rglob('*')) if f.is_file()]
    report = {'status': 'interaction_text_composed_no_install_or_live_layout_acceptance',
              'candidate': OUT.relative_to(P).as_posix(), 'resource_files': len(files),
              'tables_reviewed': len(selected), 'text_files_added_or_changed': len(outputs),
              'native_skill_text_files_unchanged_not_redundantly_shipped': unchanged,
              'permanent_reward_description_updates': overrides,
              'delta': delta_path.relative_to(P).as_posix(), 'delta_sha256': sha(delta_path.read_bytes()),
              'limits': ['Four missing PS languages use retained project translations',
                         'Linguistic review, Arabic RTL and in-game layout still pending',
                         'Text restoration does not establish guildcard unlock or tutorial invocation acceptance'],
              'files': files}
    (P/'research/horizon-complete-interaction-payload-20261021.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: report[k] for k in ['status', 'resource_files', 'text_files_added_or_changed',
                                           'permanent_reward_description_updates']}))


if __name__ == '__main__':
    main()
