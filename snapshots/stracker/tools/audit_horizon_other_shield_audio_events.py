"""Inspect shield bank event edges as asset data; never execute game/audio code."""
from pathlib import Path
import hashlib
import json
import sys
from audit_horizon_other_body068_event_chain import parse_events, cstring
from stage_horizon_other_sound_params import parse as parse_params

P = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(P/'.runtime/wwiser-source'))
from wwiser.parser.wparser import Parser


def descendants(node):
    for child in node.get_children() or []:
        yield child
        yield from descendants(child)


def main():
    path = P/'staging/Horizon-other-module/resources/nativePC/sound/wwise/Windows/pl068_0010_epvsp.nbnk'
    parser = Parser()
    assert parser.parse_bank(str(path))
    banks = parser.get_banks()
    assert len(banks) == 1
    bank = banks[0]
    assert bank.get_error_count() == bank.get_skip_count() == 0
    objects = {}
    for node in descendants(bank):
        attrs = node.get_attrs()
        if node.get_nodename() != 'object' or 'index' not in attrs:
            continue
        direct = [c.get_attrs() for c in node.get_children() or []]
        ids = [a['value'] for a in direct if a.get('name') == 'ulID']
        if not ids:
            continue
        assert len(ids) == 1 and ids[0] not in objects
        fields = {}
        for child in descendants(node):
            a = child.get_attrs()
            if child.get_nodename() == 'field':
                fields.setdefault(a.get('name'), []).append(a.get('value'))
        objects[ids[0]] = {'class': attrs['name'], 'fields': fields}

    def one(identity, field):
        values = objects[identity]['fields'][field]
        assert len(values) == 1
        return values[0]

    expected = [
        (1133299981, 717938734, 'CAkActionStop', 631390213, 673112694, 1064037692),
        (3261967559, 723575840, 'CAkActionPlay', 631390213, 673112694, 1064037692),
        (3849980617, 375998866, 'CAkActionPlay', 1002905323, 342912725, 998404204),
    ]
    rows = []
    for event, action, action_class, container, sound, media in expected:
        assert objects[event]['class'] == 'CAkEvent'
        assert one(event, 'ulActionListSize') == 1
        assert one(event, 'ulActionID') == action
        assert objects[action]['class'] == action_class
        assert one(action, 'idExt') == container
        assert objects[container]['class'] == 'CAkRanSeqCntr'
        assert one(container, 'ulNumChilds') == 1
        assert one(container, 'ulChildID') == one(container, 'ulPlayID') == sound
        assert objects[sound]['class'] == 'CAkSound'
        assert one(sound, 'sourceID') == media
        if action_class == 'CAkActionPlay':
            assert one(action, 'fileID') == 1312604792
        rows.append(dict(event=event, action=action, action_class=action_class,
                         action_type=one(action, 'ulActionType'), container=container,
                         sound=sound, media=media))
    root = P/'staging/Horizon-other-module/resources/nativePC'
    parameter = root/'pl/f_equip/pl068_0010/body/sound/f_body068.epvsp'
    params = parse_params(parameter.read_bytes())
    container_path = root/params['container']
    container_raw = container_path.read_bytes()
    import struct
    assert struct.unpack_from('<10I', container_raw) == (0x54435757, 6, 0, 0, 1, 0, 0, 0, 0, 0)
    event_name, end = cstring(container_raw, 44)
    assert end == len(container_raw)
    event_path = root/(event_name+'.wwev')
    paths, events = parse_events(event_path.read_bytes())
    assert paths[0] == 'sound/pl/snd_equip_body068_snd_bk'
    assert len(events) == 3
    local_to_event = {60301: 3849980617, 60302: 3261967559, 60303: 1133299981}
    assert {r['local_id_u16']: r['wwise_event_id'] for r in events} == local_to_event
    by_event = {r['event']: r for r in rows}
    for record in events:
        assert record['child'] == ''
        by_event[record['wwise_event_id']]['local_event_id'] = record['local_id_u16']
    selected = [r[1] for r in params['array_a'] if r[1] >= 0]
    assert selected == [60301]
    chain_files = [parameter, container_path, event_path]
    report = dict(bank=str(path.relative_to(P)), sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                  parser_receipt='research/wwiser-source-download-receipt.json',
                  parser_errors=0, parser_skips=0, event_chains=rows,
                  serialized_resource_chain=[{'path': str(f.relative_to(P)),
                    'sha256': hashlib.sha256(f.read_bytes()).hexdigest()} for f in chain_files],
                  parameter_selected_local_ids=selected,
                  scope='Serialized EPVSP/WWCT/WWEV plus bank internal explicit event/action/container/source edges.',
                  remaining=['Native mechanics request timing and runtime resource ownership',
                             'External buses, attenuation, state and RTPC dependencies',
                             'Native bank loading and PCM decoding acceptance; no playback performed'])
    out = P/'research/horizon-other-shield-audio-events.json'
    out.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'events': len(rows), 'play_events': 2, 'stop_events': 1, 'parser_errors': 0}))


if __name__ == '__main__':
    main()
