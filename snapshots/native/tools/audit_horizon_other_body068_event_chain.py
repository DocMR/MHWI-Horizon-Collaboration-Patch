"""Check Shield-Weaver serialized EPVSP/container/event/HIRC identity edges."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage
from pc_unwind_groups import decode_group
from stage_horizon_other_sound_params import parse as parse_params
from convert_horizon_other_atrac9_media import bank_chunks

P = Path(__file__).resolve().parents[1]
ROOT = P/'staging/Horizon-other-module/resources/nativePC'
BASE = P/'MHWI解包/chunk'


def cstring(data, cursor):
    end = data.index(b'\0', cursor)
    assert end-cursor < 80
    return data[cursor:end].decode('ascii').replace('\\', '/'), end+1


def parse_events(data):
    magic, version, count, reserved = struct.unpack_from('<4I', data)
    assert magic == 0x56455757 and version == 19 and reserved == 0
    cursor, paths = 16, []
    for _ in range(4):
        path, cursor = cstring(data, cursor)
        paths.append(path)
    records = []
    for _ in range(count):
        assert cursor+128 <= len(data)
        start = cursor
        raw = data[cursor:cursor+128]
        cursor += 128
        child, cursor = cstring(data, cursor)
        records.append({'offset': start, 'local_id_u16': struct.unpack_from('<H', raw)[0],
                        'wwise_event_id': struct.unpack_from('<I', raw, 4)[0],
                        'named_hash': struct.unpack_from('<I', raw, 8)[0],
                        'child': child, 'record_sha256': hashlib.sha256(raw).hexdigest()})
    assert cursor == len(data)
    return paths, records


def main():
    im = CodeImage()
    _, event_reader = decode_group(im, 0x24BDE30)
    by = {x.address: x for x in event_reader}
    for address, operand in {0x24BDE72: 'dword ptr [rsp + 0x40], 0x56455757',
                             0x24BDE80: 'dword ptr [rsp + 0x44], 0x13',
                             0x24BE03B: 'r8d, 0x80', 0x24BE041: '0x21757e0',
                             0x24BE055: '0x2175a10'}.items():
        assert by[address].op_str == operand
    _, container_reader = decode_group(im, 0x22AECC0)
    cb = {x.address: x for x in container_reader}
    for address, operand in {0x22AECFF: 'dword ptr [rsp + 0x68], 0x54435757',
                             0x22AED0D: 'dword ptr [rsp + 0x6c], 6',
                             0x22AED54: 'ebx, dword ptr [rsp + 0x78]',
                             0x22AEDC8: '0x2175990', 0x22AEDDF: '0x2175a10'}.items():
        assert cb[address].op_str == operand
    container_path = 'sound/pl/snd_equip_body068_epvsp_snd_ct.wwct'
    container = (ROOT/container_path).read_bytes()
    header = struct.unpack_from('<10I', container)
    assert header == (0x54435757, 6, 0, 0, 1, 0, 0, 0, 0, 0)
    category = struct.unpack_from('<I', container, 40)[0]
    event_path, end = cstring(container, 44)
    assert end == len(container)
    event_path += '.wwev'
    paths, records = parse_events((ROOT/event_path).read_bytes())
    assert paths[0] == 'sound/pl/snd_equip_body068_snd_bk' and paths[1:3] == ['', '']
    assert (BASE/(paths[3]+'.wwspl')).is_file()
    bank = bank_chunks((ROOT/'sound/wwise/Windows/pl068_0010_epvsp.nbnk').read_bytes())
    hirc = bank[b'HIRC']
    cursor, objects = 4, {}
    for _ in range(struct.unpack_from('<I', hirc)[0]):
        kind, size = struct.unpack_from('<BI', hirc, cursor)
        start = cursor+5
        assert start+size <= len(hirc)
        payload = hirc[start:start+size]
        identity = struct.unpack_from('<I', payload)[0]
        assert identity not in objects
        objects[identity] = (kind, payload)
        cursor = start+size
    assert cursor == len(hirc)
    for row in records:
        kind, payload = objects[row['wwise_event_id']]
        assert kind == 4
        count = struct.unpack_from('<I', payload, 4)[0]
        assert len(payload) == 8+4*count
        actions = list(struct.unpack_from('<'+'I'*count, payload, 8))
        targets = []
        for action in actions:
            kind, payload = objects[action]
            assert kind == 3 and len(payload) >= 10
            target = struct.unpack_from('<I', payload, 6)[0]
            assert target in objects
            targets.append({'action': action, 'action_type_u16': struct.unpack_from('<H', payload, 4)[0],
                            'target': target, 'target_hirc_type': objects[target][0]})
        row['actions'] = targets
    param_path = 'pl/f_equip/pl068_0010/body/sound/f_body068.epvsp'
    params = parse_params((ROOT/param_path).read_bytes())
    assert params['container'] == container_path
    selected = [row[1] for row in params['array_a'] if row[1] >= 0]
    assert selected == [60301]
    assert all(any(row['local_id_u16'] == identity for row in records) for identity in selected)
    report = {'exe_sha256': im.meta['exe_sha256'], 'parameter': param_path,
              'container': container_path, 'category': category, 'event_table': event_path,
              'parameter_nonnegative_local_ids': selected, 'events': records,
              'state': 'Serialized identity edges resolve from parameter to local event, HIRC Event, Action and target container.',
              'limits': ['Does not prove when native equipment mechanics request local60301.',
                         'Action targets are containers; complete child selection and PCM acceptance remain pending.',
                         'No playback, simulation, game or target-code execution.']}
    (P/'research/horizon-other-body068-event-chain.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'local_parameter_id': selected, 'event_records': len(records),
                      'hirc_event_actions': sum(len(r['actions']) for r in records), 'unresolved_edges': 0}))


if __name__ == '__main__':
    main()
