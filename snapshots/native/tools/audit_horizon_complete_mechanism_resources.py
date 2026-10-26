"""Check retained shield parameters and composed effect/audio dependencies."""
from pathlib import Path
import hashlib
import json
import struct
from analyze_hzd_code import CodeImage

P = Path(__file__).resolve().parents[1]
M = P / 'staging/Horizon-complete-integration-20261021/merge'
C = P / 'staging/Horizon-complete-mechanism-payload-20261021-c3/nativePC'
PC = P / 'MHWI解包/chunk'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    source = (PC / 'common/pl/pl_skill_param.plsp').read_bytes()
    ps = (P / 'research/ps-skill-parameter-approved-20261018/pl_skill_param.plsp').read_bytes()
    assert source == ps
    plain = (P / 'research/pl_skill_param.decrypted.bin').read_bytes()
    ps_plain = (P / 'research/ps-skill-parameter-approved-20261018/pl_skill_param.decrypted.bin').read_bytes()
    assert plain == ps_plain
    trace = json.loads((P / 'research/pc-skill-parameter-helper-trace.json').read_text(encoding='utf-8-sig'))
    assert trace['plain_sha256'] == sha(plain)
    expected = [(0xcd26fd, 1891, 12, '0x9d0'),
                (0xcd2706, 1903, 4, None),
                (0xcd2715, 1907, 4, None),
                (0xcd2724, 1911, 4, None)]
    for call, offset, size, destination in expected:
        found = [e for e in trace['events'] if int(e['call'], 0) == call]
        assert len(found) == 1
        assert (found[0]['file_offset'], found[0]['bytes'], found[0]['destination']) == (offset, size, destination)
    code = CodeImage()
    start, end = 0xcd26ec, 0xcd272f
    raw = code.code[start-code.base:end-code.base]
    assert all(code.valid[start-code.base:end-code.base])
    instructions = list(code.md.disasm(raw, start))
    assert sum(i.size for i in instructions) == len(raw)
    for address, mnemonic, operands in [
        (0xcd26ec, 'lea', 'rdx, [rdi + 0x9d0]'),
        (0xcd2709, 'mov', 'dword ptr [rdi + 0x9dc], eax'),
        (0xcd2718, 'mov', 'dword ptr [rdi + 0x9e0], eax'),
        (0xcd2727, 'movss', 'dword ptr [rdi + 0x9e4], xmm0')]:
        assert any(i.address == address and i.mnemonic == mnemonic and i.op_str == operands for i in instructions)
    parameters = dict(zip(['multiplier_0', 'multiplier_1', 'multiplier_2', 'threshold', 'related_count', 'duration'],
                          struct.unpack_from('<fffIIf', plain, 1891)))
    checks = []
    for name, key, field in [('effects.json', 'dependencies', 'path'),
                             ('effect-secondary-refs.json', 'textures', 'path'),
                             ('effect-secondary-refs.json', 'uv_sequences', 'path'),
                             ('effect-audio-references.json', 'dependencies', 'target')]:
        data = json.loads((M/name).read_text(encoding='utf-8-sig'))
        for row in data[key]:
            relative = row[field].replace('\\', '/').replace('/PS4/', '/Windows/')
            target = C/relative
            if not target.is_file():
                target = PC/relative
            assert target.is_file(), relative
            digest = sha(target.read_bytes())
            if row.get('sha256'):
                assert digest == row['sha256'], relative
            checks.append({'manifest': name, 'group': key, 'path': relative,
                           'resolved': target.relative_to(P).as_posix(), 'sha256': digest})
    log = P/'research/horizon-complete-shield-callbacks-test-20261021.log'
    assert 'PASS shared shield callbacks:' in log.read_text(encoding='utf-8-sig')
    result = {'status': 'loaded_parameter_and_retained_dependency_checks_passed_no_live_acceptance',
              'pc_ps_parameter_file_identical': True, 'parameter_file_sha256': sha(source),
              'plain_sha256': sha(plain), 'loaded_shield_parameters': parameters,
              'serialized_bytes': {'offset': 1891, 'size': 24},
              'runtime_resource_offsets': ['0xa80', '0xa84', '0xa88', '0xa8c', '0xa90', '0xa94'],
              'reader_window': {'start': hex(start), 'end': hex(end), 'sha256': sha(raw),
                                'instructions': [code.format(i) for i in instructions]},
              'shared_callback_fixture': {'path': log.relative_to(P).as_posix(), 'sha256': sha(log.read_bytes()),
                                         'synthetic_parameter_values_not_live_observation': True},
              'dependency_records_checked': len(checks), 'dependencies': checks,
              'limits': ['Reader window and previously traced serialized event offsets, not complete reader execution',
                         'Resource existence/hash closure does not prove rendering/playback or native trigger execution',
                         'Actual armor/timer/growth/lifecycle behavior awaits first-four grouped live acceptance']}
    (P/'research/horizon-complete-mechanism-resources-20261021.json').write_text(
        json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'status': result['status'], 'parameters': parameters,
                      'dependency_records_checked': len(checks)}))


if __name__ == '__main__':
    main()
