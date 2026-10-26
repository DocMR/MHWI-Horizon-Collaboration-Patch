#pragma once
#include "stormbolt_input_window.h"
#include "stormbolt_update_gate.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
enum class StormboltLocalRoute { fade_only, decay, trigger };
struct StormboltLocalSelection {
    StormboltLocalRoute route=StormboltLocalRoute::fade_only;
    std::uint8_t held=0;
};
// Selected PS7516 ->780E prefix. group is EDI retained by the outer action
// lookup; index is read later from the retained actor only when needed.
template<std::size_t Capacity>
bool stormbolt_select_local_input(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* actor,std::uint32_t group,
    const StormboltLocalInputApi& input,StormboltLocalSelection& output) {
    StormboltLocalSelection selected{};
    if(!store.transact(token,[&](auto& state) noexcept {
        float step{};
        std::memcpy(&step,reinterpret_cast<const std::byte*>(token.address)+0x68,4);
        selected.held=advance_stormbolt_input_window(state,step);
        return true;
    })) return false;
    std::uint8_t alive{};
    if(actor) std::memcpy(&alive,static_cast<const std::byte*>(actor)+0xC,1);
    if(actor && (alive&0xE)) {
        std::uint32_t flags{};
        if(!stormbolt_read_local_input_flags(actor,input,flags)) return false;
        StormboltStageState current{};
        if(!store.snapshot(token,current)) return false;
        std::uint32_t index=0;
        if((flags&0x01000000u) && group==3)
            std::memcpy(&index,static_cast<const std::byte*>(actor)+0x6278,4);
        selected.route=stormbolt_local_input_path(flags,group,index)==StormboltLocalInputPath::decay_branch
            ? StormboltLocalRoute::decay : StormboltLocalRoute::trigger;
    }
    output=selected;
    return true;
}
} // namespace stormslinger
