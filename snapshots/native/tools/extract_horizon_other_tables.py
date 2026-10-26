"""Copy selected PS reference tables into this workstream's research folder."""
from pathlib import Path
import hashlib
import json
import re

P = Path(__file__).resolve().parents[1]
OUT = P / 'research/horizon-other-source'
TABLES = {'armor.am_dat', 'armor.eq_crt', 'amr_series.amrs', 'bow.wp_dat_g',
          'weapon.eq_crt', 'weapon.eq_cus', 'ot_equip.eq_crt', 'ot_series.amrs',
          'otomoArmor.oam_dat', 'otomoWeapon.owp_dat', 'skill_data.skl_dat',
          'skill_point_data.skl_pt_dat', 'customOuterBow.cus_ou', 'customOuterRecipe.cus_otr',
          'equip_custom.eq_cus'}
TEXT = {'armor', 'armor_series', 'bow', 'ot_armor', 'ot_series', 'ot_weapon',
        'item', 'ou_bow', 'wep_series', 'panel_tutorial', 'panel_tutorial_ex'}


def main():
    loader = P / 'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    inventory = json.loads((P / 'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))
    selected = []
    for row in inventory['files']:
        path = row['path']
        text = re.fullmatch(r'common/text/(ps4|vfont)/(.+)_([^/_]+)\.gmd', path)
        if (path.startswith('common/equip/') and Path(path).name in TABLES) or (text and (
                text[1] == 'ps4' and text[2] in TEXT or text[1] == 'vfont' and text[2] in {'skill', 'skill_pt'})):
            selected.append(row)
    receipts = []
    for source in sorted({row['archive'] for row in selected}):
        archive = ns['Archive'](Path(source))
        try:
            for row in selected:
                if row['archive'] != source:
                    continue
                target = (OUT / row['path']).resolve()
                assert target.is_relative_to(OUT.resolve())
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    assert target.read_bytes() == data, f'Reference changed: {target}'
                else:
                    target.write_bytes(data)
                receipts.append({**row, 'output': str(target.relative_to(P)),
                                 'sha256': hashlib.sha256(data).hexdigest()})
        finally:
            archive.f.close()
    (P / 'research/horizon-other-source-receipt.json').write_text(json.dumps({
        'authorization': '2026-10-15 user approved the non-Stormslinger content migration plan',
        'limits': ['Reference extraction only; no PC conversion or installation.',
                   'Candidate archive precedence remains to be verified.'],
        'files': receipts}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'Reference tables: {len(receipts)}; bytes: {sum(r["size"] for r in receipts)}')


if __name__ == '__main__':
    main()
