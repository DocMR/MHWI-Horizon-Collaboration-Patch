"""Convert source audio data with FFmpeg; never load or execute game code."""
from pathlib import Path
import array
import hashlib
import json
import math
import struct
import subprocess

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-effect-audio-source/sound/wwise/PS4'
OUTPUT = P/'research/horizon-other-pcm-media-candidate'
FFMPEG = P/'.runtime/video/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'


def chunk(tag, data):
    return tag+struct.pack('<I', len(data))+data


def bank_chunks(data):
    result, cursor = {}, 0
    while cursor < len(data):
        tag, size = struct.unpack_from('<4sI', data, cursor)
        assert tag not in result and cursor+8+size <= len(data)
        result[tag] = data[cursor+8:cursor+8+size]
        cursor += 8+size
    assert cursor == len(data)
    return result


def convert_atrac9_media(raw, bank_name, identity, output_root=OUTPUT):
    assert raw[:4] == b'RIFF' and raw[8:12] == b'WAVE'
    # Wwise RIFF's legacy declared total can differ from DIDX.
    # Read the actual bounded subchunks instead of trimming its tail.
    cursor, riff = 12, {}
    while cursor < len(raw):
        tag, size = struct.unpack_from('<4sI', raw, cursor)
        assert cursor+8+size <= len(raw) and tag not in riff
        riff[tag] = raw[cursor+8:cursor+8+size]
        cursor += 8+size+(size & 1)
    assert cursor == len(raw)
    fmt = riff[b'fmt ']
    assert len(fmt) == 36 and struct.unpack_from('<HHI', fmt) == (0xfffc, 1, 48000)
    assert set(riff) <= {b'fmt ', b'JUNK', b'data'}
    config = fmt[24:28]
    samples, delay = struct.unpack_from('<2I', fmt, 28)
    assert config == bytes.fromhex('fe7007f0') and delay == 256 and samples > 0
    # Same ATRAC9 WAVEFORMATEXTENSIBLE wrapper as the already converted
    # body068 source, preserving compressed data and configuration.
    wrapper_fmt = struct.pack('<HHIIHHH', 0xfffe, 1, 48000,
                              struct.unpack_from('<I', fmt, 8)[0], 256, 0, 34)
    wrapper_fmt += struct.pack('<HI', 0, 4)
    wrapper_fmt += bytes.fromhex('d242e147ba368d4d88fc61654f8c836c')
    wrapper_fmt += struct.pack('<I', 1)+config+struct.pack('<I', 0)
    assert len(wrapper_fmt) == 52
    body = b'WAVE'+chunk(b'fmt ', wrapper_fmt)+chunk(b'data', riff[b'data'])
    wrapper = b'RIFF'+struct.pack('<I', len(body))+body
    directory = output_root/Path(bank_name).stem
    directory.mkdir(parents=True, exist_ok=True)
    input_path = directory/f'{identity}-atrac9.wav'
    input_path.write_bytes(wrapper)
    process = subprocess.run([str(FFMPEG), '-nostdin', '-hide_banner', '-loglevel', 'error',
                              '-i', str(input_path), '-f', 'f32le', '-acodec', 'pcm_f32le', 'pipe:1'],
                             capture_output=True, check=True)
    decoded = array.array('f')
    decoded.frombytes(process.stdout)
    assert len(decoded) >= delay+samples and all(math.isfinite(x) for x in decoded)
    valid = decoded[delay:delay+samples]
    pcm = array.array('h', (max(-32768, min(32767, round(x*32768))) for x in valid)).tobytes()
    # Native PC compact PCM format used by28A7BC0 and body068 candidate.
    native_fmt = struct.pack('<HHIIHHHHI', 0xfffe, 1, 48000, 96000, 2, 16, 0, 0, 0x4101)
    body = b'WAVE'+chunk(b'fmt ', native_fmt)+chunk(b'JUNK', bytes(196))+chunk(b'data', pcm)
    assert body[240:244] == b'data'
    converted = b'RIFF'+struct.pack('<I', len(body))+body
    target = directory/f'{identity}.wem'
    target.write_bytes(converted)
    return {'bank': bank_name, 'id': identity, 'source_sha256': hashlib.sha256(raw).hexdigest(),
                 'samples': samples, 'encoder_delay': delay, 'decoded_samples': len(decoded),
                 'valid_float_sha256': hashlib.sha256(valid.tobytes()).hexdigest(),
                 'clipped_samples': sum(x < -1 or x >= 1 for x in valid),
                 'path': target.relative_to(P).as_posix(), 'size': len(converted),
                 'sha256': hashlib.sha256(converted).hexdigest()}


def main():
    rows = []
    refs = json.loads((P/'research/horizon-other-effect-audio-references.json').read_text(encoding='utf-8'))
    names = {Path(r['target']).name for r in refs['dependencies']
             if r['kind'] == 'audio_bank' and not r['pc_present']}
    for source in sorted(SOURCE/name for name in names):
        parts = bank_chunks(source.read_bytes())
        index, payload = parts.get(b'DIDX', b''), parts.get(b'DATA', b'')
        for position in range(0, len(index), 12):
            identity, offset, length = struct.unpack_from('<3I', index, position)
            raw = payload[offset:offset+length]
            assert len(raw) == length
            rows.append(convert_atrac9_media(raw, source.name, identity))
        print(source.name, len(index)//12, 'media converted', flush=True)
    report = {'converter_sha256': hashlib.sha256(FFMPEG.read_bytes()).hexdigest(), 'media': rows,
              'state': 'PCM media data candidates only; bank reconstruction and native acceptance pending',
              'limits': ['Offline asset conversion only; no game code, playback or dynamic validation.',
                         'Bank graph, platform metadata and external references are not changed by this tool.']}
    (P/'research/horizon-other-pcm-media-conversion.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'converted': len(rows), 'samples': sum(r['samples'] for r in rows),
                      'clipped_samples': sum(r['clipped_samples'] for r in rows)}))


if __name__ == '__main__':
    main()
