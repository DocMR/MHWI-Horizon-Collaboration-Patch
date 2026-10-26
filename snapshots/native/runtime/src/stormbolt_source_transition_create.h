#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stormslinger {
// SetBomb F03CA0 allocator ABI: heap, size, alignment, DTI+34, zero, zero, DTI+8.
struct StormboltSourceTransitionCreateApi {
    void* type=nullptr;
    void* (*allocator_for)(void*)=nullptr; // 216F3F0, same DTI as destruction.
    void* (*allocate)(void*,std::size_t,std::uint32_t,std::uint32_t,
        std::uint32_t,std::uint32_t,const void*)=nullptr; // heap virtual38.
    void* (*place)(void*,void*)=nullptr; // authenticated original SetBomb F03D20.
};
inline constexpr std::size_t stormbolt_source_transition_bytes=0x38;

// Only storage whose actual allocation size is established by the caller.
// Do not promote arbitrary untracked storage to a registered sidecar instance.
inline void* stormbolt_place_source_transition(const StormboltSourceTransitionCreateApi& api,
    void* storage,std::size_t bytes) {
    if (!storage || bytes<stormbolt_source_transition_bytes || !api.type || !api.place)
        return nullptr;
    auto* result=api.place(api.type,storage);
    if (!result) return nullptr;
    const std::uint32_t command=0x20,secondary=0xFFFF;
    const std::uint8_t option=0;
    auto* object=static_cast<std::byte*>(result);
    std::memcpy(object+0x28,&command,sizeof(command));
    std::memcpy(object+0x2C,&secondary,sizeof(secondary));
    std::memcpy(object+0x30,&option,sizeof(option));
    return result;
}

// Dedicated SetBomb DTI virtual8 creation path, F03CA0 and PS272CD90 defaults.
// Bootstrap authenticates type, allocator ABI and placement return identity;
// it must establish quiescence before replacing the DTI slot. This component
// neither changes reflection metadata nor installs the transition predicate.
inline void* stormbolt_create_source_transition(const StormboltSourceTransitionCreateApi& api) {
    if (!api.type || !api.allocator_for || !api.allocate || !api.place) return nullptr;
    auto* heap=api.allocator_for(api.type);
    const auto* type=static_cast<const std::byte*>(api.type);
    std::uint32_t tag{};
    const void* label{};
    std::memcpy(&tag,type+0x34,sizeof(tag));
    std::memcpy(&label,type+8,sizeof(label));
    auto* storage=api.allocate(heap,stormbolt_source_transition_bytes,0x10,tag,0,0,label);
    if (!storage) return nullptr;
    return stormbolt_place_source_transition(api,storage,stormbolt_source_transition_bytes);
}
} // namespace stormslinger
