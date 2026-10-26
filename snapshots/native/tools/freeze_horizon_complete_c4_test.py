"""Freeze the native PS Aloy voice route repair and remove the cancelled notice."""
import freeze_horizon_complete_c3_test as freeze

freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c3-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c4-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c4-aloy-voice-20261022/dinput8.dll'
freeze.LABEL='C4'
freeze.SOURCE_TAG='c4'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c4-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(
    status='private_c4_aloy_voice_repair_pending_live_acceptance',
    runtime_build='horizon-complete-20261022-c4-aloy-voice-no-notice',
    live_acceptance='C3 Focus/tutorial/Banuk/Stormslinger passed user testing; Aloy alpha/gamma, Shield-Weaver and Aloy layered failed loading/preview. C4 not installed.',
    first_entry_notice='Cancelled by user; no notice participant or title Hook in complete-content session',
    remaining_project_items=['Retest all three Aloy full sets and Aloy layered preview/equip/departure after voice route repair',
      'Actual voice and full resource-task completion acceptance',
      'Remaining full-content acceptance and final cross-PC delivery'])

if __name__=='__main__':freeze.main()
