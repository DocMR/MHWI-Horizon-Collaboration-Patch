#pragma once
#include "prepared_text_pool.h"

namespace stormslinger {
// These function pointers must be bound only after resource/type/code validation.
// GMD allocation slot 15 consumes a DWORD size; release slot 16 consumes a pointer.
struct GmdTextAllocatorContext {
    void* resource = nullptr;
    void* (*allocate)(void*, std::uint32_t) noexcept = nullptr;
    void (*release)(void*, void*) noexcept = nullptr;
};
inline bool valid_gmd_allocation_size(std::size_t size) noexcept {
    return size != 0 && size <= std::numeric_limits<std::uint32_t>::max();
}
inline void* allocate_gmd_text(void* opaque, std::size_t size) noexcept {
    const auto* context = static_cast<const GmdTextAllocatorContext*>(opaque);
    if (!context || !context->resource || !context->allocate || !context->release ||
        !valid_gmd_allocation_size(size)) return nullptr;
    return context->allocate(context->resource, static_cast<std::uint32_t>(size));
}
inline void release_gmd_text(void* opaque, void* pointer) noexcept {
    const auto* context = static_cast<const GmdTextAllocatorContext*>(opaque);
    if (pointer && context && context->resource && context->release)
        context->release(context->resource, pointer);
}
inline TextAllocator make_gmd_text_allocator(GmdTextAllocatorContext& context) noexcept {
    if (!context.resource || !context.allocate || !context.release) return {};
    return {&context, allocate_gmd_text, release_gmd_text};
}
} // namespace stormslinger
