"""Compare adapter event name CRC values with serialized WWEV records."""
from pathlib import Path
import hashlib
import json
import re
import zlib
from audit_horizon_other_body068_event_chain import parse_events

P = Path(__file__).resolve().parents[1]


def main():
    source = P/'runtime/src/stormbolt_power_cell_sound.h'
    event = P/'staging/Horizon-other-module/resources/nativePC/sound/pl/snd_equip_body068_snd_ev.wwev'
    text = source.read_text(encoding='utf-8')
    names = re.findall(r'return request\(container,-1,"([A-Z0-9_]+)",&parameters\);', text)
    assert names == ['PLAY_PL_BODY_068_DMG_R_LP', 'STOP_PL_BODY_068_DMG_R_LP']
    _, records = parse_events(event.read_bytes())
    matches = []
    for name, local, expected_event in zip(names, (60302, 60303), (3261967559, 1133299981)):
        # File name CRC convention: reflected CRC32 initial FFFFFFFF, no final XOR.
        value = zlib.crc32(name.encode('ascii')) ^ 0xffffffff
        found = [r for r in records if r['named_hash'] == value]
        row, = found
        assert row['local_id_u16'] == local and row['wwise_event_id'] == expected_event
        matches.append(dict(name=name, crc32_no_final_xor=value, local_id=local,
                            wwise_event_id=expected_event, record_offset=row['offset']))
    report = dict(files=[dict(path=str(f.relative_to(P)), sha256=hashlib.sha256(f.read_bytes()).hexdigest())
                         for f in (source, event)], matches=matches,
                  category=-1,
                  scope='Source adapter literal names match the exact serialized named records.',
                  limits=['Not a new proof of native dispatcher CRC implementation or category search semantics.',
                          'Source linkage does not prove production activation or resource lifetime.',
                          'No native mechanism execution, simulation or playback.'])
    (P/'research/horizon-other-shield-named-events.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(named_events=len(matches), local_ids=[r['local_id'] for r in matches])))


if __name__ == '__main__':
    main()
