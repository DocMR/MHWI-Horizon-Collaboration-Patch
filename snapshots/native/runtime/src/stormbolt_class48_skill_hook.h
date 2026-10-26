#pragma once
#include "stormbolt_class48_skill_adapter.h"
#include "hooks.h"

namespace stormslinger {
using StormboltNativeSkillMultiplier=float (*)(void*,float,std::uint32_t);
template<std::size_t S>
class StormboltClass48SkillHook {
    StormboltPowerCellStore<S>& statuses_;
    const StormboltClass48SkillBindings bindings_;
    StormboltNativeSkillMultiplier original_=nullptr;
    inline static StormboltClass48SkillHook* active_=nullptr;
    static float callback(void* status,float input,std::uint32_t classification) {
        auto& hook=*active_;
        const float native=hook.original_(status,input,classification);
        float result=native;
        (void)apply_stormbolt_class48_skill(status,classification,native,hook.statuses_,hook.bindings_,result);
        return result;
    }
public:
    StormboltClass48SkillHook(StormboltPowerCellStore<S>& statuses,const StormboltClass48SkillBindings& bindings)
        :statuses_(statuses),bindings_(bindings) {}
    bool owns_bridge() const noexcept {return active_==this;}
    MH_STATUS add_to_transaction(HookTransaction& transaction,void* entry) {
        if(active_) return MH_ERROR_ALREADY_CREATED;
        if(!entry || !bindings_.query || !bindings_.status_vtable) return MH_ERROR_NOT_EXECUTABLE;
        active_=this;
        return transaction.add(entry,reinterpret_cast<void*>(&callback),reinterpret_cast<void**>(&original_));
    }
    // Native execution must be quiescent and the shared transaction stopped.
    bool release_after_unhook() {
        if(active_!=this) return false;
        active_=nullptr;original_=nullptr;return true;
    }
};
} // namespace stormslinger
