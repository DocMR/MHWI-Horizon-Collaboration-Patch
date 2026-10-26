"""Build four missing quest languages with PC terms and project prose.

Writes only workspace candidates; never installs or executes game code.
"""
from pathlib import Path
import hashlib
import json
import struct
from build_horizon_other_text_candidate import parse

P = Path(__file__).resolve().parents[1]
QUESTS = (67101, 67102, 67607, 67802, 67803, 67804)
LANGUAGES = ('ara', 'pol', 'ptB', 'rus')
MODULE = P/'staging/Horizon-other-module'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    project_path = P/'research/horizon-other-quest-translations.json'
    project_bytes = project_path.read_bytes()
    project = json.loads(project_bytes)
    shared_path = P/project['shared_source']
    shared_bytes = shared_path.read_bytes()
    shared = json.loads(shared_bytes)['languages']
    translations = {}
    for language in LANGUAGES:
        translations[language] = project['languages'][language].copy()
        for quest in project['shared_quests']:
            assert str(quest) not in translations[language]
            translations[language][str(quest)] = shared[language][str(quest)]
        assert set(translations[language]) == set(map(str, QUESTS))

    sources, terms = {}, set()
    for quest in QUESTS:
        source = P/f'research/horizon-other-quest-source/common/text/quest/q{quest}_eng.gmd'
        data = source.read_bytes()
        offset, values, keys = parse(data)
        assert len(values) == 12 and len(keys) == 11
        for index in (1, 2, 6, 7, 8, 9):
            if not keys[index].startswith(b'DUMMY'):
                terms.add(values[index].decode('utf-8'))
        sources[quest] = (data, offset, values, keys)

    # Match whole original English entries, then retrieve each localized value
    # by the SAME stable key, not an assumed cross-language numeric index.
    pc_root = P/'MHWI解包/chunk/common/text/quest'
    adaptations = project['objective_adaptations']
    lookup_terms = terms-set(adaptations)
    lookup_terms.update(row['source'] for row in adaptations.values())
    localized = {}
    for source in sorted(pc_root.glob('*_eng.gmd')):
        if not all((pc_root/(source.name[:-7]+language+'.gmd')).is_file() for language in LANGUAGES):
            continue
        data = source.read_bytes()
        _, values, keys = parse(data)
        matches = [(i, values[i].decode('utf-8')) for i in keys
                   if values[i].decode('utf-8') in lookup_terms-localized.keys()]
        if not matches:
            continue
        tables = {}
        for language in LANGUAGES:
            path = pc_root/(source.name[:-7]+language+'.gmd')
            raw = path.read_bytes()
            _, lv, lk = parse(raw)
            reverse = {key: i for i, key in lk.items()}
            tables[language] = (path, raw, lv, reverse)
        for index, english in matches:
            if english in localized:
                continue
            values_by_language = {}
            provenance = {}
            for language, (path, raw, lv, reverse) in tables.items():
                key = keys[index]
                assert key in reverse
                text = lv[reverse[key]].decode('utf-8')
                assert text not in ('', 'HARDUMMY', 'Unavailable')
                values_by_language[language] = text
                provenance[language] = dict(path=path.relative_to(P).as_posix(), sha256=sha(raw), key=key.decode('utf-8'))
            localized[english] = dict(values=values_by_language, provenance=provenance)
        if set(localized) == lookup_terms:
            break
    assert set(localized) == lookup_terms, sorted(lookup_terms-set(localized))
    for english, adaptation in adaptations.items():
        native = localized[adaptation['source']]
        if 'replace_number' in adaptation:
            old, new = adaptation['replace_number']
            assert all(text.count(old) == 1 for text in native['values'].values())
            values = {language: text.replace(old, new) for language, text in native['values'].items()}
        else:
            values = adaptation['translations']
            assert set(values) == set(LANGUAGES)
        localized[english] = dict(values=values, provenance=native['provenance'],
            origin='Project objective count adaptation of native PC terminology', source_english=adaptation['source'])

    receipts = []
    for quest in QUESTS:
        data, offset, before, keys = sources[quest]
        for language in LANGUAGES:
            values = before.copy()
            prose = translations[language][str(quest)]
            assert len(prose) == 3
            for index, text in zip((0, 3, 4), prose):
                assert text and '\0' not in text
                values[index] = text.encode('utf-8')
            for index in (1, 2, 6, 7, 8, 9):
                if not keys[index].startswith(b'DUMMY'):
                    values[index] = localized[before[index].decode('utf-8')]['values'][language].encode('utf-8')
            for index, key in keys.items():
                if key.startswith(b'DUMMY'):
                    assert values[index] == before[index] == b'Unavailable'
            result = bytearray(data[:offset]+b'\0'.join(values))
            struct.pack_into('<I', result, 32, len(result)-offset)
            new_offset, actual, new_keys = parse(result)
            assert new_offset == offset and actual == values and new_keys == keys
            assert result[:32] == data[:32] and result[36:offset] == data[36:offset]
            path = f'common/text/quest/q{quest}_{language}.gmd'
            assert not (P/'MHWI解包/chunk'/path).exists(), 'Existing PC quest requires stable-key merge instead'
            # Shared quests must remain byte-identical to the other chat's
            # existing candidate, when present, so a final merge deduplicates.
            shared_candidate = P/'staging/Stormslinger-material-quest-localization-candidate/nativePC'/path
            if shared_candidate.is_file() and quest in project['shared_quests']:
                assert shared_candidate.read_bytes() == result, str(shared_candidate)
            target = MODULE/'resources/nativePC'/path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(result)
            receipts.append(dict(path=path, quest_id=quest, language=language,
                size=len(result), sha256=sha(result), base_action='add',
                template_sha256=sha(data), project_source_sha256=sha(project_bytes),
                shared_source_sha256=sha(shared_bytes) if quest in project['shared_quests'] else None,
                origin='PC original failure/creatures/available targets; project title/client/request and two target-count adaptations',
                changed_keys=[keys[i].decode('utf-8') for i in keys if before[i] != values[i]],
                shared_candidate_identical=shared_candidate.is_file() if quest in project['shared_quests'] else None))
    report = dict(schema_version=1, module='horizon-other',
        status='Static project quest text candidates; linguistic/layout review pending',
        policy='Deduplicate identical shared quest path/hash; register quest once. New GMDs retain original English stable keys and embedded quest ID.',
        source_paths=[project_path.relative_to(P).as_posix(), shared_path.relative_to(P).as_posix()],
        resources=receipts, native_terms=localized,
        limits=['Four languages use project title/client/prose, not official PS localization.',
                'Hunt2 Deviljho and Slay8 Barnos adapt original PC target terminology/counts; not verbatim original PC quest targets.',
                'Coverage is six quests and four added languages; no game layout or RTL validation.',
                'Quest resources alone do not activate admission, reward or persistence mechanisms.'])
    for path in (P/'research/horizon-other-quest-project-text.json', MODULE/'merge/quest-project-text.json'):
        path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(quests=len(QUESTS), languages=len(LANGUAGES), added_files=len(receipts),
                         native_terms=len(localized), shared_identical=sum(r['shared_candidate_identical'] is True for r in receipts))))


if __name__ == '__main__':
    main()
