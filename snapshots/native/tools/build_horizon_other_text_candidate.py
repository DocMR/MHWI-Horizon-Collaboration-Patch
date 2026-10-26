"""Restore proven equipment text into isolated PC candidates, preserving keys."""
from pathlib import Path
import hashlib
import json
import re
import struct
import zlib

P = Path(__file__).resolve().parents[1]
OUT = P / 'staging/Horizon-other-text-candidate/nativePC/common/text/steam'


def parse(data):
    header = struct.unpack_from('<10I', data)
    assert header[:2] == (0x444D47, 0x10302)
    start = 40 + header[9] + 1
    pool = start + header[5] * 32 + 2048 + header[7]
    assert pool + header[8] == len(data)
    keys = data[pool-header[7]:pool]
    mapping = {}
    for i in range(header[5]):
        index, crc2, crc3, padding, offset, _ = struct.unpack_from('<IIIIQQ', data, start+i*32)
        assert index < header[6] and offset < len(keys) and padding == 0xCDCDCDCD
        end = keys.find(b'\0', offset)
        assert end >= offset
        key = keys[offset:end]
        assert zlib.crc32(key*2) ^ 0xFFFFFFFF == crc2
        assert zlib.crc32(key*3) ^ 0xFFFFFFFF == crc3
        assert index not in mapping
        mapping[index] = key
    values = data[pool:].split(b'\0')
    assert len(values) == header[6]+1 and not values[-1]
    return pool, values, mapping


