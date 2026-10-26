"""Bounded exact-displacement PS consumer inventory, not a full-mechanism proof."""
import hashlib
import json
import re
import struct
from analyze_hzd_code import P
from analyze_ps_stormbolt_types import PsCodeImage
from capstone.x86 import X86_OP_MEM, X86_OP_IMM, X86_REG_RIP


def main():
    ps = PsCodeImage()
    # Classify individually evidenced sites, rather than all matching integers.
    roles = {
        0x2C34127: ('sync_lower_band', 'horizon_shield_sync_calls.h'),
        0x2C34199: ('sync_upper_band', 'horizon_shield_sync_calls.h'),
        0x2C34210: ('sync_exhausted', 'horizon_shield_sync_calls.h'),
        0x2C53031: ('status_event_eligibility', 'horizon_shield_status_calls.h'),
        0x2C55B42: ('timer_expiry_prompt', 'horizon_shield_status_calls.h'),
        0x2C659EF: ('intermediate_defense_multiplier', 'horizon_shield_multiplier_call.h'),
        0x2CE08C0: ('reaction_local_gate', 'horizon_shield_reaction_call.h'),
        0x2CEE285: ('growth_source_gate', 'horizon_shield_growth_calls.h'),
        0x2CEE2F3: ('growth_associated_actor_gate_1', 'horizon_shield_growth_calls.h'),
        0x2CEE34C: ('growth_associated_actor_gate_2', 'horizon_shield_growth_calls.h'),
        0x2E33AD9: ('loop_audio_material_eligibility', 'horizon_shield_loop_call.h'),
        0x4998515: ('stormslinger_special_shot_shared_state_consumer', None),
    }
    hits, unresolved, unowned, rejected, raw_count = [], set(), [], [], 0
    for segment in ps.mapped:
        if not segment['flags'] & 1:
            continue
        base = segment['vaddr']
        code = ps.read(base, segment['filesz'])
        for match in re.finditer(bytes.fromhex('1c140000'), code):
            raw_count += 1
            address = base + match.start()
            owner = ps.owner(address)
            if not owner:
                unowned.append(hex(address))
                continue
            body = ps.instructions(address)
            if not body:
                unresolved.add(tuple(owner))
                continue
            indexed = next(((n, i) for n, i in enumerate(body)
                            if i.address <= address < i.address+i.size), None)
            assert indexed
            n, ins = indexed
            load = (ins.mnemonic == 'movsxd' and len(ins.operands) == 2 and
                    ins.operands[1].type == X86_OP_MEM and
                    ins.operands[1].mem.disp == 0x141C and
                    not ins.operands[1].mem.index)
            if not load:
                rejected.append({'raw': hex(address), 'instruction': ps.format(ins),
                    'reason': 'Not a direct signed record-ordinal load; equal offset alone is not a skill binding.'})
                continue
            role, source = roles[ins.address]  # An unknown consumer must stop classification.
            window = body[max(0, n-4):n+15]
            assert any(op.type == X86_OP_MEM and op.mem.disp == 0x12758
                       for i in body[max(0,n-4):n] for op in i.operands)
            source_path = P/'staging/Horizon-other-module/src'/source if source else None
            hits.append({'site': hex(ins.address), 'role': role,
                'owner': [hex(v) for v in owner],
                'full_owner_instruction_count': len(body),
                'full_owner_sha256': hashlib.sha256(ps.read(owner[0], owner[1]-owner[0])).hexdigest(),
                'window': [ps.format(i) for i in window],
                'module_source': source_path.relative_to(P).as_posix() if source_path else None,
                'module_source_sha256': hashlib.sha256(source_path.read_bytes()).hexdigest() if source_path else None,
                'scope': 'other_content_equipment' if source else 'full_stormslinger_chat',
                'coverage': 'adapter_source_present_host_composition_pending' if source else 'excluded_weapon_specific_consumer'})
    assert {int(h['site'],16) for h in hits} == set(roles)
    # Authenticate the excluded consumer's concrete weapon table and ctor.
    ctor = ps.instructions(0x4998E40)
    assert ctor
    assert struct.unpack('<Q', ps.read(0x68783E8+123*8,8))[0] == 0x4998470
    assert any(op.type == X86_OP_MEM and op.mem.base == X86_REG_RIP and
               i.address+i.size+op.mem.disp == 0x68783E8
               for i in ctor for op in i.operands)
    assert any(i.mnemonic == 'mov' and len(i.operands) == 2 and
               i.operands[0].type == X86_OP_MEM and i.operands[0].mem.disp == 0xA50 and
               i.operands[1].type == X86_OP_IMM and i.operands[1].imm == 13 for i in ctor)
    report = {'status': 'bounded_direct_skill209_consumers_classified_not_full_coverage',
        'ps_sha256': hashlib.sha256(ps.data).hexdigest(),
        'raw_exact_displacement_candidates': raw_count,
        'classified_consumers': sorted(hits, key=lambda h:int(h['site'],16)),
        'rejected_equal_offset_candidates': rejected,
        'undecodable_owners': [[hex(v) for v in row] for row in sorted(unresolved)],
        'unowned_raw_candidates': unowned,
        'excluded_weapon_identity': 'PS ctor4998E40 installs VT68783E8, class13; slot123 points4998470. Full Stormslinger shooting belongs to the other chat.',
        'limits': ['Only executable-segment exact displacement141C signed direct loads. Indexed skill lookup, propagated constants, missing/unsupported unwind and incomplete decoding remain outside this denominator.',
            'Source presence is not semantic equivalence or a completed host installation.',
            'Remote/loop state consumers can read shared count without a direct selected-record load and are not enumerated by this scan.',
            'No native execution, dynamic test or game-directory write.']}
    (P/'research/horizon-other-shield-consumer-inventory.json').write_text(
        json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'classified': len(hits), 'equipment': sum(h['module_source'] is not None for h in hits),
        'weapon_excluded': 1, 'raw': raw_count, 'undecodable_owners': len(unresolved),
        'unowned_raw': len(unowned), 'full_coverage': False}))


if __name__ == '__main__':
    main()
