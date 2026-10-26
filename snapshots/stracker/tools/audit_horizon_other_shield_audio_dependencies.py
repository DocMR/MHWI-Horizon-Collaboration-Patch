"""Resolve explicit shield bank dependencies against local objects and PC Init."""
from pathlib import Path
import hashlib
import json
import sys

P = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(P/'.runtime/wwiser-source'))
from wwiser.parser.wparser import Parser
from audit_horizon_other_shield_audio_events import descendants


def load(path):
    parser = Parser()
    assert parser.parse_bank(str(path))
    bank, = parser.get_banks()
    assert bank.get_error_count() == bank.get_skip_count() == 0
    return bank


def main():
    shield = P/'staging/Horizon-other-module/resources/nativePC/sound/wwise/Windows/pl068_0010_epvsp.nbnk'
    init = P/'MHWI解包/chunk/sound/wwise/Windows/Init.nbnk'
    bank, common = load(shield), load(init)
    definitions = {}
    for origin, tree in [('shield', bank), ('PC_Init', common)]:
        for node in descendants(tree):
            attrs = node.get_attrs()
            if node.get_nodename() != 'field' or attrs.get('type') != 'sid':
                continue
            name = attrs.get('name')
            if name in ('ulID', 'RTPC_ID', 'ulStateGroupID', 'ulStateID'):
                definitions.setdefault(attrs['value'], []).append(dict(origin=origin,
                    field=name, offset=attrs['offset']))
    roles = {'OverrideBusId': 'ulID', 'auxID': 'ulID',
             'uAttenuationID': 'ulID', 'DirectParentID': 'ulID',
             'ulStateInstanceID': 'ulStateID', 'ulStateGroupID': 'ulStateGroupID',
             'RTPCID': 'RTPC_ID'}
    rows = []
    for node in descendants(bank):
        attrs = node.get_attrs()
        name, value = attrs.get('name'), attrs.get('value')
        if node.get_nodename() != 'field' or name not in roles or not value:
            continue
        found = [d for d in definitions.get(value, []) if d['field'] == roles[name]]
        assert found, (name, value)
        rows.append(dict(field=name, identity=value, offset=attrs['offset'], definitions=found))
    assert len({r['identity'] for r in rows if r['field'] == 'RTPCID'}) == 4
    assert {r['identity'] for r in rows if r['field'] == 'OverrideBusId'} == {734376344, 439959608}
    report = dict(files=[dict(path=str(f.relative_to(P)), sha256=hashlib.sha256(f.read_bytes()).hexdigest())
                         for f in (shield, init)], parser_errors=0, parser_skips=0,
                  explicit_dependency_references=rows,
                  scope='Nonzero parent, attenuation, bus, aux, state-instance/group and RTPC references only.',
                  limits=['State value IDs are selectors, not independently required HIRC objects.',
                          'RTPC default definitions do not prove runtime parameter updates.',
                          'No proof of runtime loading, bus effects/plugin availability or native decoding.',
                          'No playback or game execution.'])
    (P/'research/horizon-other-shield-audio-dependencies.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(references=len(rows), unresolved=0,
        pc_init_ids=len({r['identity'] for r in rows if any(d['origin']=='PC_Init' for d in r['definitions'])}))))


if __name__ == '__main__':
    main()
