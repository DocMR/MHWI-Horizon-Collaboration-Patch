"""Check target paired recipe costs and one-charge/two-output code shape."""
import hashlib
import json
from analyze_hzd_code import CodeImage, P
from pc_unwind_groups import decode_group


def main():
    im=CodeImage()
    associations=json.loads((P/'research/horizon-other-palico-set-recipe-path.json').read_text(encoding='utf-8'))['associations']
    recipes=json.loads((P/'research/horizon-other-armor-palico-recipes.json').read_text(encoding='utf-8'))['recipes']
    pairs=[]
    for series in (66,156,179):
        members=sorted((r for r in associations if r['series']==series),key=lambda r:r['slot'])
        assert [r['slot'] for r in members]==[0,1]
        matched=[]
        for member in members:
            row,=[r for r in recipes if r['file']=='ot_equip.eq_crt' and r['recipe_row']==member['recipe_row']]
            assert row['pc_record_identical']
            matched.append(row)
        head,body=matched
        # Serialized category+equipment-index differ; every remaining byte agrees.
        assert bytes.fromhex(head['ps_raw_hex'])[3:]==bytes.fromhex(body['ps_raw_hex'])[3:]
        assert head['materials']==body['materials'] and head['unlock_item']==body['unlock_item']
        pairs.append(dict(series=series,head_recipe=head['recipe_row'],body_recipe=body['recipe_row'],
            unlock_item=head['unlock_item'],materials=head['materials'],identical_cost_payload=True))
    fragments,code=decode_group(im,0x1DC2570)
    material_calls=[i.address for i in code if i.mnemonic=='call' and i.op_str=='0x16b85b0']
    assert material_calls==[0x1DC262B]
    by={i.address:i for i in code}
    assert by[0x1DC2700].op_str=='r12d, 2' and by[0x1DC2765].op_str=='0x1dc2706'
    report=dict(exe_sha256=im.meta['exe_sha256'],pairs=pairs,
        handler_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in code)).hexdigest(),
        material_call_sites=[hex(a) for a in material_calls],
        finding='Paired head/body recipes have identical cost payloads. Handler charges selected recipe once before its two-entry output loop.',
        limits=['Need prove full-set menu mode selects this handler/row without additional upstream charges.',
            'Do not sum paired recipe costs or add new payment logic merely because two outputs exist.',
            'Funds price equality and complete admission/save semantics remain separate.',
            'Static data/code inspection only; no mechanism execution.'])
    (P/'research/horizon-other-palico-set-cost-shape.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(pairs=len(pairs),material_charge_sites=len(material_calls))))


if __name__=='__main__':
    main()
