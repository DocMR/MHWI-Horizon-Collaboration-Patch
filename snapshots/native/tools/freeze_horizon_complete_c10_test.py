"""Freeze compact notice and native-idle restoration; no installation."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c9-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c10-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c10-notice-close-20261022/dinput8.dll'
freeze.LABEL='C10'
freeze.SOURCE_TAG='c10'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c10-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(
    status='private_c10_compact_notice_pending_live_acceptance',
    runtime_build='horizon-complete-20261022-c10-notice-close',
    candidate_resources='C9 resources unchanged; compact owned layout and restore only after native message queue idle',
    live_acceptance='Other content accepted by user. C9 video shows left body but window too large and stock smaller frame flashes on close. C10 acceptance pending.',
    first_entry_notice='Six unchanged pages;760x620 local frame,660x460 left body, separate button; retain font/geometry through closing until native idle, including final page; C10 acknowledgement',
    remaining_project_items=['C10 six-page compact layout and no smaller-frame flash on close',
                             'Final actual install/uninstall and portable native delivery'])
if __name__=='__main__':freeze.main()
