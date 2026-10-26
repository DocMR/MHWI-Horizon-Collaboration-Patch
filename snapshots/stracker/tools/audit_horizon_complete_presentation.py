"""Check final retained presentation resources and bounded localization scope."""
from pathlib import Path
import hashlib
import json
from build_horizon_other_text_candidate import parse

P = Path(__file__).resolve().parents[1]
M = P/'staging/Horizon-complete-integration-20261021'
C = P/'staging/Horizon-complete-tutorial-payload-20261021-c7/nativePC'
PC = P/'MHWI解包/chunk'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    groups = []
    for name, hash_field in [('model-materials.json','sha256'), ('equipment-textures.json','output_sha256'),
                             ('equip-controls.json','sha256'), ('physics-candidates.json','sha256'),
                             ('effect-audio-references.json','sha256')]:
        data = json.loads((M/'merge'/name).read_text(encoding='utf-8-sig'))
        for row in data['files']:
            assert sha(C/row['path']) == row[hash_field], row['path']
        groups.append({'manifest': name, 'files': len(data['files']), 'sha256': sha(M/'merge'/name)})
    interactions = json.loads((M/'merge/interaction-table-baselines.json').read_text(encoding='utf-8-sig'))
    for row in interactions['files']:
        assert row['identical'] and sha(PC/row['path']) == row['pc_sha256'] == row['source_sha256']
    module = json.loads((M/'module-manifest.json').read_text(encoding='utf-8-sig'))
    fixes = json.loads((P/'research/horizon-complete-language-fixes-20261021.json').read_text(encoding='utf-8-sig'))
    changed_quest = {r['target']: r['after_sha256'] for r in fixes['fixes'] if r['target'].startswith('common/text/quest/')}
    same, amended = 0, 0
    for row in module['resources']:
        digest = sha(C/row['path'])
        if digest == row['sha256']:
            same += 1
        else:
            assert changed_quest.get(row['path']) == digest, row['path']
            amended += 1
    assert same == 417 and amended == 4
    tutorial_rows = []
    for language in ['ara','chS','chT','eng','fre','ger','ita','jpn','kor','pol','ptB','rus','spa','spL']:
        f = C/f'common/text/steam/panel_tutorial_{language}.gmd'
        _, strings, keys = parse(f.read_bytes())
        by_key = {key.decode(): strings[i].decode('utf-8') for i,key in keys.items()}
        for key in ['PANEL_171_HEAD','PANEL_171_TEXT','PANEL_172_HEAD','PANEL_172_TEXT']:
            assert by_key[key] not in ('','HARDUMMY') and '\ufffd' not in by_key[key]
        tutorial_rows.append({'language': language, 'sha256': sha(f)})
    files = [{'path': f.relative_to(C).as_posix(), 'size': f.stat().st_size, 'sha256': sha(f)}
             for f in sorted(C.rglob('*')) if f.is_file()]
    assert len(files) == 686
    result = {'status': 'major5_presentation_static_preparation_complete_live_pending',
              'candidate': C.parent.relative_to(P).as_posix(), 'resource_files': len(files),
              'asset_groups': groups, 'original_module_resources_unchanged': same,
              'original_module_quest_texts_explicitly_amended': amended,
              'original_pc_interaction_tables_retained': len(interactions['files']),
              'tutorial_both_pages_all14_languages': tutorial_rows,
              'motion_fix': 'research/horizon-complete-guildcard-motion-20261021.json',
              'shared_audio_composition': 'research/horizon-complete-mechanism-payload-20261021.json',
              'newly_resolved': ['PS guildcard pose262 absent from PC resource, now selectively appended',
                                 'Native tutorial group123 page171 was still HARDUMMY, now restored'],
              'live_pending': ['Guildcard pose/title unlock/select/confirm/cancel/save/reload',
                               'Tutorial first request, both pages, repeat suppression/reopen and keyboard/controller icons',
                               'Hunter/Palico/bow models, textures, physics, VFX and sound native playback',
                               'Real text layout/clipping and Arabic shaping/RTL'],
              'limits': ['Checks cover retained explicit file/reference inventories; no exhaustive arbitrary runtime dependency claim',
                         'Four PS-missing languages use project translations, not official localization or external native-speaker certification'],
              'game_modified': False, 'full_project_complete': False, 'files': files}
    (P/'research/horizon-complete-presentation-readiness-20261021.json').write_text(
        json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status','resource_files','original_module_resources_unchanged','original_module_quest_texts_explicitly_amended']}))


if __name__ == '__main__':
    main()
