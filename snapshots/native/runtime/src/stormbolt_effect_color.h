#pragma once
#include "stormbolt_effect_alpha.h"
#include <cstring>

namespace stormslinger {
inline constexpr std::uint32_t ps_stormbolt_color_default=0xFFFF8214u; // PS499907C initialization.
struct StormboltEffectColorApi {
    bool (*active)(const void*)=nullptr;
    void (*set_color)(void*,const std::uint32_t*)=nullptr; // PC22F9B70.
};
// PS4997ED4..7F67, after the alpha update. Caller supplies the current base
// color and borrows component/effect lifetimes. Only the first reference is
// written, even though active checks the whole four-reference bank.
inline bool stormbolt_write_effect_color(void* component,float alpha,std::uint32_t base_color,
    const StormboltEffectColorApi& api) {
    if (!component || !api.active || !api.set_color) return false;
    std::byte* buffer{};std::memcpy(&buffer,static_cast<std::byte*>(component)+8,sizeof(buffer));
    if (!buffer) return false;
    if (!api.active(buffer+0x1AA0)) return true;
    void* first{};std::memcpy(&first,buffer+0x1AA0,sizeof(first));
    if (!first) return false;
    std::uint8_t flags{};std::memcpy(&flags,static_cast<std::byte*>(first)+0xC,1);
    if (!(flags&0xE)) return false; // Inconsistent bank, no substitute reference.
    const auto color=stormbolt_pack_effect_color(alpha,base_color);
    api.set_color(first,&color);
    return true;
}
} // namespace stormslinger
