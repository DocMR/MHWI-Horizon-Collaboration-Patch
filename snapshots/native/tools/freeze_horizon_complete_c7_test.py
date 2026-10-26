"""Freeze the scoped notice layout repair on the user-accepted C6 baseline."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c6-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c7-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c7-left-notice-20261022/dinput8.dll'
freeze.LABEL='C7'
freeze.SOURCE_TAG='c7'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c7-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(
    status='private_c7_notice_layout_pending_live_acceptance',
    runtime_build='horizon-complete-20261022-c7-left-notice',
    candidate_resources='C6 resources unchanged; scoped original GUI Layout/Size/transform setters in DLL',
    live_acceptance='User accepted C6 tests except notice body alignment, frame size and button separation. Final actual delivery installation/uninstallation remains separate.',
    first_entry_notice='Six unchanged pages; native large frame with owned 900x750 local layout, 800x580 left body, separate button row; native parent viewport mapping; exact property restoration; C7 acknowledgement',
    remaining_project_items=['C7 six-page notice left alignment, complete text and separate button at actual resolutions',
                             'Final actual install/uninstall and portable native delivery'])
if __name__=='__main__':freeze.main()
