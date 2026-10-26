"""Freeze native large notice and scoped button font repair; no installation."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c5-20261022/package'
freeze.WORK=freeze.BASE/'payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c6-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c6-native-large-notice-20261022/dinput8.dll'
freeze.LABEL='C6'
freeze.SOURCE_TAG='c6'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c6-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(status='private_c6_native_large_notice_pending_live_acceptance',runtime_build='horizon-complete-20261022-c6-native-large-notice',candidate_resources='C5 resources unchanged; native large notice and button font repair in DLL',live_acceptance='User accepted C5 items except notice defects and untested Palico equipment, Shield-Weaver mechanisms, logging/feedback; final delivery acceptance remains separate.',first_entry_notice='Six original pages; native windowG flag; localized button borrows body font and restores it; no manual node scale/position changes; C6 acknowledgement',remaining_project_items=['C6 native large notice layout/button/language acceptance','Palico equipment acceptance','Shield-Weaver mechanism acceptance','Logging/feedback acceptance','Final actual install/uninstall and portable native delivery'])
if __name__=='__main__':freeze.main()
