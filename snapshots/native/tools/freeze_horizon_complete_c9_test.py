"""Freeze native body reflow after owned layout properties; no installation."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c8-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c9-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c9-text-reflow-20261022/dinput8.dll'
freeze.LABEL='C9'
freeze.SOURCE_TAG='c9'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c9-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(
    status='private_c9_notice_reflow_pending_live_acceptance',
    runtime_build='horizon-complete-20261022-c9-text-reflow',
    candidate_resources='C8 resources and coordinates unchanged; original body SetText once after layout and anchor properties',
    live_acceptance='C6 other tests accepted by user. C8 body was outside frame; C9 rebuilds glyph cache after layout instead of adjusting coordinates again. Visual acceptance pending.',
    first_entry_notice='Six unchanged pages; existing native large frame and left body; original SetText rebuilds body once per page after properties; localized button retained; C9 acknowledgement',
    remaining_project_items=['C9 actual six-page alignment, full body and button visibility',
                             'Final actual install/uninstall and portable native delivery'])
if __name__=='__main__':freeze.main()
