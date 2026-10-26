"""Extract and compare retained delivery/card/item tables, without installing."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]
PATHS = ('common/facility/l_delivery.stmp', 'common/guildcard/gc_data_pose.gcod',
         'common/guildcard/gc_data_title1.gcod', 'common/guildcard/gc_data_title2.gcod',
         'common/guildcard/gc_data_bg.gcod', 'common/guildcard/gc_data_face.gcod',
         'common/item/itemData.itm')


def main():
    loader = P/'tools/extract_ps_stormslinger.py'
    ns = {'__file__': str(loader)}
    exec(loader.read_text(encoding='utf-8-sig').split('selected={};')[0], ns)
    inventory = json.loads((P/'research/horizon-other-archive-directory.json').read_text(encoding='utf-8'))['files']
    selected = [row for row in inventory if row['path'] in PATHS]
    assert {r['path'] for r in selected} == set(PATHS)
    reports = []
    for name in sorted({r['archive'] for r in selected}):
        archive = ns['Archive'](Path(name))
        try:
            for row in [r for r in selected if r['archive'] == name]:
                data = archive.read(row['offset'], row['size'])
                assert len(data) == row['size']
                target = P/'research/horizon-other-source'/row['path']
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.is_file():
                    assert target.read_bytes() == data
                else:
                    target.write_bytes(data)
                pc = (P/'MHWI解包/chunk'/row['path']).read_bytes()
                count = struct.unpack_from('<I', data, 6)[0]
                pc_count = struct.unpack_from('<I', pc, 6)[0]
                assert count and (len(data)-10) % count == 0
                width = (len(data)-10)//count
                assert pc_count and (len(pc)-10)//pc_count == width and (len(pc)-10)%pc_count == 0
                common = min(count, pc_count)
                different = [i for i in range(common) if data[10+i*width:10+(i+1)*width] != pc[10+i*width:10+(i+1)*width]]
                reports.append(dict(**row, output=target.relative_to(P).as_posix(),
                    source_sha256=hashlib.sha256(data).hexdigest(), pc_sha256=hashlib.sha256(pc).hexdigest(),
                    identical=data == pc, record_count=count, pc_record_count=pc_count,
                    inferred_record_width=width, different_common_rows=different))
        finally:
            archive.f.close()
    report = dict(status='Static serialized table comparison; native field meanings not inferred',
        files=reports, limits=['Widths/counts are arithmetic partitions of the retained header and bytes, not complete field schemas.',
            'Text label numeric suffix is not automatically a table row or runtime identifier.',
            'Table identity does not prove availability, DLC admission, animations, completion or save paths.',
            'No target execution, dynamic validation or game-directory writes.'])
    for path in (P/'research/horizon-other-interaction-table-baselines.json',
                 P/'staging/Horizon-other-module/merge/interaction-table-baselines.json'):
        path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps([dict(path=r['path'], identical=r['identical'], count=r['record_count'],
        width=r['inferred_record_width'], differing_rows=r['different_common_rows'][:30]) for r in reports]))


if __name__ == '__main__':
    main()
