"""Stage six source quests and reward references without runtime activation."""
from pathlib import Path
import hashlib
import json
import re
import struct
from cryptography.hazmat.decrepit.ciphers.algorithms import Blowfish
from cryptography.hazmat.primitives.ciphers import Cipher, modes
from build_horizon_other_text_candidate import parse
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]
QUESTS = (67101, 67102, 67607, 67802, 67803, 67804)
REMS = (70045, 70046, 70088, 71035, 71054, 71055)
SOURCE = P/'research/horizon-other-quest-source'
MODULE = P/'staging/Horizon-other-module'


def swap(data):
    assert len(data) % 4 == 0
    return b''.join(data[i:i+4][::-1] for i in range(0, len(data), 4))


def main():
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    index = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))
    paths = {f'quest/questData_{q}.mib' for q in QUESTS} | {f'quest/rem/remData_{r}.rem' for r in REMS}
    selected = []
    for row in index['files']:
        match = re.fullmatch(r'common/text/quest/q(\d+)_([^/]+)\.gmd', row['path'])
        if row['path'] in paths or match and int(match[1]) in QUESTS:
            selected.append(row)
    assert paths <= {r['path'] for r in selected}
    receipts = []
    for archive_name in sorted({r['archive'] for r in selected}):
        archive = ns['Archive'](Path(archive_name))
        try:
            for row in [r for r in selected if r['archive'] == archive_name]:
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                target = (SOURCE/row['path']).resolve()
                assert target.is_relative_to(SOURCE.resolve())
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                receipts.append({**row, 'sha256': hashlib.sha256(data).hexdigest()})
        finally:
            archive.f.close()
    key = b'TZNgJfzyD2WKiuV4SglmI6oN5jP2hhRJcBwzUooyfIUTM4ptDYGjuRTP'
    cipher = Cipher(Blowfish(key), modes.ECB())
    image=CodeImage()
    start,end=0x1EA1E30,0x1EA2080
    code=image.code[start-image.base:end-image.base]
    assert all(image.valid[start-image.base:end-image.base])
    draw=list(image.md.disasm(code,start))
    assert sum(i.size for i in draw)==len(code)
    by={i.address:i for i in draw}
    pins={0x1EA1F2D:'418b4010',0x1EA1F35:'448d6001',0x1EA1FA7:'413bec',
          0x1EA1FAC:'33d2',0x1EA1FB7:'e8742e2d00',0x1EA1FC3:'f7f6',
          0x1EA1FE8:'8b548714',0x1EA1FEC:'0fb65c3854'}
    for address,expected in pins.items():assert by[address].bytes.hex()==expected
    reward_selection={'exe_sha256':image.meta['exe_sha256'],
        'code_range':[hex(start),hex(end)],'code_sha256':hashlib.sha256(code).hexdigest(),
        'branch_pins':{hex(a):v for a,v in pins.items()},
        'policy':'Preserve original selection, counts, skills, item transfer and save behavior; no extra grant hook.',
        'limit':'Conditional table semantics only; actual caller counts, inventory transfer and persistence are not established here.'}
    quests = []
    for q, expected_rem in zip(QUESTS, REMS):
        raw = (SOURCE/f'quest/questData_{q}.mib').read_bytes()
        assert len(raw) % 8 == 0
        dec = cipher.decryptor()
        plain = swap(dec.update(swap(raw))+dec.finalize())
        enc = cipher.encryptor()
        assert swap(enc.update(swap(plain))+enc.finalize()) == raw
        assert struct.unpack_from('<I', plain)[0] == 0x18091001
        assert struct.unpack_from('<I', plain, 10)[0] == q
        rems = struct.unpack_from('<3I', plain, 0x88)
        assert rems == (expected_rem, 0, 0)
        decoded = SOURCE/f'decoded/questData_{q}.bin'
        decoded.parent.mkdir(parents=True, exist_ok=True)
        decoded.write_bytes(plain)
        rem = (SOURCE/f'quest/rem/remData_{expected_rem}.rem').read_bytes()
        assert len(rem) == 114 and struct.unpack_from('<IH', rem) == (0x18091001, 0x40)
        items = struct.unpack_from('<16I', rem, 18)
        mode=struct.unpack_from('<I',rem,14)[0]
        weights=rem[98:114]
        assert mode==0 and items[0] and weights[0]>0 and rem[82]>0
        assert all(item or weight==0 for item,weight in zip(items,weights))
        assert sum(weight for item,weight in zip(items,weights) if item)==100
        rewards = [{'slot': i, 'item_id': item, 'quantity': rem[82+i], 'weight': rem[98+i]}
                   for i, item in enumerate(items) if item]
        languages = sorted(re.fullmatch(rf'common/text/quest/q{q}_([^/]+)\.gmd', r['path'])[1]
                           for r in selected if re.fullmatch(rf'common/text/quest/q{q}_([^/]+)\.gmd', r['path']))
        quests.append({'quest_id': q, 'raw_star_byte': plain[14],
                       'raw_rank_dword': struct.unpack_from('<I', plain, 15)[0],
                       'raw_file_flags': hex(struct.unpack_from('<I', plain, 0x84)[0]),
                       'rem_ids': list(rems), 'rewards': rewards,
                       'rem_mode': mode,
                       'eligible_weight_sum':100,
                       'conditional_first_draw':{'item_id':items[0],'quantity':rem[82],
                           'condition':'Original draw called with positive count; source mode0 uses threshold0 for first selection.'},
                       'source_languages': languages, 'mib_encryption_roundtrip': True,
                       'mib_plain_sha256': hashlib.sha256(plain).hexdigest()})
    resources, shared_text = [], []
    for row in receipts:
        data = (SOURCE/row['path']).read_bytes()
        base = P/'MHWI解包/chunk'/row['path']
        if row['path'].endswith('.gmd'):
            _, values, keys = parse(data)
            assert all(value.decode('utf-8') is not None for value in values)
        state = 'new_resource'
        if base.exists():
            state = 'provided_by_pc_base' if base.read_bytes() == data else 'shared_file_requires_merge'
            if state == 'shared_file_requires_merge' and row['path'].endswith('.gmd'):
                old = base.read_bytes()
                _, old_values, old_keys = parse(old)
                assert old_keys == keys and len(old_values) == len(values)
                shared_text.append({'target': row['path'], 'base_sha256': hashlib.sha256(old).hexdigest(),
                                    'changes': [{'key': keys[i].decode('utf-8'), 'index_hint': i,
                                                 'before': old_values[i].decode('utf-8'), 'after': values[i].decode('utf-8')}
                                                for i in keys if old_values[i] != values[i]]})
        if state == 'new_resource':
            target = MODULE/'resources/nativePC'/row['path']
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                assert target.read_bytes() == data
            else:
                target.write_bytes(data)
        resources.append({'path': row['path'], 'sha256': row['sha256'], 'size': len(data), 'state': state})
    report = {'status': 'Static source resources only; runtime registration and full dependencies unfinished',
              'source_files': receipts, 'quests': quests, 'resources': resources,
              'native_reward_selection':reward_selection,
              'limits': ['Original event flags and other quest bytes are preserved; permanent-free adaptation is not yet activated.',
                         'Raw rank/star bytes require native consumer mapping; never apply the existing two-MR-quest hook to all six.',
                         'Reward slots are serialized facts, not an executed reward draw or proof of successful material acquisition.',
                         'Complete scene/script dependencies, prerequisites and first-clear/save integration remain pending.',
                         'Only ten PS quest languages are present here; four project/PC-terminology languages are separately recorded in merge/quest-project-text.json.']}
    (P/'research/horizon-other-six-quest-resources.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (MODULE/'merge/quest-resources.json').write_text(json.dumps({'schema_version': 1, 'module': 'horizon-other',
        'policy': 'Deduplicate identical paths/hashes with Stormslinger; resolve different bytes explicitly; register each quest ID once.',
        'resources': resources, 'quests': quests, 'native_reward_selection':reward_selection}, ensure_ascii=False, indent=2), encoding='utf-8')
    (MODULE/'merge/quest-text-deltas.json').write_text(json.dumps({'schema_version': 1, 'module': 'horizon-other',
        'files': shared_text}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'source_files': len(receipts), 'new_resources': sum(r['state']=='new_resource' for r in resources),
                      'pc_base_resources': sum(r['state']=='provided_by_pc_base' for r in resources),
                      'merge_required': sum(r['state']=='shared_file_requires_merge' for r in resources),
                      'quests': [{'id': q['quest_id'], 'rem': q['rem_ids'][0], 'languages': len(q['source_languages'])} for q in quests]}))


if __name__ == '__main__':
    main()
