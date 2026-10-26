#pragma once
#include <Windows.h>
#include <cstdint>
#include <limits>
#include <type_traits>
namespace stormslinger {
// Readability alone is not native lifetime; callers still borrow live objects.
template<class T>
inline bool read_stormbolt_field(std::uintptr_t base,std::size_t offset,T& result) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    if(!base || offset>maximum-base || sizeof(T)>maximum-(base+offset)) return false;
    T copy{};SIZE_T bytes=0;
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(base+offset),&copy,sizeof(copy),&bytes) || bytes!=sizeof(copy)) return false;
    result=copy;return true;
}
inline bool stormbolt_native_span(std::uintptr_t base,std::size_t offset,std::size_t size,bool writable=false) noexcept {
    const auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    if(!base || !size || offset>maximum-base || size>maximum-(base+offset)) return false;
    auto current=base+offset;const auto end=current+size;
    while(current<end) {
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<const void*>(current),&info,sizeof(info)) ||
            info.State!=MEM_COMMIT || (info.Protect&PAGE_GUARD)) return false;
        const auto access=info.Protect&0xFF;
        const bool write=access==PAGE_READWRITE || access==PAGE_WRITECOPY || access==PAGE_EXECUTE_READWRITE || access==PAGE_EXECUTE_WRITECOPY;
        const bool read=write || access==PAGE_READONLY || access==PAGE_EXECUTE_READ;
        if(!read || (writable && !write)) return false;
        const auto address=reinterpret_cast<std::uintptr_t>(info.BaseAddress);
        if(info.RegionSize>maximum-address || address+info.RegionSize<=current) return false;
        current=address+info.RegionSize;
    }
    return true;
}
} // namespace stormslinger
