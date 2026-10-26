#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stormslinger {
struct StormboltCleanupBankApi {
    bool (*active)(const void*)=nullptr;
    void (*stop)(void*)=nullptr;
    void* (*assign)(void*,void*)=nullptr;
};
// One PS cleanup bank only. Caller selects 1980, 19E0 or 1AA0, owns component/buffer
// lifetimes, and serializes effect access. Outer owner rereads remain caller work.
inline bool stormbolt_cleanup_bank(void* component,std::size_t offset,const StormboltCleanupBankApi& api,bool* cleared=nullptr) {
    if (cleared) *cleared=false;
    if (!component || !api.active || !api.stop || !api.assign || (offset!=0x1980 && offset!=0x19E0 && offset!=0x1AA0)) return false;
    std::byte* buffer{};
    std::memcpy(&buffer,static_cast<const std::byte*>(component)+8,sizeof(buffer));
    if (!buffer) return false;
    if (!api.active(buffer+offset)) return true;
    void* first{}; std::memcpy(&first,buffer+offset,sizeof(first));
    std::uint8_t flags{};
    if (first) std::memcpy(&flags,static_cast<const std::byte*>(first)+0xC,1);
    if (flags&0xE) {
        api.stop(first);
        std::memcpy(&buffer,static_cast<const std::byte*>(component)+8,sizeof(buffer));
        if (!buffer) return false;
    }
    api.assign(buffer+offset,nullptr);
    if (cleared) *cleared=true;
    return true;
}
// PS4997179..7276, stopping before the sidecar flag/timer writes. Initial actor
// is the one selected by the outer update, not an unconditional owner reread.
inline bool stormbolt_cleanup_stage_effects(void* weapon,void* actor,const StormboltCleanupBankApi& api) {
    if (!weapon || !actor) return false;
    void* component{};
    std::memcpy(&component,static_cast<const std::byte*>(actor)+0x8808,sizeof(component));
    bool cleared=false;
    if (!stormbolt_cleanup_bank(component,0x19E0,api,&cleared)) return false;
    if (cleared) {
        std::memcpy(&actor,static_cast<const std::byte*>(weapon)+0x9B0,sizeof(actor));
        if (!actor) return false;
        std::memcpy(&component,static_cast<const std::byte*>(actor)+0x8808,sizeof(component));
    }
    return stormbolt_cleanup_bank(component,0x1AA0,api);
}
} // namespace stormslinger
