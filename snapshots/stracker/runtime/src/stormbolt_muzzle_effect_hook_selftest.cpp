#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_muzzle_effect_hook.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
unsigned calls=0; bool ready=true;
void* expected_component{};
__declspec(noinline) void* native_effect(void* component,std::uint32_t group,std::uint32_t record,std::uint32_t parameter) {
    assert(component==expected_component && group==2000 && record==0 && parameter==200);
    ++calls;return reinterpret_cast<void*>(0x1234);
}
template<class T,class O> void put(O& object,std::size_t offset,T value) {
    std::memcpy(object.data()+offset,&value,sizeof(value));
}
}
int main() {
    StormboltInstanceStore<2> store;
    StormboltMuzzleEffectHook<2> hook(store);
    alignas(8) std::array<unsigned char,0x8810> actor{};
    alignas(8) std::array<unsigned char,0x27D8> weapon{};
    alignas(8) std::array<unsigned char,0x20> component{};
    std::array<void*,1> actor_table{},weapon_table{};
    put(actor,0,actor_table.data());put(actor,0xC,std::uint8_t{2});
    put(actor,0x76B0,weapon.data());put(actor,0x8808,component.data());
    put(component,0x18,actor.data());expected_component=component.data();
    put(weapon,0,weapon_table.data());put(weapon,0xC,std::uint8_t{2});
    put(weapon,0x9B0,actor.data());put(weapon,0x9F8,std::uint32_t{13});
    put(weapon,0x9FC,std::uint32_t{239});
    // Execute a real CALL with a known return address, preserving all4 args.
    unsigned char code[]{0x48,0xB8,0,0,0,0,0,0,0,0,0x48,0x83,0xEC,0x28,
                         0xFF,0xD0,0x48,0x83,0xC4,0x28,0xC3};
    const auto target=reinterpret_cast<std::uintptr_t>(&native_effect);
    std::memcpy(code+2,&target,8);
    auto* memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(memory);std::memcpy(memory,code,sizeof(code));DWORD old{};
    assert(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),memory,sizeof(code)));
    auto invoke=reinterpret_cast<StormboltMuzzleEffect>(memory);
    HookTransaction tx;assert(tx.begin()==MH_OK);
    assert(hook.prepare(tx,{native_effect,reinterpret_cast<std::uintptr_t>(memory+16),
        reinterpret_cast<std::uintptr_t>(actor_table.data()),reinterpret_cast<std::uintptr_t>(weapon_table.data()),
        +[](void*){return ready;},nullptr})==MH_OK);
    assert(tx.commit()==MH_OK);
    unsigned cases=0;
    for(unsigned scenario=0;scenario<9;++scenario) {
        ready=true;put(weapon,0x2388,std::uint32_t{1});put(weapon,0x9FC,std::uint32_t{239});
        put(weapon,0x27D0,std::uint32_t{40});put(weapon,0x27C8,std::uint32_t{40});
        put(actor,0x8808,component.data());
        auto token=store.initialize(reinterpret_cast<std::uintptr_t>(weapon.data()));assert(token);
        if(scenario==1) put(weapon,0x2388,std::uint32_t{0});
        if(scenario==2) put(weapon,0x27D0,std::uint32_t{0});
        if(scenario==3) put(weapon,0x9FC,std::uint32_t{237});
        if(scenario==4) ready=false;
        if(scenario==5) store.retire(token);
        if(scenario==6) put(actor,0x8808,weapon.data());
        if(scenario==7) put(weapon,0x27D0,std::uint32_t{0xFF});
        const auto before=calls;
        const auto result=invoke(component.data(),2000,0,200);
        const bool suppressed=scenario==0 || scenario==7 || scenario==8;
        assert(result==(suppressed?nullptr:reinterpret_cast<void*>(0x1234)));
        assert(calls==before+(suppressed?0:1));++cases;
    }
    // Exact original from an unrelated callsite must survive, even selected40.
    StormboltMuzzleEffect volatile other=native_effect;
    const auto before=calls;assert(other(component.data(),2000,0,200)==reinterpret_cast<void*>(0x1234));
    assert(calls==before+1);
    hook.withdraw_while_quiescent();
    assert(invoke(component.data(),2000,0,200)==reinterpret_cast<void*>(0x1234));
    assert(tx.stop()==MH_OK);hook.release_after_stop();
    assert(invoke(component.data(),2000,0,200)==reinterpret_cast<void*>(0x1234));
    assert(VirtualFree(memory,0,MEM_RELEASE));
    std::printf("PASS:%u scoped muzzle cases, exact callsite, no-battery/ordinary/unrelated fallback and restore.\n",cases);
}
