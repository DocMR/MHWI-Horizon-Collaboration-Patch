"""Decode reference Palico weapon tables and validate their serialized layout."""
from pathlib import Path
import hashlib
import json
import struct
from cryptography.hazmat.decrepit.ciphers.algorithms import Blowfish
from cryptography.hazmat.primitives.ciphers import Cipher, modes
from build_horizon_other_content_inventory import strings

P = Path(__file__).resolve().parents[1]
KEY_SOURCE = 'https://raw.githubusercontent.com/Synthlight/MHW-Editor/master/MHW-Template/EncryptionKeys.cs'
KEY = b'FZoS8QLyOyeFmkdrz73P9Fh2N4NcTwy3QQPjc1YRII5KWovK6yFuU8SL'


def swap(data):
    assert len(data) % 4 == 0
    return b''.join(data[i:i+4][::-1] for i in range(0, len(data), 4))


def decode(data):
    assert len(data) % 8 == 0
    cipher = Cipher(Blowfish(KEY), modes.ECB())
    dec = cipher.decryptor()
    plain = swap(dec.update(swap(data)) + dec.finalize())
    enc = cipher.encryptor()
    assert swap(enc.update(swap(plain)) + enc.finalize()) == data
    assert struct.unpack_from('<IH', plain) == (0x18091001, 0x9A)
    count = struct.unpack_from('<I', plain, 6)[0]
    end = 10 + count * 38
    assert end <= len(plain) and 0 < len(plain)-end <= 8
    assert not any(plain[end:])
    return plain, count


def main():
    results, candidates = [], []
    for platform, root, text_dir in [('PS', P/'research/horizon-other-source', 'ps4'),
                                      ('PC', P/'MHWI解包/chunk', 'steam')]:
        source = root / 'common/equip/otomoWeapon.owp_dat'
        encrypted = source.read_bytes()
        plain, count = decode(encrypted)
        text = strings(root / f'common/text/{text_dir}/ot_weapon_eng.gmd')
        for index in range(count):
            row = plain[10+38*index:10+38*(index+1)]
            assert struct.unpack_from('<I', row)[0] == index
            name, description = struct.unpack_from('<HH', row, 34)
            assert name < len(text) and description < len(text)
            if (platform == 'PS' and text[name].startswith(('Felyne Watcher', 'Forgefire')) or
                    platform == 'PC' and index in {r['row'] for r in candidates if r['platform'] == 'PS'}):
                candidates.append({'platform': platform, 'row': index, 'name_index': name,
                                   'native_equipment_id': struct.unpack_from('<H', row, 32)[0],
                                   'description_index': description, 'name': text[name],
                                   'raw_hex': row.hex()})
        output = P / f'research/horizon-other-palico-weapon-{platform.lower()}.decrypted.bin'
        output.write_bytes(plain)
        results.append({'platform': platform, 'source': str(source.relative_to(P)), 'count': count,
                        'encrypted_sha256': hashlib.sha256(encrypted).hexdigest(),
                        'plain_sha256': hashlib.sha256(plain).hexdigest(),
                        'padding_bytes': len(plain)-10-count*38, 'roundtrip': True})
    report = {'key_source': KEY_SOURCE, 'record_size': 38, 'files': results,
              'weapon_candidates': candidates,
              'ps_pc_identical_plain': results[0]['plain_sha256'] == results[1]['plain_sha256'],
              'limits': ['38-byte rows and final name/description fields are validated structurally across the entire table; native reader consumer audit pending.',
                         'No game code execution, installation or dynamic validation.']}
    (P/'research/horizon-other-palico-weapon.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
