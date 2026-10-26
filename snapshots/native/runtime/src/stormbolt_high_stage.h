#pragma once
#include "stormbolt_cleanup_bank.h"
#include "stormbolt_attachment_update.h"

namespace stormslinger {
// Selected PS4996FE9..7085 only, after the outer gates select counter>=9.
// initial_local is the retained actor flag; reread owner only after actual cleanup.
template<std::size_t Capacity>
bool stormbolt_run_high_stage(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_component,bool initial_local,
    const StormboltCleanupBankApi& cleanup,const StormboltMaterialColorApi& material,
    const std::array<float,4>& low_hlsa,const std::array<float,4>& high_hlsa) {
    StormboltStageState current{};
    if (!store.snapshot(token,current) || current.counter<9) return false;
    bool cleared=false;
    if (!stormbolt_cleanup_bank(initial_component,0x19E0,cleanup,&cleared) ||
        !store.snapshot(token,current)) return false;
    bool local=initial_local;
    if(cleared) {
        void* owner{};
        std::memcpy(&owner,reinterpret_cast<const std::byte*>(token.address)+0x9B0,sizeof(owner));
        if(!owner) return false;
        std::uint32_t flags{};
        std::memcpy(&flags,static_cast<const std::byte*>(owner)+0x8938,4);
        local=(flags&1)!=0;
    }
    if(!local && !store.transact(token,[](auto& state) noexcept {
        state.input_effect_active=0;return true;
    })) return false;
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
