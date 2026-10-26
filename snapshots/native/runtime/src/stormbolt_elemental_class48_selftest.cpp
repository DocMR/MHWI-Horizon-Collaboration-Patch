#include "stormbolt_elemental_class48_patch.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstring>
#include <cstdint>
using namespace stormslinger;
int main() {
    // Actual original category gate and both continuation offsets; only replace
    // the subsequent native arithmetic with Boolean returns for this fixture.
    const unsigned char code[]{0x8B,0xC1,
        0x83,0xF8,5,0x74,8,0x83,0xC0,0xE4,0x83,0xF8,4,0x77,6,
        0xB8,1,0,0,0,0xC3,0xB8,0,0,0,0,0xC3};
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)); assert(memory);
    std::memcpy(memory,code,sizeof(code)); DWORD old=0;
    // Preserve RSI as in the native function; RDX supplies the live weapon.
    const unsigned char ammo_code[]{0x56,0x48,0x8B,0xF2,0x8B,0xC1,
        0x83,0xF8,0x1F,0x75,0x76,0x48,0x85,0xF6,0x74,0x71,
        0xB8,1,0,0,0,0x5E,0xC3};
    auto* ammo=memory+256;
    std::memcpy(ammo,ammo_code,sizeof(ammo_code));
    const unsigned char failed[]{0x33,0xC0,0x5E,0xC3};
    std::memcpy(ammo+6+0x7B,failed,sizeof(failed));
    assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),memory,sizeof(code)));
    using Fn=unsigned (*)(std::uint32_t);
    auto fn=reinterpret_cast<Fn>(memory);
    using AmmoFn=unsigned (*)(std::uint32_t,std::uintptr_t);
    auto ammo_fn=reinterpret_cast<AmmoFn>(ammo);
    auto native=[](std::uint32_t value){return value==5 || (value>=28 && value<=32);};
    for(unsigned i=0;i<256;++i) assert(fn(i)==native(i));
    HookTransaction tx; assert(tx.begin()==MH_OK);
    StormboltElementalClass48Patch patch(&tx);
    for(unsigned i=0;i<256;++i) for(unsigned owner=0;owner<2;++owner)
        assert(ammo_fn(i,owner)==(i==31 && owner!=0));
    assert(patch.prepare(memory+2,ammo+6));
    assert(tx.commit_pending_without_rollback()==MH_OK);
    assert(patch.activate());
    for(unsigned i=0;i<256;++i) assert(fn(i)==(native(i)||i==48));
    assert(fn(0xFFFFFFFFu)==0 && fn(0x80000000u)==0);
    for(unsigned i=0;i<256;++i) for(unsigned owner=0;owner<2;++owner)
        assert(ammo_fn(i,owner)==((i==31 || i==48) && owner!=0));
    assert(ammo_fn(0xFFFFFFFFu,1)==0 && ammo_fn(0x80000000u,1)==0);
    assert(patch.restore());
    for(unsigned i=0;i<256;++i) assert(fn(i)==native(i));
    assert(!std::memcmp(memory,code,sizeof(code)));
    assert(!std::memcmp(ammo,ammo_code,sizeof(ammo_code)));
    for(unsigned i=0;i<256;++i) for(unsigned owner=0;owner<2;++owner)
        assert(ammo_fn(i,owner)==(i==31 && owner!=0));
    assert(tx.stop()==MH_OK); assert(patch.release_after_quiescence());
    assert(VirtualFree(memory,0,MEM_RELEASE));
}
