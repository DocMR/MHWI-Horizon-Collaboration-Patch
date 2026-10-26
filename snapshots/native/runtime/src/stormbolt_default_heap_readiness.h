#pragma once
#include "stormbolt_native_memory.h"

namespace stormslinger {
struct StormboltDefaultHeapBindings {
    std::uintptr_t heap_slots=0,vtable=0,allocate=0,release=0;
    std::uintptr_t typed_vtable=0,typed_allocate=0,typed_release=0;
};
struct StormboltDefaultHeapSelection {
    void* heap=nullptr;
    unsigned index=0;
    std::uintptr_t allocate=0,release=0;
};
// A synchronous native borrow must cover this observation and subsequent use.
// This identifies the audited default profile; it does not initialize its lock,
// establish heap ownership, or certify a specialized allocator profile.
inline bool stormbolt_select_default_heap(const StormboltDefaultHeapBindings& api,
    const void* type,StormboltDefaultHeapSelection& output) noexcept {
    if(!api.heap_slots || !api.vtable || !api.allocate || !api.release) return false;
    std::uint32_t packed=0;
    if(!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(type),0x30,packed)) return false;
    const unsigned index=(packed>>23)&0x3F;
    std::uintptr_t heap=0,table=0,allocate=0,release=0;
    if(!read_stormbolt_field(api.heap_slots,index*sizeof(void*),heap) ||
        !read_stormbolt_field(heap,0,table) ||
        !read_stormbolt_field(table,0x38,allocate) ||
        !read_stormbolt_field(table,0x68,release)) return false;
    const bool standard=table==api.vtable && allocate==api.allocate && release==api.release;
    const bool typed=api.typed_vtable && api.typed_allocate && api.typed_release &&
        table==api.typed_vtable && allocate==api.typed_allocate && release==api.typed_release;
    if((!standard && !typed) || !stormbolt_native_span(heap,0,typed?0x650:0x88,true)) return false;
    output={reinterpret_cast<void*>(heap),index,allocate,release};
    return true;
}
using StormboltDefaultHeapAllocate=void*(*)(void*,std::size_t,std::uint32_t,std::uint32_t,
    std::uint32_t,std::uint32_t,const void*);
// Recheck selection at use, so startup observation cannot authorize a different
// heap/profile later. The native callback borrow still covers allocation.
inline void* stormbolt_allocate_default_heap(const StormboltDefaultHeapBindings& api,
    const void* type,void* heap,std::size_t bytes,std::uint32_t alignment,std::uint32_t tag,
    std::uint32_t a,std::uint32_t b,const void* label) {
    StormboltDefaultHeapSelection selected{};
    if(!stormbolt_select_default_heap(api,type,selected) || selected.heap!=heap) return nullptr;
    return reinterpret_cast<StormboltDefaultHeapAllocate>(selected.allocate)(heap,bytes,alignment,tag,a,b,label);
}
}
