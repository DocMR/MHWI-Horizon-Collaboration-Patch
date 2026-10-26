#pragma once
#include "../../../runtime/src/stormbolt_power_cell_event_eligibility_adapter.h"
#include "../../../runtime/src/stormbolt_power_cell_timer_adapter.h"

namespace horizon_other {
struct ShieldStatusBindings {
    stormslinger::StormboltLoopInputBindings inputs{};
    stormslinger::StormboltSkillValueQuery query = nullptr;
    stormslinger::StormboltSkillPrompt prompt = nullptr;
    void (*actor_flag)(void*, std::uint32_t) = nullptr;
};
// Equipment-only adapter: no weapon object, hook registration or private store.
// The unified host owns one status store and its native constructor/reset/destroy
// hooks. Both modules must borrow that SAME store, including generation tokens.
// Host certifies APIs/types, borrowed lifetimes, native call boundaries and thread
// serialization. Do not call this as well as an existing dispatch at the same site.
template<std::size_t Capacity>
class ShieldStatusAdapter {
    stormslinger::StormboltPowerCellStore<Capacity>& shared_;
    ShieldStatusBindings bindings_;
public:
    ShieldStatusAdapter(stormslinger::StormboltPowerCellStore<Capacity>& shared,
                        const ShieldStatusBindings& bindings)
        : shared_(shared), bindings_(bindings) {}
    ShieldStatusAdapter(const ShieldStatusAdapter&) = delete;
    ShieldStatusAdapter& operator=(const ShieldStatusAdapter&) = delete;
    bool update_eligibility(void* status) {
        return stormslinger::update_stormbolt_power_cell_event_eligibility_from_status(
            status, shared_, bindings_.inputs, bindings_.query, bindings_.actor_flag);
    }
    // native_step is the cached native update step; no assumed time units.
    bool advance_timer(void* status, float native_step) {
        return stormslinger::advance_stormbolt_power_cell_timer_from_status(
            status, native_step, shared_, bindings_.inputs, bindings_.query, bindings_.prompt);
    }
};
}
