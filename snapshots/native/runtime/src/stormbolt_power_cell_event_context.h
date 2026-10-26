#pragma once
#include "stormbolt_shell_hook.h"

namespace stormslinger {
struct StormboltPowerCellEventMetadata {
    float scalar=0.0f;
    std::uint32_t code=0;
};
// Native event boundary supplies a borrowed, initialized cAttackParam family
// pointer at event+B8. This reads proven fields, not the result object's type.
inline bool read_stormbolt_power_cell_event_metadata(const void* event,
    StormboltPowerCellEventMetadata& output) {
    std::uintptr_t metadata=0;
    StormboltPowerCellEventMetadata candidate{};
    if (!event || !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(event),0xB8,metadata) ||
        !metadata || !read_stormbolt_field(metadata,0x20,candidate.scalar) ||
        !read_stormbolt_field(metadata,0xA0,candidate.code)) return false;
    output=candidate;
    return true;
}
inline bool stormbolt_power_cell_metadata_allows_growth(
    const StormboltPowerCellEventMetadata& metadata) noexcept {
    // PS VUCOMISS zero,scalar / JB skips exclusion for positive OR unordered.
    return !(metadata.scalar<=0.0f && (metadata.code|0x20u)==0x27u);
}
struct StormboltPowerCellEventContext {
    void* owner=nullptr;
    void* event_actor=nullptr;
    std::uint64_t event_actor_flags=0;
    float amount=0.0f;
    std::uint32_t result_type=0;
    bool owner_enabled=false;
};
// PC1219308/34/53/58 and1219412/44: authenticated component/event/result
// arguments at the synchronous event boundary. This gathers proven shared
// inputs only; it does not mean that the PS PowerCell growth gates passed.
inline bool read_stormbolt_power_cell_event_context(void* component,const void* event,
    const void* result,void* (*resolveActor)(const void*),StormboltPowerCellEventContext& output) {
    if (!component || !event || !result || !resolveActor) return false;
    StormboltPowerCellEventContext candidate{};
    std::uintptr_t owner=0;
    std::uint8_t enabled=0;
    if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(component),0x60,owner) ||
        !owner || !read_stormbolt_field(owner,0x8938,enabled) ||
        !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(result),0,candidate.amount) ||
        !read_stormbolt_field(reinterpret_cast<std::uintptr_t>(result),0x10,candidate.result_type)) return false;
    candidate.owner=reinterpret_cast<void*>(owner);
    candidate.owner_enabled=(enabled&1)!=0;
    candidate.event_actor=resolveActor(event);
    if (!candidate.event_actor || !read_stormbolt_field(
        reinterpret_cast<std::uintptr_t>(candidate.event_actor),0x60,candidate.event_actor_flags)) return false;
    output=candidate;
    return true;
}
} // namespace stormslinger
