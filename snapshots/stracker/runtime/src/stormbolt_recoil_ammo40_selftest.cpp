#include "stormbolt_recoil_ammo40.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace stormslinger;
int main() {
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(memory);
    const unsigned char caller[]{0x48,0x83,0xEC,0x28,0x8B,0xD1,0x33,0xC9,
        0xE8,0xCE,0x0D,0,0,0x48,0x83,0xC4,0x28,0xC3};
    const unsigned char original[]{0x8B,0xC2,0x83,0xC0,0x64,0xC3};
    std::memcpy(memory,caller,sizeof(caller));
    auto* helper=memory+13+0xDCE;std::memcpy(helper,original,sizeof(original));
    DWORD old{};assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),memory,4096));
    auto run=reinterpret_cast<std::uint32_t(*)(std::uint32_t)>(memory);
    for(unsigned n=0;n<256;++n) assert(run(n)==n+100);
    HookTransaction tx;assert(tx.begin()==MH_OK);
    StormboltRecoilAmmo40Patch patch(&tx);assert(patch.prepare(memory+8));
    assert(tx.commit_pending_without_rollback()==MH_OK);assert(patch.activate());
    for(unsigned n=0;n<256;++n) assert(run(n)==(n==40 ? 15 : n+100));
    for(auto n:{0x7FFFFFFFu,0x80000000u,0xFFFFFFFFu}) assert(run(n)==n+100u);
    assert(!std::memcmp(helper,original,sizeof(original)));
    assert(patch.restore());
    for(unsigned n=0;n<256;++n) assert(run(n)==n+100);
    assert(!std::memcmp(memory,caller,sizeof(caller)));
    assert(tx.stop()==MH_OK);assert(patch.release_after_quiescence());
    assert(VirtualFree(memory,0,MEM_RELEASE));
    std::puts("PASS: ammo40 firing selector15; 256 classifications, extreme inputs, native fallback and exact restoration");
}
