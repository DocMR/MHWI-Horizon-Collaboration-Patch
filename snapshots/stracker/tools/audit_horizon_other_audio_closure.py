"""Check staged sound-param/container and bank-name edges as file data."""
from pathlib import Path
import json
import re
import struct
from stage_horizon_other_sound_params import parse

P = Path(__file__).resolve().parents[1]
MODULE = P/'staging/Horizon-other-module'
ROOT = MODULE/'resources/nativePC'
BASE = P/'MHWI解包/chunk'


def locate(path):
    if (ROOT/path).is_file():
        return 'module candidate'
    if (BASE/path).is_file():
        return 'PC baseline'
    return 'missing'


def main():
    params, banks = [], []
    for source in sorted(ROOT.rglob('*.epvsp')):
        parsed = parse(source.read_bytes())
        params.append({'owner': source.relative_to(ROOT).as_posix(),
                       'container': parsed['container'], 'state': locate(parsed['container'])})
    for source in sorted(ROOT.rglob('*.wwbk')):
        data = source.read_bytes()
        assert data[:4] == b'WWBK'
        version, count, reserved = struct.unpack_from('<3I', data, 4)
        assert version == 4 and reserved == 0
        names = re.findall(rb'[^\x00]+', data[16:])
        assert len(names) == count
        for name in names:
            target = 'sound/wwise/Windows/'+name.decode('ascii')+'.nbnk'
            banks.append({'owner': source.relative_to(ROOT).as_posix(), 'bank': target, 'state': locate(target)})
    missing = [r for r in params+banks if r['state'] == 'missing']
    report = {'module': 'horizon-other', 'sound_parameter_edges': params, 'bank_reference_edges': banks,
              'missing': missing,
              'limits': ['This covers EPVSP container paths and staged WWBK bank-name edges only.',
                         'WWCT/WWEV event indices, bootstrap/loading, PCM parser and actual triggering remain separate.',
                         'File closure is not game validation; no target code executed.']}
    (P/'research/horizon-other-audio-file-closure.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'sound_parameter_containers': len(params), 'bank_reference_edges': len(banks), 'missing': len(missing)}))


if __name__ == '__main__':
    main()
