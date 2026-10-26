#pragma once
#include "horizon_module_prepare.h"
#include "horizon_mapping_transaction.h"
#include "horizon_tutorial_trigger.h"
#include "horizon_shield_multiplier_call.h"
#include "horizon_shield_reaction_call.h"
#include "horizon_aloy_voice_call.h"
#include "horizon_notice_native.h"
namespace horizon_other {
// Full shooting already owns the shared status/timer/growth/loop/sync/remote
// sites and stores. This participant adds only content hooks and the two
// equipment-specific sites, never a second ShieldEquipmentSession.
template<std::size_t Capacity>
class CompleteContentSession {
    const std::uint8_t* image_;
    stormslinger::HookTransaction* tx_=nullptr;
    const PreparedQuestMapping* mapping_=nullptr;
    QuestMappingTransaction tutorial_menu_;
    TutorialTriggerCall tutorial_;
    AloyVoiceCall voice_;
    NoticeNativeSession notice_;
    ShieldMultiplierCall<Capacity> multiplier_;
    ShieldReactionCall<Capacity> reaction_;
    bool attempted_=false,restored_=false;
public:
    CompleteContentSession(const std::uint8_t* image,
        stormslinger::StormboltPowerCellStore<Capacity>& statuses,
        const stormslinger::StormboltLoopInputBindings& inputs,
        stormslinger::StormboltSkillValueQuery query)
        :image_(image),voice_(image),multiplier_(image,statuses,inputs,query),reaction_(image,statuses,inputs,query) {}
    bool prepare(stormslinger::HookTransaction& tx,const ModulePrepareResult& prepared) {
        if(attempted_ || !tx.can_add_hooks() || prepared.stage!=ModulePrepareStage::queued ||
            !prepared.mapping_owned_by_shared_host || !prepared.prepared_mapping ||
            prepared.prepared_mapping->image()!=image_)return false;
        attempted_=true;tx_=&tx;mapping_=prepared.prepared_mapping;
        const auto& mapping=*mapping_;
        return notice_.prepare_while_quiescent(image_,tx) && voice_.prepare_while_quiescent(tx) && tutorial_.prepare_while_quiescent(image_,tx) &&
            tutorial_menu_.install_while_quiescent(const_cast<std::uint8_t*>(image_),mapping,false)==MappingInstallResult::installed &&
            multiplier_.prepare_while_quiescent(tx) && reaction_.prepare_while_quiescent(tx);
    }
    bool activate_after_shared_commit() {
        return attempted_ && !restored_ && tx_ && tx_->last.active &&
            quest_mapping_references_active(*mapping_) &&
            notice_.activate_after_shared_commit() &&
            voice_.activate_after_shared_commit_while_quiescent() &&
            tutorial_.activate_after_shared_commit_while_quiescent() &&
            multiplier_.activate_after_shared_commit_while_quiescent() &&
            reaction_.activate_after_shared_commit_while_quiescent();
    }
    bool restore_before_shared_stop() {
        if(!attempted_)return true;
        if(!notice_.restore_while_quiescent() || !voice_.restore_while_quiescent() || !reaction_.restore_while_quiescent() || !multiplier_.restore_while_quiescent() ||
            !tutorial_.restore_while_quiescent())return false;
        restored_=true;return true;
    }
    bool release_after_shared_stop() {
        if(!attempted_)return true;
        if(!restored_ || !tx_ || tx_->has_library_state())return false;
        return notice_.release_after_shared_stop() && voice_.release_after_shared_stop() && reaction_.release_after_shared_stop() && multiplier_.release_after_shared_stop() &&
            tutorial_.release_after_shared_stop_while_quiescent() && tutorial_menu_.restore_while_quiescent();
    }
};
}
