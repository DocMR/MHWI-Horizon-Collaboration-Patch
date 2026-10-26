#pragma once
#include "stormbolt_decay_prefix.h"
#include "stormbolt_decay_owner_cleanup.h"
#include "stormbolt_local_decay_timer.h"
#include "stormbolt_effect_fade.h"
#include "stormbolt_attachment_update.h"

namespace stormslinger {
// Selected PS decay route only: remote768A or local785D. This does not select
// outer authority/action/input gates or execute the ordinary weapon update.
template<std::size_t Capacity>
bool stormbolt_run_decay_route(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_component,bool local,bool rising,
    std::uint32_t base_color,const StormboltCleanupBankApi& cleanup,
    const StormboltDecayEffectApi& effect,const StormboltDecayOwnerApi& owner,
    const StormboltEffectColorApi& color) {
    const auto result=stormbolt_decay_prefix(store,token,initial_component,cleanup,effect);
    if (result==StormboltDecayPrefixResult::invalidated) return false;
    if (result==StormboltDecayPrefixResult::continue_after_effect &&
        !stormbolt_cleanup_decay_latch(store,token,owner,cleanup)) return false;
    if (!local) return true;
    if (!stormbolt_reload_local_decay_action_timer(store,token)) return false;
    return stormbolt_update_effect_fade(store,token,rising,base_color,color);
}
// PS remote decay and local fade both converge on70B0/70BE attachment tail.
// Failure stops at its point of occurrence: never replay native effect operations.
template<std::size_t Capacity>
bool stormbolt_run_decay_with_attachment(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_component,bool local,bool rising,
    std::uint32_t base_color,const StormboltCleanupBankApi& cleanup,
    const StormboltDecayEffectApi& effect,const StormboltDecayOwnerApi& owner,
    const StormboltEffectColorApi& color,const StormboltMaterialColorApi& material,
    const std::array<float,4>& low_hlsa,const std::array<float,4>& high_hlsa) {
    if (!stormbolt_run_decay_route(store,token,initial_component,local,rising,
                                  base_color,cleanup,effect,owner,color)) return false;
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
