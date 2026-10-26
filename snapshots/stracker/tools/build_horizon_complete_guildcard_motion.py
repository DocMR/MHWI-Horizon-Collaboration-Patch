"""Append only the retained PS guildcard static pose262 to the PC LMT95."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
from compare_ps_pc_lmt import parse

P = Path(__file__).resolve().parents[1]
REL = Path('hm/common/mot/co00_03/co00_03.lmt')
BASE = P/'staging/Horizon-complete-language-payload-20261021-c5'
OUT = P/'staging/Horizon-complete-presentation-payload-20261021-c6'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if OUT.exists():
        raise ValueError('Preserve frozen presentation candidate')
    ps_path = P/'research/horizon-other-source'/REL
    pc_path = P/'MHWI解包/chunk'/REL
    source, native = ps_path.read_bytes(), pc_path.read_bytes()
    assert sha(source) == '50471490a2dbb4e32186b2858799e71a5e68807f225b565160a1f74aebecec56'
    assert sha(native) == '52391a1377f83665437165b935ac6b0de749be9c2d71c8d249fcf1a21ce51f78'
    assert not (BASE/'nativePC'/REL).exists()
    ps, pc = parse(ps_path), parse(pc_path)
    assert 262 in ps and 262 not in pc
    assert all(ps[index] == row for index, row in pc.items())
    q = lambda offset: struct.unpack_from('<Q', source, offset)[0]
    motion = q(16+262*8)
    tracks, count, frames = q(motion), struct.unpack_from('<I', source, motion+8)[0], struct.unpack_from('<I', source, motion+12)[0]
    assert count == 79 and frames == 2
    header = bytearray(source[motion:motion+96])
    descriptors = source[tracks:tracks+count*48]
    assert len(descriptors) == count*48
    # This exact pose contains static transform values in its descriptors,
    # no keyframe buffers/bounds, and an empty event header. Do not generalize
    # its pointer layout into a general animation converter.
    for n in range(count):
        assert struct.unpack_from('<I', descriptors, n*48+12)[0] == 0
        assert struct.unpack_from('<Q', descriptors, n*48+16)[0] == 0
        assert struct.unpack_from('<Q', descriptors, n*48+40)[0] == 0
    assert source[q(motion+88):q(motion+88)+48] == bytes(48)
    assert header[72:88] == bytes(16)
    output = bytearray(native)
    output.extend(bytes((-len(output)) % 16))
    new_motion = len(output)
    new_tracks = new_motion+96
    new_events = new_tracks+len(descriptors)
    struct.pack_into('<Q', header, 0, new_tracks)
    struct.pack_into('<Q', header, 88, new_events)
    output.extend(header+descriptors+bytes(48))
    slot = 16+262*8
    struct.pack_into('<Q', output, slot, new_motion)
    assert output[:slot] == native[:slot] and output[slot+8:len(native)] == native[slot+8:]
    fixture = P/'research/horizon-complete-guildcard-pose262-fixture.lmt'
    assert not fixture.exists()
    fixture.write_bytes(output)
    actual = parse(fixture)
    assert set(actual) == set(pc)|{262}
    assert all(actual[index] == row for index, row in pc.items())
    assert actual[262] == ps[262]
    assert 269 not in actual and 293 not in actual
    shutil.copytree(BASE, OUT)
    target = OUT/'nativePC'/REL
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(output)
    for f in (BASE/'nativePC').rglob('*'):
        if f.is_file():
            assert f.read_bytes() == (OUT/'nativePC'/f.relative_to(BASE/'nativePC')).read_bytes()
    result = {'status': 'guildcard_pose262_selectively_appended_no_live_acceptance',
              'candidate': OUT.relative_to(P).as_posix(), 'target': REL.as_posix(),
              'source': ps_path.relative_to(P).as_posix(), 'source_sha256': sha(source),
              'pc_sha256': sha(native), 'output_sha256': sha(output),
              'pc_bytes': len(native), 'output_bytes': len(output),
              'existing_file_change': {'offset': slot, 'bytes': 8, 'meaning': 'Previously zero motion262 pointer'},
              'all_existing_pc_bytes_other_than_slot_pointer_preserved': True,
              'existing_motion_semantics_preserved': len(pc), 'added_slot': 262,
              'source_motion_semantics': ps[262], 'unrelated_ps_slots_not_added': [269, 293],
              'resource_files': sum(f.is_file() for f in (OUT/'nativePC').rglob('*')),
              'limits': ['Native viewer resource selection and rendering await grouped live acceptance',
                         'Exact static pose only; no general LMT conversion claim'],
              'game_modified': False}
    (P/'research/horizon-complete-guildcard-motion-20261021.json').write_text(
        json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status', 'existing_motion_semantics_preserved', 'resource_files']}))


if __name__ == '__main__':
    main()
