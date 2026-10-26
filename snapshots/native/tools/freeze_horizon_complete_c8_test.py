"""Freeze the drawing-node coordinate correction; no installation."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c7-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c8-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c8-text-anchor-20261022/dinput8.dll'
freeze.LABEL='C8'
freeze.SOURCE_TAG='c8'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c8-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(
    status='private_c8_notice_anchor_pending_live_acceptance',
    runtime_build='horizon-complete-20261022-c8-text-anchor',
    candidate_resources='C7 resources unchanged; correct drawing-node position and ControlPoint setters in DLL',
    live_acceptance='C6 other tests accepted by user. C7 screenshot shows larger frame/separate button but body still central. C8 visual acceptance pending.',
    first_entry_notice='Six unchanged pages; native large frame900x750, body800x580, drawing-node upper-left ControlPoint0 and PosX/Y -400/-320, separate button315; exact restoration; C8 acknowledgement',
    remaining_project_items=['C8 six-page body anchored to left margin and full text/button visibility',
                             'Final actual install/uninstall and portable native delivery'])
if __name__=='__main__':freeze.main()