def main():
    audit = json.loads((P / 'research/horizon-other-equipment-audit.json').read_text(encoding='utf-8'))
    content = json.loads((P / 'research/horizon-other-content-inventory.json').read_text(encoding='utf-8'))
    indices = {'armor': set(), 'ot_armor': set(), 'bow': set(), 'ot_weapon': set()}
    palico = json.loads((P / 'research/horizon-other-palico-weapon.json').read_text(encoding='utf-8'))
    for row in palico['weapon_candidates']:
        if row['platform'] == 'PS':
            indices['ot_weapon'].update((row['name_index'], row['description_index']))
    for key, table in [('armor', 'armor'), ('palico_armor', 'ot_armor')]:
        for row in audit[key]:
            if row['platform'] == 'PS':
                indices[table].update((row['name_index'], row['description_index']))
    raw = (P / 'research/horizon-other-source/common/equip/bow.wp_dat_g').read_bytes()
    for row in content['bow_records']:
        if row['platform'] == 'PS':
            indices['bow'].update(struct.unpack_from('<HH', raw, row['file_offset']+61))
    indices['armor_series'] = {row['string_index'] for row in content['equipment_text_candidates'] if row['table'] == 'armor_series'}
    indices['ou_bow'] = {row['string_index'] for row in content['equipment_text_candidates'] if row['table'] == 'ou_bow'}
    indices['ot_series'] = {row['string_index'] for row in content['equipment_text_candidates'] if row['table'] == 'ot_series'}
    indices['wep_series'] = {46}
    indices['skill'] = {934, 935}
    indices['skill_pt'] = {627, 628}
    indices['item'] = set()
    interaction_keys = {
        'l_delivery': {'57_TITLE_D', '57_CLIENT_D', '57_TEXT_D', '57_RESULT_D'},
        'GC_Pose': {'GC_Pose_13_id', 'GC_Pose_13_doc'},
        'GC_Title_1': {f'GC_Title_1_{identity}_{suffix}'
                       for identity in (*range(782, 787), *range(1500, 1505))
                       for suffix in ('id', 'doc')},
        'panel_tutorial': {'PANEL_172_HEAD', 'PANEL_172_TEXT'},
        'panel_tutorial_cat': {'PANEL_17_CAT_TEXT'},
    }
    for table, selected_keys in interaction_keys.items():
        _, _, keys = parse((P/f'research/horizon-other-source/common/text/ps4/{table}_eng.gmd').read_bytes())
        indices[table] = {index for index, key in keys.items() if key.decode('utf-8') in selected_keys}
        assert len(indices[table]) == len(selected_keys)
    _, item_values, item_keys = parse((P/'research/horizon-other-source/common/text/ps4/item_eng.gmd').read_bytes())
    item_names = {'Watcher Lens', 'Nora Brave Trophy', 'Aloy Ticket',
                  'Banuk Warrior Symbol', 'Bluegleam', 'Daemon Ticket'}
    found = set()
    for i, value in enumerate(item_values):
        if value.decode('utf-8') in item_names:
            assert item_keys[i].endswith(b'_NAME')
            assert item_keys[i+1] == item_keys[i][:-5]+b'_EXP'
            indices['item'].update((i, i+1))
            found.add(value.decode('utf-8'))
    assert found == item_names
    project_path = P/'research/horizon-other-material-translations.json'
    project_bytes = project_path.read_bytes()
    project = json.loads(project_bytes)
    shared_path = P/project['shared_source']
    shared_bytes = shared_path.read_bytes()
    shared = json.loads(shared_bytes)['languages']
    project_materials = {}
    for language, values in project['languages'].items():
        values = values.copy()
        assert len(shared[language]) == len(project['shared_keys'])
        for key, value in zip(project['shared_keys'], shared[language]):
            assert key not in values
            values[key] = value
        assert set(values) == {item_keys[i].decode('utf-8') for i in indices['item']}
        assert all(value and value != 'HARDUMMY' and '\0' not in value for value in values.values())
        project_materials[language] = values
    bow_project_path = P/'research/horizon-other-bow-translations.json'
    bow_project_bytes = bow_project_path.read_bytes()
    bow_project = json.loads(bow_project_bytes)['languages']
    palico_project_path = P/'research/horizon-other-palico-translations.json'
    palico_project_bytes = palico_project_path.read_bytes()
    palico_project = json.loads(palico_project_bytes)['languages']
    table_sources = {table: (bow_project_path, bow_project_bytes, bow_project)
                     for table in ('bow', 'ou_bow', 'wep_series')}
    table_sources.update({table: (palico_project_path, palico_project_bytes, palico_project)
                          for table in ('ot_armor', 'ot_weapon', 'ot_series')})
    hunter_path = P/'research/horizon-other-hunter-translations.json'
    hunter_bytes = hunter_path.read_bytes()
    hunter = json.loads(hunter_bytes)
    assert hunter['slot_order'] == ['head', 'body', 'arms', 'waist', 'legs']
    _, _, armor_keys = parse((P/'research/horizon-other-source/common/text/ps4/armor_eng.gmd').read_bytes())
    hunter_tables = {}
    for language, values in hunter['languages'].items():
        translated_armor = values['armor_descriptions'].copy()
        for row in audit['armor']:
            if row['platform'] != 'PS':
                continue
            names = values['armor_names_by_series'][str(row['set_layered_id'])]
            assert len(names) == 5
            key = armor_keys[row['name_index']].decode('utf-8')
            assert key not in translated_armor
            translated_armor[key] = names[row['slot']]
        _, native, native_keys = parse((P/f'MHWI解包/chunk/common/text/steam/armor_{language}.gmd').read_bytes())
        reverse = {key.decode('utf-8'): i for i, key in native_keys.items()}
        for key in hunter['retained_keys']:
            assert key not in translated_armor and native[reverse[key]] not in (b'', b'HARDUMMY')
            translated_armor[key] = native[reverse[key]].decode('utf-8')
        hunter_tables[language] = {'armor': translated_armor, 'armor_series': values['armor_series']}
    table_sources.update({table: (hunter_path, hunter_bytes, hunter_tables)
                          for table in ('armor', 'armor_series')})
    interaction_path = P/'research/horizon-other-interaction-translations.json'
    interaction_bytes = interaction_path.read_bytes()
    interaction = json.loads(interaction_bytes)
    reward_policy = interaction['reward_policy_overrides']
    reward_policy_keys = {
        'GC_Pose': {'GC_Pose_13_doc'},
        'GC_Title_1': {f'GC_Title_1_{identity}_doc'
                       for identity in (*range(782, 787), *range(1500, 1505))},
    }
    assert set(reward_policy) == {'ara', 'chS', 'chT', 'eng', 'fre', 'ger', 'ita',
                                  'jpn', 'kor', 'pol', 'ptB', 'rus', 'spa', 'spL'}
    for tables in reward_policy.values():
        assert set(tables) == set(reward_policy_keys)
        for table, values in tables.items():
            assert set(values) == reward_policy_keys[table]
            assert all(text and '\0' not in text for text in values.values())
    weapon_path = P/'research/weapon-localization-translations.json'
    weapon_bytes = weapon_path.read_bytes()
    weapon_names = json.loads(weapon_bytes)['languages']
    interaction_tables, interaction_dependencies = {}, {}

    def native_term(table, language, dependencies, *, key=None, english=None, folder='steam'):
        root = P/'MHWI解包/chunk/common/text'
        if folder:
            root /= folder
        path = root/f'{table}_{language}.gmd'
        data = path.read_bytes()
        _, values, keys = parse(data)
        reverse = {value.decode('utf-8'): i for i, value in keys.items()}
        if key is None:
            english_path = root/f'{table}_eng.gmd'
            english_data = english_path.read_bytes()
            _, ev, ek = parse(english_data)
            matches = [ek[i].decode('utf-8') for i in ek if ev[i].decode('utf-8') == english]
            assert matches, (table, english)
            key = matches[0]
            dependencies.append({'path': english_path.relative_to(P).as_posix(), 'sha256': hashlib.sha256(english_data).hexdigest(), 'key': key})
        value = values[reverse[key]].decode('utf-8')
        assert value not in ('', 'HARDUMMY', 'Unavailable')
        dependencies.append({'path': path.relative_to(P).as_posix(), 'sha256': hashlib.sha256(data).hexdigest(), 'key': key})
        return value

    for language, tables in interaction['languages'].items():
        dependencies = []
        terms = {'shield': hunter['languages'][language]['armor_series']['AMR_SERIES_472_LABEL'],
                 'adept': weapon_names[language]['lbg'][2],
                 'skill': native_term('skill', language, dependencies, key='HORIZON_SKILL', folder='vfont'),
                 'bolt': native_term('cm_status', language, dependencies, key='STATUS_BOWGUN_SPECIAL_SS', folder=None)}
        translated = {table: {key: text.format_map(terms) for key, text in values.items()}
                      for table, values in tables.items()}
        for table, copies in interaction['pc_copies'].items():
            for key, english in copies.items():
                assert key not in translated[table]
                translated[table][key] = native_term(table, language, dependencies, english=english)
        interaction_tables[language] = translated
        for table in tables:
            interaction_dependencies[(table, language)] = dependencies+[
                {'path': hunter_path.relative_to(P).as_posix(), 'sha256': hashlib.sha256(hunter_bytes).hexdigest()},
                {'path': weapon_path.relative_to(P).as_posix(), 'sha256': hashlib.sha256(weapon_bytes).hexdigest()}]
    table_sources.update({table: (interaction_path, interaction_bytes, interaction_tables)
                          for table in interaction_keys})
    receipts, deltas, coverage = [], [], []
    for table, selected in indices.items():
        source_folder = 'vfont' if table in ('skill', 'skill_pt') else 'ps4'
        target_folder = 'vfont' if source_folder == 'vfont' else 'steam'
        source_root = P/'research/horizon-other-source/common/text'/source_folder
        _, english_values, english_keys = parse((source_root/f'{table}_eng.gmd').read_bytes())
        languages = sorted(match[1] for path in source_root.glob(f'{table}_*.gmd')
                           if (match := re.fullmatch(re.escape(table)+r'_([^_]+)\.gmd', path.name)))
        assert 'eng' in languages and 'chS' in languages
        pc_root = P/'MHWI解包/chunk/common/text'/target_folder
        pc_languages = sorted(match[1] for path in pc_root.glob(f'{table}_*.gmd')
                              if (match := re.fullmatch(re.escape(table)+r'_([^_]+)\.gmd', path.name)))
        missing = sorted(set(pc_languages)-set(languages))
        translated_tables = ('item', *table_sources)
        project_languages = missing if table in translated_tables else []
        retained_languages = missing if table in ('skill', 'skill_pt') else []
        coverage.append({'table': table, 'target_folder': target_folder,
                         'source_languages': languages, 'pc_languages': pc_languages,
                         'missing_source_languages': sorted(set(pc_languages)-set(languages)),
                         'project_translation_languages': project_languages,
                         'pc_retained_languages': retained_languages,
                         'missing_candidate_languages': sorted(set(missing)-set(project_languages)-set(retained_languages))})
        for language in sorted(set(languages) | set(project_languages) | set(retained_languages)):
            relative = f'{table}_{language}.gmd'
            original = P / 'MHWI解包/chunk/common/text' / target_folder / relative
            source = source_root / relative
            old = original.read_bytes()
            offset, before, keys = parse(old)
            if language in project_languages:
                material = table == 'item'
                translation_path, translation_bytes, translated = ((project_path, project_bytes, None)
                    if material else table_sources[table])
                ps = translation_bytes
                translations = project_materials[language] if material else translated[language][table]
                assert set(translations) == {english_keys[i].decode('utf-8') for i in selected}
                assert all(value and value != 'HARDUMMY' and '\0' not in value for value in translations.values())
                reference, ps_keys = before.copy(), english_keys
                for index in selected:
                    text = translations[english_keys[index].decode('utf-8')]
                    reference[index] = text.encode('utf-8')
                    assert re.findall(r'<[^>]+>', text) == re.findall(r'<[^>]+>', english_values[index].decode('utf-8')), (table, language, index)
                origin = 'Project translation; linguistic and layout review pending'
                source_paths = ([str(project_path.relative_to(P)), str(shared_path.relative_to(P))]
                                if material else [str(translation_path.relative_to(P))])
            elif language in retained_languages:
                ps = old
                reference, ps_keys = before.copy(), english_keys
                assert all(before[i] not in (b'', b'HARDUMMY') for i in selected)
                origin = 'PC original retained; no translation or overwrite'
                source_paths = [str(original.relative_to(P))]
            else:
                ps = source.read_bytes()
                _, reference, ps_keys = parse(ps)
                origin = 'PS original'
                source_paths = [str(source.relative_to(P))]
            dependencies = list(interaction_dependencies.get((table, language), []))
            policy_overrides = reward_policy.get(language, {}).get(table, {})
            if policy_overrides:
                reverse = {key.decode('utf-8'): i for i, key in english_keys.items()}
                reference = reference.copy()
                for key, text in policy_overrides.items():
                    index = reverse[key]
                    assert index in selected
                    reference[index] = text.encode('utf-8')
                policy_source = interaction_path.relative_to(P).as_posix()
                if policy_source not in source_paths:
                    source_paths.append(policy_source)
                dependencies.append({'path': policy_source,
                                     'sha256': hashlib.sha256(interaction_bytes).hexdigest(),
                                     'section': f'reward_policy_overrides.{language}.{table}',
                                     'keys': sorted(policy_overrides)})
                origin += '; permanent collaboration reward-policy override'
            assert len(before) == len(reference)
            after = before.copy()
            for index in selected:
                assert index in keys and keys[index] == ps_keys[index] == english_keys[index], (relative, index)
                after[index] = reference[index]
            result = bytearray(old[:offset] + b'\0'.join(after))
            struct.pack_into('<I', result, 32, len(result)-offset)
            new_offset, actual, new_keys = parse(result)
            assert new_offset == offset and new_keys == keys
            assert result[:32] == old[:32] and result[36:offset] == old[36:offset]
            assert all(actual[i] == (reference[i] if i in selected else before[i]) for i in range(len(before)))
            if language in retained_languages:
                assert result == old, 'Retained PC text must remain byte-identical'
            target = OUT.parent / target_folder / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(result)
            deltas.append({'target': f'common/text/{target_folder}/{relative}',
                           'base_sha256': hashlib.sha256(old).hexdigest(),
                           'base_metadata_sha256': hashlib.sha256(old[:offset]).hexdigest(),
                           'source_sha256': hashlib.sha256(ps).hexdigest(),
                           'origin': origin, 'source_paths': source_paths,
                           'translation_dependencies': dependencies,
                           'shared_translation_sha256': hashlib.sha256(shared_bytes).hexdigest() if table == 'item' and language in project_languages else None,
                           'changes': [{'index_hint': i, 'key': keys[i].decode('utf-8'),
                                        'before': before[i].decode('utf-8'),
                                        'after': actual[i].decode('utf-8')}
                                       for i in sorted(selected) if before[i] != actual[i]]})
            receipts.append({'table': table, 'language': language, 'origin': origin,
                             'source_paths': source_paths, 'selected_indices': sorted(selected),
                             'translation_dependencies': dependencies,
                             'shared_translation_sha256': hashlib.sha256(shared_bytes).hexdigest() if table == 'item' and language in project_languages else None,
                             'changed_indices': [i for i in selected if before[i] != actual[i]],
                             'original_sha256': hashlib.sha256(old).hexdigest(),
                             'source_sha256': hashlib.sha256(ps).hexdigest(),
                             'output_sha256': hashlib.sha256(result).hexdigest(),
                             'output': str(target.relative_to(P)),
                             'verification': 'All key CRCs, selected key identities, full string count, unchanged metadata and every unrelated string passed'})
    (P / 'research/horizon-other-text-candidate.json').write_text(json.dumps({
        'status': 'Static PS and project-translation candidates; not installed or runtime verified',
        'limits': ['All16 selected equipment/material/skill/interaction tables have14-language candidates. This is a finite text selection, not proof of all native interactions.',
                   'Four material/bow/Palico/hunter languages use project translations, not official PS text; linguistic and layout review pending.',
                   'Text restoration does not activate the underlying skill, recipe, task or equipment mechanism.'],
        'files': receipts}, ensure_ascii=False, indent=2), encoding='utf-8')
    delta_path = P / 'staging/Horizon-other-module/merge/text-deltas.json'
    delta_path.parent.mkdir(parents=True, exist_ok=True)
    delta_path.write_text(json.dumps({'schema_version': 1, 'module': 'horizon-other',
                                     'policy': 'Merge by stable key; accept only before or already-applied after; preserve all unrelated strings; conflicts require resolution by the final merge chat.',
                                     'files': deltas}, ensure_ascii=False, indent=2), encoding='utf-8')
    coverage_path = P/'research/horizon-other-text-language-coverage.json'
    coverage_report = json.loads(coverage_path.read_text(encoding='utf-8')) if coverage_path.exists() else {}
    coverage_report.update({
        'coverage': coverage, 'complete_all_pc_languages': all(not row['missing_candidate_languages'] for row in coverage)
    })
    coverage_path.write_text(json.dumps(coverage_report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'files': len(receipts), 'selected_strings': sum(len(r['selected_indices']) for r in receipts),
                      'changed_strings': sum(len(r['changed_indices']) for r in receipts), 'validation': 'PASS'}))


if __name__ == '__main__':
    main()
