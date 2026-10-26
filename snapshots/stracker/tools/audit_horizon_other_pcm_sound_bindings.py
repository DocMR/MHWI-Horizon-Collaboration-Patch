"""Join converted media, staged bank bytes and every known Sound owner.

This is file-structure inspection, not native parser execution or playback.
"""
from pathlib import Path
import hashlib
import json
import struct
from convert_horizon_other_atrac9_media import bank_chunks
from merge_horizon_other_audio_objects import sound_objects

P = Path(__file__).resolve().parents[1]
MODULE = P/'staging/Horizon-other-module'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pcm(raw, expected_samples=None):
    assert raw[:4] == b'RIFF' and raw[8:12] == b'WAVE'
    assert struct.unpack_from('<I', raw, 4)[0]+8 == len(raw)
    chunks, offset = {}, 12
    while offset < len(raw):
        tag, size = struct.unpack_from('<4sI', raw, offset)
        assert tag not in chunks and offset+8+size <= len(raw)
        chunks[tag] = raw[offset+8:offset+8+size]
        offset += 8+size+(size & 1)
    assert offset == len(raw)
    assert chunks[b'fmt '] == struct.pack('<HHIIHHHHI', 0xFFFE, 1, 48000, 96000, 2, 16, 0, 0, 0x4101)
    assert len(chunks[b'data']) % 2 == 0
    samples = len(chunks[b'data'])//2
    if expected_samples is not None:
        assert samples == expected_samples
    return samples


