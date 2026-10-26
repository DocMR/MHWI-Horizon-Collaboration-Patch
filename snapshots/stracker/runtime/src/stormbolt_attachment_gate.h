#pragma once
#include <cstdint>
#include <cstring>

namespace stormslinger {
enum class StormboltAttachmentGate { unavailable, inactive, ready };
// PS49970B0..70ED mapped through PC2115AD0 and model material population.
// Borrowed weapon lifetime; output remains unchanged unless ready.
inline StormboltAttachmentGate stormbolt_select_attachment_material(void* weapon,void*& output) noexcept {
    if (!weapon) return StormboltAttachmentGate::unavailable;
    const auto* p=static_cast<const unsigned char*>(weapon);
    std::uint32_t category{},model{},count{};
    std::memcpy(&category,p+0x9F8,4);
    if(category!=13) return StormboltAttachmentGate::inactive;
    std::memcpy(&model,p+0x1D00,4);
    if(model!=24) return StormboltAttachmentGate::inactive;
    std::memcpy(&count,p+0x2B8,4);
    if(!count) return StormboltAttachmentGate::inactive;
    void* array{};std::memcpy(&array,p+0x2B0,sizeof(array));
    if(!array) return StormboltAttachmentGate::unavailable;
    void* first{};std::memcpy(&first,array,sizeof(first));
    if(!first) return StormboltAttachmentGate::inactive;
    output=first;
    return StormboltAttachmentGate::ready;
}
} // namespace stormslinger
