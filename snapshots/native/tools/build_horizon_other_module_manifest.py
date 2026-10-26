"""Record staged file identities for the final merge chat; not an installer."""
from pathlib import Path
import hashlib
import json

P = Path(__file__).resolve().parents[1]
MODULE = P/'staging/Horizon-other-module'


def main():
    files = []
    for source in sorted((MODULE/'resources/nativePC').rglob('*')):
        if not source.is_file():
            continue
        relative = source.relative_to(MODULE/'resources/nativePC')
        data = source.read_bytes()
        base = P/'MHWI解包/chunk'/relative
        files.append({'path': relative.as_posix(), 'size': len(data),
                      'sha256': hashlib.sha256(data).hexdigest(),
                      'base_action': 'replace' if base.is_file() else 'add',
                      'base_sha256': hashlib.sha256(base.read_bytes()).hexdigest() if base.is_file() else None})
    lifecycle=json.loads((P/'research/horizon-other-mapping-lifecycle.json').read_text(encoding='utf-8'))
    persistence=lifecycle['character_streams']
    assert persistence['fits_original_payload'] and persistence['appended_index_range']==[352,357]
    report = {'schema_version': 1, 'module': 'horizon-other', 'content_complete': False,
              'status': 'Work in progress; source-backed resources and partial static library, not a finished migration',
              'current_status_document': '当前状态.md',
              'build_entry': 'tools/build_horizon_other_module.py (project root)',
              'resource_root': 'resources/nativePC', 'resources': files,
              'merge_inputs': ['merge/text-deltas.json', 'merge/quest-resources.json',
                               'merge/quest-text-deltas.json', 'merge/quest-project-text.json', 'merge/weapon-tree-edits.json',
                               'merge/equipment-textures.json', 'merge/model-materials.json',
                               'merge/physics-candidates.json', 'merge/equip-controls.json',
                               'merge/quest-bridge-interface.json', 'merge/quest-save-map-candidate.json',
                               'merge/catalog-build.json', 'merge/effect-audio-references.json', 'merge/effects.json',
                               'merge/audio-object-deltas.json', 'merge/pcm-sound-bindings.json', 'merge/effect-secondary-refs.json',
                               'merge/sound-params.json', 'merge/audio-candidates.json',
                               'merge/equipment-table-baselines.json', 'merge/interaction-table-baselines.json', 'merge/recipe-native-gates.json',
                               'merge/armor-exclusions.json', 'merge/palico-exclusion-edits.json',
                               'merge/palico-native-fields.json', 'merge/native-clone-specs.json',
                               'merge/quest-classification.json', 'merge/shield-shared-core.json', 'merge/quest-open-requirements.json', 'merge/native-rank-readers.json', 'merge/quest-open-maintenance.json'],
              'dynamic_validation': {'performed': False, 'owner': 'final merge chat', 'required_in_this_chat': False},
              'quest_persistence_status': persistence,
              'quest_policy': {'owned_ids': [67101,67102,67607,67802,67803,67804],
                  'menu': 'optional', 'availability': 'permanent after installation',
                  'preserve': ['native rank', 'prerequisites', 'acceptance', 'results', 'rewards', 'first-clear persistence']},
              'guildcard_status': {'owned_title_ids': list(range(782,787))+list(range(1500,1505)),
                  'owned_pose_ids': [13], 'native_admission_adapter': 'implemented; statically compiled and linked only',
                  'permanent_optional_condition_adapter': 'implemented: exact owned GCOD pointers, active mapping and native registered quest resources; statically compiled only',
                  'native_gui_update': {'virtual_slot': 'uGUIGuildCard+0x2d0 -> 0x109fc90', 'setup_call': '0x109fec3 -> 0x60e6d0', 'update_call': '0x60e6f9 -> 0xee3400(filter=-1)', 'condition_and_writers': ['0xee345d -> 0xee27a0', '0xee3477 -> 0x1368400', '0xee35b0 -> 0xee27a0', '0xee35ca -> 0x13682e0']},
                  'native_character_streams': {'title_open_and_new': ['0x16dbe0/256', '0x16dce0/256'], 'pose_open_and_new': ['0x16de68/16', '0x16de80/16'], 'stream_pair': ['0x1361d90/virtual70', '0x136bef0/virtual60'], 'modified': False},
                  'unlock_notification_persistence': 'native GUI update, original open/new writers and matching character stream fields statically connected; production bootstrap and actual persistence acceptance remain pending'},
              'remaining_implementation': ['Production early loader and shared transaction publication with the actual native lifetime boundary',
                  'Six permanent optional quests: actual menu/acceptance integration, native prerequisites and first-clear persistence',
                  'Owned guildcard titles/pose: complete production invocation and original unlock/notification/persistence integration',
                  'Complete equipment menu/creation/upgrade native interaction',
                  'Model/material/skeleton/animation/effect/audio compatibility and dependency conversion',
                  'Shield-Weaver and other equipment-specific native mechanics',
                  'Material/series/layered/delivery text and all target PC languages',
                  'Complete module bootstrap, packaging and merge instructions'],
              'merge_rule': 'Keep identical path/hash once; apply stable-key text deltas; compose native edits against one original function; resolve differing path hashes explicitly.'}
    (MODULE/'module-manifest.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'resource_files': len(files), 'resource_bytes': sum(r['size'] for r in files),
                      'add': sum(r['base_action']=='add' for r in files),
                      'replace': sum(r['base_action']=='replace' for r in files), 'content_complete': False}))


if __name__ == '__main__':
    main()
