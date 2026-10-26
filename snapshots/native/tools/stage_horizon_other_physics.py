"""Stage structurally matching CTC/CCL references; retain unusual tails separately."""
from pathlib import Path
import hashlib
import json
import struct

P = Path(__file__).resolve().parents[1]
SOURCE = P/'research/horizon-other-equipment-source'
OUTPUT = P/'staging/Horizon-other-module/resources/nativePC'


def layout(raw, suffix):
    if suffix == '.ccl':
        assert struct.unpack_from('<II', raw) == (0x4C4343, 0x80618)
        count, byte_size = struct.unpack_from('<II', raw, 8)
        assert byte_size == count*64
        expected = 16+count*64
        return {'count': count, 'expected_bytes': expected, 'remaining_bytes': len(raw)-expected}
    assert struct.unpack_from('<II', raw) == (0x435443, 28)
    chains, nodes = struct.unpack_from('<II', raw, 16)
    assert len(raw) >= 80+80*chains
    counts = [struct.unpack_from('<I', raw, 80+80*i)[0] for i in range(chains)]
    assert sum(counts) == nodes
    # rCnsTinyChain reader2382330 maps file+4E to object+F6 and,
    # when set, reads one extra DWORD per chain after all ordinary nodes.
    extension_enabled = bool(raw[0x4E])
    expected = 80+80*chains+112*nodes+(4*chains if extension_enabled else 0)
    return {'chains': chains, 'nodes': nodes, 'per_chain_nodes': counts,
            'extension_enabled': extension_enabled,
            'expected_bytes': expected, 'remaining_bytes': len(raw)-expected}


def main():
    witnesses = {}
    for suffix in ('.ctc', '.ccl'):
        for path in (P/'MHWI解包/chunk').rglob('*'+suffix):
            raw = path.read_bytes()
            try:
                parsed = layout(raw, suffix)
                if parsed['remaining_bytes'] != 0:
                    continue
                witnesses[suffix] = {'path': path.relative_to(P/'MHWI解包/chunk').as_posix(),
                                     'sha256': hashlib.sha256(raw).hexdigest(), **parsed}
                break
            except (AssertionError, struct.error):
                continue
        assert suffix in witnesses
    files, deferred = [], []
    for source in sorted(SOURCE.rglob('*')):
        if source.suffix not in ('.ctc', '.ccl'):
            continue
        raw = source.read_bytes()
        parsed = layout(raw, source.suffix)
        row = {'path': source.relative_to(SOURCE).as_posix(), 'size': len(raw),
               'sha256': hashlib.sha256(raw).hexdigest(), **parsed}
        assert parsed['remaining_bytes'] >= 0
        if parsed['remaining_bytes']:
            row['uninterpreted_tail_hex'] = raw[parsed['expected_bytes']:].hex()
            deferred.append(row)
            continue
        target = OUTPUT/row['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert target.read_bytes() == raw
        else:
            target.write_bytes(raw)
        files.append(row)
    report = {'status': 'CTC read extents certified statically; downstream physics and CCL field semantics pending',
              'ctc_native_reader_evidence': 'horizon-other-ctc-native-reader.json',
              'pc_layout_witnesses': witnesses, 'files': files, 'deferred': deferred,
              'limits': ['Header, record length and chain-node count consistency only; no physics simulation or dynamic tests.',
                         'Unusual source trailers remain preserved in the reference folder, never silently truncated.',
                         'Bone references, native flags and exact PS/PC behavior still require static consumer audit.']}
    (P/'research/horizon-other-physics-layout.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    (P/'staging/Horizon-other-module/merge/physics-candidates.json').write_text(json.dumps({
        'schema_version': 1, 'module': 'horizon-other', 'files': files, 'deferred': deferred,
        'operation': 'preserve original serialization; static layout candidates'}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'staged': len(files), 'deferred': [{'path': r['path'], 'tail_bytes': r['remaining_bytes']} for r in deferred]}, ensure_ascii=False))


if __name__ == '__main__':
    main()
