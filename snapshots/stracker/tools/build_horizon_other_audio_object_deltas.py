"""Certify PS/PC Sound identity and emit composable PCM source-field edits."""
import hashlib
import json
import struct
from pathlib import Path
from convert_horizon_other_atrac9_media import bank_chunks
from merge_horizon_other_audio_objects import sound_objects, merge

P = Path(__file__).resolve().parents[1]


def main():
    search = json.loads((P/'research/horizon-other-external-sound-object-candidates.json').read_text(encoding='utf-8'))
    assert not search['unresolved_media_ids'] and not search['skipped']
    conversion = json.loads((P/'research/horizon-other-pcm-media-conversion.json').read_text(encoding='utf-8'))
    media = {}
    source_sizes = {}
    layout = json.loads((P/'research/horizon-other-audio-bank-layout.json').read_text(encoding='utf-8'))
    for bank in layout['banks']:
        for row in bank['media']:
            source_sizes[(Path(bank['path']).name, row['id'])] = row['size']
    for row in conversion['media']:
        media.setdefault(row['id'], []).append(row)
    files = []
    for relative in sorted({r['path'] for r in search['matches']}):
        pc_data = (P/'MHWI解包/chunk'/relative).read_bytes()
        source = P/'research/horizon-other-effect-audio-source'/relative.replace('/Windows/', '/PS4/')
        ps_data = source.read_bytes()
        pc = sound_objects(bank_chunks(pc_data)[b'HIRC'])
        ps = sound_objects(bank_chunks(ps_data)[b'HIRC'])
        changes = []
        for candidate in [r for r in search['matches'] if r['path'] == relative]:
            identity = candidate['sound_id']
            before, source_payload = pc[identity][1], ps[identity][1]
            medium = candidate['media_id']
            assert len(media[medium]) == 1
            converted = media[medium][0]
            assert len(before) == len(source_payload)
            allowed = set(range(4, 8)) | set(range(13, 17))
            assert all(a == b or i in allowed for i, (a, b) in enumerate(zip(before, source_payload)))
            assert struct.unpack_from('<I', source_payload, 4)[0] == 0xc0001
            assert struct.unpack_from('<I', before, 4)[0] == 0x40001
            assert before[8] == source_payload[8] == 0
            assert struct.unpack_from('<I', source_payload, 13)[0] == source_sizes[(converted['bank'], medium)]
            after = bytearray(before)
            struct.pack_into('<I', after, 4, 0x10001)
            struct.pack_into('<I', after, 13, converted['size'])
            changes.append({'sound_id': identity, 'media_id': medium, 'source_media_bank': converted['bank'],
                            'before_sha256': hashlib.sha256(before).hexdigest(),
                            'after_sha256': hashlib.sha256(after).hexdigest(),
                            'ps_source_object_sha256': hashlib.sha256(source_payload).hexdigest(),
                            'edits': [{'offset': offset, 'before': before[offset:offset+4].hex(),
                                       'after': after[offset:offset+4].hex()} for offset in (4, 13)]})
        result = merge(pc_data, changes)
        assert merge(result, changes) == result
        output = P/'research/horizon-other-pcm-shared-bank-candidate'/relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(result)
        files.append({'target': relative, 'base_sha256': hashlib.sha256(pc_data).hexdigest(),
                      'ps_sha256': hashlib.sha256(ps_data).hexdigest(),
                      'candidate_sha256': hashlib.sha256(result).hexdigest(), 'changes': changes})
    report = {'schema_version': 1, 'module': 'horizon-other', 'files': files,
              'install_allowed': False,
              'limits': ['Only certified Sound codec and media-size fields changed; common bank loading still pending.',
                         'Use stable-ID deltas during final composition, not whole shared-bank replacements.',
                         'PCM bank platform/native acceptance remains pending; no dynamic validation.']}
    for output in (P/'research/horizon-other-audio-object-deltas.json', P/'staging/Horizon-other-module/merge/audio-object-deltas.json'):
        output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'shared_banks': len(files), 'certified_sound_objects': sum(len(r['changes']) for r in files)}))


if __name__ == '__main__':
    main()
