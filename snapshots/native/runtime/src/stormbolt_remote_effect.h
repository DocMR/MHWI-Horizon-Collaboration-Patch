#pragma once
#include "stormbolt_decay_effect.h"
#include "stormbolt_attachment_update.h"

namespace stormslinger {
// PS4997494..74BD selects records; 4997A53..7ACD creates the active remote effect.
// Attachment parameter C8/C9/CA must be selected by the preceding native path.
inline bool stormbolt_update_remote_effect(void* component,std::uint32_t stage,
    std::uint32_t attachment,const StormboltDecayEffectApi& api,bool* created=nullptr) {
    if(created) *created=false;
    if(!component || !api.active || !api.request || !api.assign || attachment<0xC8 || attachment>0xCA) return false;
    std::byte* buffer{};
    std::memcpy(&buffer,static_cast<std::byte*>(component)+8,sizeof(buffer));
    if(!buffer) return false;
    if(api.active(buffer+0x19E0)) return true;
    const std::uint32_t record=stage>=1 && stage<=2 ? 1u : stage>=3 && stage<=5 ? 2u : stage>=6 && stage<=8 ? 3u : 0u;
    void* effect=api.request(component,0x7DC,record,attachment);
    std::memcpy(&buffer,static_cast<std::byte*>(component)+8,sizeof(buffer));
    if(!buffer) return false;
    api.assign(buffer+0x19E0,effect);
    if(created) *created=true;
    return true;
}
// Selected remote-active branch only. Capture stage before the native selector,
// and retain the caller's component even if native dispatch changes the owner.
template<std::size_t Capacity>
bool stormbolt_run_remote_effect(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* initial_component,
    std::uint32_t (*select_attachment)(void*),const StormboltDecayEffectApi& effect,
    const StormboltMaterialColorApi& material,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa) {
    StormboltStageState captured{},after{};
    if(!initial_component || !select_attachment || !store.snapshot(token,captured)) return false;
    const auto attachment=select_attachment(reinterpret_cast<void*>(token.address));
    if(!store.snapshot(token,after)) return false;
    if(!stormbolt_update_remote_effect(initial_component,captured.counter,attachment,effect) ||
       !store.snapshot(token,after)) return false;
    return stormbolt_run_attachment_tail(store,token,low_hlsa,high_hlsa,material);
}
} // namespace stormslinger