def main():
    conversion = json.loads((P/'research/horizon-other-pcm-media-conversion.json').read_text(encoding='utf-8'))['media']
    media = {(r['bank'], r['id']): r for r in conversion}
    assert len(media) == len(conversion) and media
    banks = json.loads((P/'research/horizon-other-pcm-bank-candidates.json').read_text(encoding='utf-8'))['banks']
    owners = {key: [] for key in media}
    bank_rows = []
    for bank in banks:
        name = Path(bank['path']).name
        raw = (MODULE/'resources/nativePC/sound/wwise/Windows'/name).read_bytes()
        assert sha(raw) == bank['sha256']
        parts = bank_chunks(raw)
        seen = set()
        for offset in range(0, len(parts[b'DIDX']), 12):
            identity, start, size = struct.unpack_from('<3I', parts[b'DIDX'], offset)
            assert identity not in seen and start % 16 == 0
            seen.add(identity)
            entry = media[name, identity]
            wem = parts[b'DATA'][start:start+size]
            assert len(wem) == size == entry['size'] and sha(wem) == entry['sha256']
            assert wem == (P/entry['path']).read_bytes()
            pcm(wem, entry['samples'])
        assert seen == {identity for bank_name, identity in media if bank_name == name}
        for sound_id, (_, payload) in sound_objects(parts[b'HIRC']).items() if b'HIRC' in parts else []:
            plugin, = struct.unpack_from('<I', payload, 4)
            if plugin == 0x650002:
                preserved = {r['sound_id']: r for r in bank['preserved_generators']}
                assert sound_id in preserved and sha(payload) == preserved[sound_id]['sha256']
                continue
            medium, size = struct.unpack_from('<2I', payload, 9)
            assert plugin == 0x10001 and payload[8] == 0
            entry = media[name, medium]
            assert size == entry['size']
            owners[name, medium].append(dict(bank=name, sound_id=sound_id, source='own HIRC'))
        bank_rows.append(dict(bank=name, embedded_media=len(seen), has_own_hirc=b'HIRC' in parts))
    shared = json.loads((MODULE/'merge/audio-object-deltas.json').read_text(encoding='utf-8'))['files']
    for bank in shared:
        candidate = (P/'research/horizon-other-pcm-shared-bank-candidate'/bank['target']).read_bytes()
        assert sha(candidate) == bank['candidate_sha256']
        sounds = sound_objects(bank_chunks(candidate)[b'HIRC'])
        for edit in bank['changes']:
            payload = sounds[edit['sound_id']][1]
            assert sha(payload) == edit['after_sha256']
            plugin, = struct.unpack_from('<I', payload, 4)
            identity, size = struct.unpack_from('<2I', payload, 9)
            assert plugin == 0x10001 and payload[8] == 0 and identity == edit['media_id']
            key = edit['source_media_bank'], identity
            assert size == media[key]['size']
            owners[key].append(dict(bank=bank['target'], sound_id=edit['sound_id'], source='shared stable-ID delta'))
    missing = [dict(bank=key[0], media_id=key[1]) for key, values in owners.items() if not values]
    assert not missing, missing
    body_path = MODULE/'resources/nativePC/sound/wwise/Windows/pl068_0010_epvsp.nbnk'
    body_data = body_path.read_bytes()
    body_receipt = json.loads((P/'research/body068-native-pcm-candidate-report.json').read_text(encoding='utf-8'))
    assert sha(body_data) == body_receipt['output_sha256']
    body = bank_chunks(body_data)
    body_media = {}
    for offset in range(0, len(body[b'DIDX']), 12):
        identity, start, size = struct.unpack_from('<3I', body[b'DIDX'], offset)
        raw = body[b'DATA'][start:start+size]
        assert len(raw) == size
        body_media[identity] = dict(size=size, samples=pcm(raw))
    assert len(body_media) == 2
    body_sounds = sound_objects(body[b'HIRC'])
    assert len(body_sounds) == 2
    for sound_id, (_, payload) in body_sounds.items():
        identity, size = struct.unpack_from('<2I', payload, 9)
        assert struct.unpack_from('<I', payload, 4)[0] == 0x10001 and payload[8] == 0
        assert body_media[identity]['size'] == size
    from convert_horizon_other_npc_streams import package_entries
    streams = json.loads((P/'research/horizon-other-npc-stream-conversion.json').read_text(encoding='utf-8'))
    stream_media = {(r['bank'], r['id']): r for r in streams['media']}
    assert len(stream_media) == len(streams['media'])
    stream_seen, stream_refs = set(), []
    for row in streams['files']:
        data = (MODULE/'resources/nativePC'/row['path']).read_bytes()
        assert sha(data) == row['sha256'] and len(data) == row['size']
        name = Path(row['path']).name
        if name.endswith('.npck'):
            _, entries = package_entries(data)
            for _, identity, offset, size in entries:
                key = name, identity
                entry = stream_media[key]
                raw = data[offset:offset+size]
                assert len(raw) == entry['size'] and sha(raw) == entry['sha256']
                pcm(raw, entry['samples']); stream_seen.add(key)
        else:
            sounds = sound_objects(bank_chunks(data)[b'HIRC'])
            source = (P/'research/horizon-other-effect-audio-source/sound/wwise/PS4'/name).read_bytes()
            original = sound_objects(bank_chunks(source)[b'HIRC'])
            assert set(sounds) == set(original)
            for identity, (_, payload) in sounds.items():
                before = original[identity][1]
                medium = struct.unpack_from('<I', payload, 9)[0]
                assert struct.unpack_from('<I', payload, 4)[0] == 0x10001 and payload[8] == 2
                assert payload[:4] == before[:4] and payload[8:] == before[8:]
                key = name.replace('.nbnk', '.npck'), medium
                assert key in stream_media
                stream_refs.append(key)
    assert stream_seen == set(stream_media) == set(stream_refs)
    report = dict(schema_version=1, module='horizon-other',
        status='Static converted-media to Sound-source closure; runtime acceptance/event triggering incomplete',
        converted_banks=bank_rows, converted_media=len(media), body068_media=body_media,
        streamed_media=len(stream_media), streamed_sound_references=len(stream_refs),
        shared_banks=len(shared), sound_references=sum(map(len, owners.values())), missing=missing,
        media=[dict(bank=key[0], media_id=key[1], size=media[key]['size'], samples=media[key]['samples'],
                    sha256=media[key]['sha256'], owners=values) for key, values in sorted(owners.items())],
        limits=['DIDX payload, RIFF structure and Sound codec/stream/media-size references only; not native decoder execution.',
                'Shared Sound deltas must be composed at final merge; shared candidate whole banks are not installed.',
                'Actions, containers, switches, WWCT/WWEV, bank readiness and complete event reachability remain separate.',
                'No playback, native code execution, game launch or game-directory writes.'])
    for path in (P/'research/horizon-other-pcm-sound-bindings.json', MODULE/'merge/pcm-sound-bindings.json'):
        path.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(media=len(media)+len(body_media)+len(stream_media), banks=len(banks)+3,
        sound_references=sum(map(len, owners.values()))+len(body_sounds)+len(stream_refs),
        shared_banks=len(shared), missing=len(missing))))


if __name__ == '__main__':
    main()
