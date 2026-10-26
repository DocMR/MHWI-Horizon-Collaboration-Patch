#pragma once
#include "stormbolt_loop_call_patch.h"

namespace stormslinger {
// Process-lifetime executable storage. Keep the owning DLL pinned once used.
// JMP [RIP] preserves registers/flags and the original CALL return address.
class StormboltLoopRelay {
    void* allocation_=nullptr;
    std::uintptr_t target_=0;
public:
    void* entry() const noexcept { return allocation_; }
    bool prepare(std::uintptr_t site,std::uintptr_t target) {
        std::array<unsigned char,7> call{};
        if (allocation_)
            return target==target_ && build_stormbolt_loop_call(
                site,reinterpret_cast<std::uintptr_t>(allocation_),call);
        if (!site || !target) return false;
        const auto aligned=site&~std::uintptr_t(0xFFFF);
        void* memory=nullptr;
        for (std::uintptr_t delta=0;delta<0x4000000 && !memory;delta+=0x10000) {
            for (bool above:{true,false}) {
                if ((above && aligned>(std::numeric_limits<std::uintptr_t>::max)()-delta) ||
                    (!above && aligned<delta)) continue;
                const auto candidate=above ? aligned+delta : aligned-delta;
                if (!build_stormbolt_loop_call(site,candidate,call)) continue;
                memory=VirtualAlloc(reinterpret_cast<void*>(candidate),4096,
                    MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
                if (memory) break;
            }
        }
        if (!memory) return false;
        std::array<unsigned char,14> bytes{{0xFF,0x25,0,0,0,0}};
        std::memcpy(bytes.data()+6,&target,8);
        std::memcpy(memory,bytes.data(),bytes.size());
        DWORD old=0;
        if (!VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old) ||
            !FlushInstructionCache(GetCurrentProcess(),memory,bytes.size())) {
            VirtualFree(memory,0,MEM_RELEASE);
            return false;
        }
        allocation_=memory;
        target_=target;
        return true;
    }
};
} // namespace stormslinger
