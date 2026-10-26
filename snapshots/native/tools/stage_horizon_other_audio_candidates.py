"""Assemble isolated audio candidates and merge inputs, never install them."""
from pathlib import Path
import hashlib
import json
import struct
from convert_horizon_other_atrac9_media import bank_chunks
from merge_horizon_other_audio_objects import sound_objects

P = Path(__file__).resolve().parents[1]
MODULE = P/'staging/Horizon-other-module'


def put(relative, data):
    target = (MODULE/'resources/nativePC'/relative).resolve()
    assert target.is_relative_to((MODULE/'resources/nativePC').resolve())
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists():
        assert target.read_bytes() == data
    else:
        target.write_bytes(data)
    return {'path': relative, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    files = []
    banks = json.loads((P/'research/horizon-other-pcm-bank-candidates.json').read_text(encoding='utf-8'))
    headers = json.loads((P/'research/horizon-other-native-bank-header.json').read_text(encoding='utf-8'))
    assert len(headers['banks']) == len(banks['banks']) and banks['banks']
    for row in banks['banks']:
        data = (P/row['path']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == row['sha256']
        assert any(h['path'] == row['path'] and h['version'] == 120 and not h['native_low_word_nonzero']
                   for h in headers['banks'])
        files.append(put('sound/wwise/Windows/'+Path(row['path']).name, data))
    # Reuse only the exact source-backed Shield-Weaver audio result from the
    # shared project; the firing/charge implementation remains separate.
    receipt = json.loads((P/'research/ps-body068-approved-extraction.json').read_text(encoding='utf-8'))
    approved = P/'research/ps-body068-approved'
    for row in receipt['files']:
        data = (approved/row['path']).read_bytes()
        assert len(data) == row['size'] and hashlib.sha256(data).hexdigest() == row['sha256']
        files.append(put(row['path'], data))
    assert (approved/'sound/pl/snd_equip_body068_snd_bk.wwbk').read_bytes() == (
        b'WWBK'+struct.pack('<III', 4, 1, 0)+b'pl068_0010_epvsp\0\0\0')
    common = (P/'MHWI解包/chunk/sound/pl/pl_cmn_snd_ct.wwct').read_bytes()
    assert b'sound\\pl\\snd_equip_body068_snd_ev\0' in common
    assert (P/'MHWI解包/chunk/sound/pl/wwspl/pl_speker_set.wwspl').is_file()
    body = json.loads((P/'research/body068-native-pcm-candidate-report.json').read_text(encoding='utf-8'))
    original = (approved/'sound/wwise/PS4/pl068_0010_epvsp.nbnk').read_bytes()
    data = (P/'research/body068-native-pcm-candidate/pl068_0010_epvsp.nbnk').read_bytes()
    assert hashlib.sha256(original).hexdigest() == body['source_sha256']
    assert hashlib.sha256(data).hexdigest() == body['output_sha256']
    source_parts, parts = bank_chunks(original), bank_chunks(data)
    assert source_parts[b'BKHD'] == parts[b'BKHD']
    version, _, _, flags, project = struct.unpack_from('<5I', parts[b'BKHD'])
    assert version == 120 and flags & 0xffff == 0 and project == 1114
    source_objects, converted = sound_objects(source_parts[b'HIRC']), sound_objects(parts[b'HIRC'])
    allowed = set(range(4, 8)) | set(range(13, 17))
    assert set(source_objects) == set(converted)
    for identity, (_, payload) in converted.items():
        before = source_objects[identity][1]
        assert len(before) == len(payload)
        assert all(a == b or i in allowed for i, (a, b) in enumerate(zip(before, payload)))
        assert struct.unpack_from('<I', payload, 4)[0] == 0x10001
    files.append(put('sound/wwise/Windows/pl068_0010_epvsp.nbnk', data))
    streams = json.loads((P/'research/horizon-other-npc-stream-conversion.json').read_text(encoding='utf-8'))
    for row in streams['files']:
        data = (MODULE/'resources/nativePC'/row['path']).read_bytes()
        assert len(data) == row['size'] and hashlib.sha256(data).hexdigest() == row['sha256']
        files.append(put(row['path'], data))
    report = {'schema_version': 1, 'module': 'horizon-other', 'files': files,
              'npc_stream_conversion': streams,
              'shared_bank_edits': 'merge/audio-object-deltas.json',
              'reused_body068_source_sha256': body['source_sha256'],
              'state': 'Static module candidates, not installed; full native acceptance and event triggering unverified',
              'install_allowed': False,
              'limits': ['Do not install audio banks without composing the three shared Sound-object deltas.',
                         'Source media bytes converted, original IDs/actions preserved; dynamic behavior is not validated.',
                         'PCM parser coverage gap and complete event/container ownership remain open.',
                         'No game process, target native code or audio playback executed.']}
    for output in (P/'research/horizon-other-audio-staging.json', MODULE/'merge/audio-candidates.json'):
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'staged_files': len(files), 'pcm_banks': len(banks['banks'])+3,
                      'stream_packages': 2, 'body068_sound_objects': len(converted)}))


if __name__ == '__main__':
    main()
