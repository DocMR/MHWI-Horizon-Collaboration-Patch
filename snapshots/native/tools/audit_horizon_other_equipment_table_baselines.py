"""Certify retained equipment tables and export preserve-only merge inputs."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]
TABLES = ('amr_series.amrs', 'armor.am_dat', 'armor.eq_crt', 'bow.wp_dat_g',
          'customOuterBow.cus_ou', 'customOuterRecipe.cus_otr', 'ot_equip.eq_crt',
          'ot_series.amrs', 'otomoArmor.oam_dat', 'otomoWeapon.owp_dat',
          'skill_data.skl_dat', 'skill_point_data.skl_pt_dat', 'weapon.eq_crt', 'weapon.eq_cus',
          'equip_custom.eq_cus')


def main():
    files = []
    for name in TABLES:
        relative = 'common/equip/'+name
        ps = (P/'research/horizon-other-source'/relative).read_bytes()
        pc = (P/'MHWI解包/chunk'/relative).read_bytes()
        assert ps == pc, relative
        row = {'path': relative, 'size': len(pc), 'sha256': hashlib.sha256(pc).hexdigest(),
               'ps_pc_byte_identical': True, 'operation': 'preserve_existing_pc_table',
               'replacement_payload': None}
        if name != 'otomoWeapon.owp_dat':
            row['serialized_count'] = struct.unpack_from('<I', pc, 6)[0]
        else:
            row['count_note'] = 'Encrypted OWP, count certified separately after decryption; not read from ciphertext.'
        files.append(row)
    report = {'schema_version': 1, 'module': 'horizon-other', 'files': files,
              'merge_rule': 'No whole-table replacement is requested. Preserve other module changes; inspect stable record identities if later edits become necessary.',
              'finding': f'The entire{len(files)} source tables already exist byte-for-byte in the extracted PC baseline.',
              'limits': ['Data retention alone does not establish menu visibility, native platform gates, crafting admission or special skill behavior.',
                         'PC baseline hashes identify the inspected extraction; installation bootstrap must bind the intended game build.',
                         'No game, native reader or dynamic validation executed.']}
    for output in (P/'research/horizon-other-equipment-table-baselines.json',
                   P/'staging/Horizon-other-module/merge/equipment-table-baselines.json'):
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'identical_tables': len(files), 'bytes': sum(r['size'] for r in files), 'table_overrides': 0}))


if __name__ == '__main__':
    main()
