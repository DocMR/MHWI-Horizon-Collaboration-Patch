"""Freeze owned guildcard list repair and restored large first-entry notice."""
import freeze_horizon_complete_c3_test as freeze
freeze.BASE=freeze.ROOT/'staging/Horizon-complete-cumulative-c4-20261022/package'
freeze.WORK=freeze.ROOT/'staging/Horizon-complete-c5-work-20261022/payload'
freeze.OUT=freeze.ROOT/'staging/Horizon-complete-cumulative-c5-20261022'
freeze.DLL=freeze.ROOT/'runtime/ninja-horizon-complete-c5-guildcard-notice-20261022/dinput8.dll'
freeze.LABEL='C5'
freeze.SOURCE_TAG='c5'
freeze.BUILD_SCRIPT=freeze.ROOT/'tools/build-horizon-complete-c5-runtime-20261022.cmd'
freeze.EXTRA_MANIFEST=dict(status='private_c5_guildcard_list_notice_pending_live_acceptance',runtime_build='horizon-complete-20261022-c5-guildcard-notice',candidate_resources='C4 plus two PC GCOD tables with only eleven owned platform fields normalized',live_acceptance='C4 Aloy armor and layered wear/load passed user feedback; guildcard listing failed despite open bits true. C5 not installed.',first_entry_notice='Restored per latest user request: six native pages, larger layout, same-language close button, C5 acknowledgement',remaining_project_items=['C5 list/pose/title selection and persistence acceptance','C5 larger notice visual acceptance','C4 remaining departure/menu/save acceptance and Shield-Weaver mechanics','Remaining full-content acceptance and final cross-PC delivery'])
if __name__=='__main__':freeze.main()
