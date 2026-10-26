#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stormslinger {
struct StormboltDecayEffectApi {
    bool (*active)(const void*)=nullptr;
    void* (*request)(void*,std::uint32_t,std::uint32_t,std::uint32_t)=nullptr; // AE2910, raw result.
    void* (*assign)(void*,void*)=nullptr;
};
// PS4997AF1..7B74 / 7BCF..7C52 only. Caller selects this continuation,
// reloads the owner/component after preceding cleanup, and owns native/resource
// lifetimes. Subsequent UI/weapon effects and timers are separate work.
inline bool stormbolt_update_decay_effect(void* component,std::uint32_t stage,
    const StormboltDecayEffectApi& api) {
    if (!stage) return true;
    if (!component || !api.active || !api.request || !api.assign) return false;
    std::byte* buffer{};
    std::memcpy(&buffer,static_cast<std::byte*>(component)+8,sizeof(buffer));
    if (!buffer) return false;
    if (api.active(buffer+0x1980)) return true;
    const std::uint32_t record=stage>=3 && stage<=5 ? 1u : stage>=6 && stage<=8 ? 2u : 0u;
    auto* result=api.request(component,0x7DB,record,0xFFFFFFFF);
    std::memcpy(&buffer,static_cast<std::byte*>(component)+8,sizeof(buffer));
    if (!buffer) return false; // Creation already occurred; caller must not replay.
    api.assign(buffer+0x1980,result); // Native null result is assigned unchanged.
    return true;
}
} // namespace stormslinger
