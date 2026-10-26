#pragma once
#include "owned_code_word.h"
#include <array>
#include <cstring>
#include <limits>

namespace stormslinger {
template<std::size_t Size>
inline bool build_stormbolt_loop_call(std::uintptr_t site,std::uintptr_t entry,
                                      std::array<unsigned char,Size>& output) noexcept {
    static_assert(Size==5 || Size==6 || Size==7 || Size==8 || Size==9 || Size==12);
    if (!site || !entry || site>(std::numeric_limits<std::uintptr_t>::max)()-5) return false;
    const auto next=site+5;
    const bool forward=entry>=next;
    const auto distance=forward ? entry-next : next-entry;
    if (distance>(forward ? 0x7FFFFFFFull : 0x80000000ull)) return false;
    const auto displacement=static_cast<std::uint32_t>(forward ? distance : 0-distance);
    std::array<unsigned char,Size> candidate{};
    candidate.fill(0x90);candidate[0]=0xE8;
    std::memcpy(candidate.data()+1,&displacement,4);
    output=candidate;
    return true;
}
// Install/remove only while execution and other writers are quiescent.
// The verified site lies entirely in one protection region.
template<class Api=CodeWordApi,std::size_t Size=7>
class StormboltLoopCallPatch {
    static_assert(Size==5 || Size==6 || Size==7 || Size==8 || Size==9 || Size==12);
    static constexpr std::array<unsigned char,Size> default_expected() {
        if constexpr (Size==5) return {};
        else if constexpr (Size==6) return {{0x8B,0x87,0x98,0x59,0,0}};
        else if constexpr (Size==7) return {{0x80,0xBF,0xB8,0x09,0,0,0}};
        else if constexpr (Size==8) return {{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0}};
        else if constexpr (Size==9) return {{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76}};
        else return {{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0}};
    }
    void* site_=nullptr;
    std::array<unsigned char,Size> before_{},after_{};
    DWORD protection_=0;
    bool owned_=false,protectPending_=false,flushPending_=false;
    bool maintain() {
        if (flushPending_) {
            if (!Api::flush(site_,Size)) return false;
            flushPending_=false;
        }
        if (protectPending_) {
            DWORD unused=0;
            if (!Api::protect(site_,Size,protection_,&unused)) return false;
            protectPending_=false;
        }
        return true;
    }
    bool write(const std::array<unsigned char,Size>& expected,
               const std::array<unsigned char,Size>& desired,bool installed) {
        if (!maintain()) return false;
        std::array<unsigned char,Size> live{};
        SIZE_T count=0;
        if (!ReadProcessMemory(GetCurrentProcess(),site_,live.data(),Size,&count) || count!=Size || live!=expected)
            return false;
        if (!Api::protect(site_,Size,PAGE_EXECUTE_READWRITE,&protection_)) return false;
        protectPending_=true;
        std::memcpy(site_,desired.data(),Size);
        owned_=installed;
        flushPending_=true;
        return maintain();
    }
public:
    bool active() const noexcept { return owned_ || protectPending_ || flushPending_; }
    bool install(void* site,std::uintptr_t entry,const std::array<unsigned char,Size>& expected=
        default_expected()) {
        if (active() || !site) return false;
        std::array<unsigned char,Size> call{};
        if (!build_stormbolt_loop_call(reinterpret_cast<std::uintptr_t>(site),entry,call)) return false;
        MEMORY_BASIC_INFORMATION region{};
        if (!VirtualQuery(site,&region,sizeof(region)) || region.State!=MEM_COMMIT) return false;
        const auto offset=reinterpret_cast<std::uintptr_t>(site)-reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (offset>region.RegionSize || Size>region.RegionSize-offset) return false;
        site_=site;
        before_=expected;
        after_=call;
        return write(before_,after_,true);
    }
    bool restore() { return maintain() && (!owned_ || write(after_,before_,false)); }
};
} // namespace stormslinger
