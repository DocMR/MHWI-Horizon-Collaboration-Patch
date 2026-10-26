#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_class48_skill_adapter.h"
#include "stormbolt_class48_skill_hook.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
static void* expected_owner=nullptr;
static int fixture_percent=100;
static unsigned queries=0;
static StormboltPowerCellStore<2>* reset_store=nullptr;
static std::uintptr_t reset_address=0;
template<class T> void put(void* p,std::size_t offset,T value) {std::memcpy(static_cast<unsigned char*>(p)+offset,&value,sizeof(value));}
static unsigned native_calls=0;
__declspec(noinline) float native_multiplier(void* status,float input,std::uint32_t classification) {
    assert(status && classification<100);++native_calls;return input*2.0f;
}
int main() {
    std::array<unsigned char,0xA10> status{};std::array<unsigned char,0x28> perks{};
    std::array<unsigned char,0xBD8> resource{};int owner=0;
    put(status.data(),0,std::uintptr_t{2});put(status.data(),8,static_cast<void*>(&owner));
    put(resource.data(),0,std::uintptr_t{3});put(resource.data(),0xBD4,1.25f);
    std::uintptr_t slot=reinterpret_cast<std::uintptr_t>(resource.data());
    StormboltPowerCellStore<2> store;auto token=store.initialize(reinterpret_cast<std::uintptr_t>(status.data()));assert(token);
    expected_owner=&owner;
    StormboltClass48SkillBindings b{2,reinterpret_cast<std::uintptr_t>(&slot),3,
        +[](void* actor,std::uint32_t id,std::uint32_t selector,std::int32_t fallback) {
            assert(actor==expected_owner && id==70 && selector==0 && fallback==100);++queries;
            if(reset_store) (void)reset_store->initialize(reset_address);
            return fixture_percent;
        }};
    for(unsigned kind:{0u,17u,20u,39u,48u,49u}) for(int value:{0,100,110,115,120,-5})
        for(unsigned perk=0;perk<4;++perk) {
            fixture_percent=value;queries=0;std::memset(perks.data(),0,perks.size());
            put(status.data(),0x18,perk?static_cast<void*>(perks.data()):nullptr);
            if(perk) put(perks.data(),0x1C+4*(perk-1),std::uint32_t{22});
            float result=-7;assert(apply_stormbolt_class48_skill(status.data(),kind,13.75f,store,b,result));
            float factor=static_cast<float>(value)/100.0f;if(perk) factor*=1.25f;
            float expected=kind==48?13.75f*factor:13.75f;
            assert(result==expected && queries==unsigned(kind==48));
        }
    float result=123;reset_store=&store;reset_address=reinterpret_cast<std::uintptr_t>(status.data());
    assert(!apply_stormbolt_class48_skill(status.data(),48,10,store,b,result) && result==123);
    reset_store=nullptr;slot=0;
    assert(!apply_stormbolt_class48_skill(status.data(),48,10,store,b,result) && result==123);
    assert(apply_stormbolt_class48_skill(nullptr,17,10,store,{},result) && result==10);
    slot=reinterpret_cast<std::uintptr_t>(resource.data());
    put(status.data(),0x18,static_cast<void*>(nullptr));fixture_percent=120;
    StormboltClass48SkillHook<2> hook(store,b);
    HookTransaction transaction;assert(transaction.begin()==MH_OK);
    assert(hook.add_to_transaction(transaction,reinterpret_cast<void*>(&native_multiplier))==MH_OK);
    assert(transaction.commit_pending_without_rollback()==MH_OK);
    StormboltNativeSkillMultiplier volatile invoke=&native_multiplier;
    for(unsigned kind:{17u,20u,48u}) {
        native_calls=0;queries=0;
        const float actual=invoke(status.data(),10.0f,kind);
        assert(actual==(kind==48?24.0f:20.0f) && native_calls==1 && queries==unsigned(kind==48));
    }
    // Keep the same hooked status/token: equipment changes must be observed on
    // the next calculation, without reinitializing the patch-owned state.
    for(int percent:{100,110,120,100,120,110,100}) {
        fixture_percent=percent;
        for(bool enabled:{false,true,false}) {
            put(status.data(),0x18,enabled?static_cast<void*>(perks.data()):nullptr);
            std::memset(perks.data(),0,perks.size());
            put(perks.data(),0x1C,std::uint32_t{22});
            for(float set_factor:{1.25f,1.5f}) {
                put(resource.data(),0xBD4,set_factor);
                native_calls=0;queries=0;
                float expected=20.0f*(static_cast<float>(percent)/100.0f*
                    (enabled?set_factor:1.0f));
                assert(invoke(status.data(),10.0f,48)==expected);
                assert(native_calls==1 && queries==1);
            }
        }
    }
    put(status.data(),0x18,static_cast<void*>(nullptr));
    reset_store=&store;reset_address=reinterpret_cast<std::uintptr_t>(status.data());
    native_calls=0;assert(invoke(status.data(),10.0f,48)==20.0f && native_calls==1);
    reset_store=nullptr;
    assert(transaction.stop()==MH_OK && hook.release_after_unhook());
    native_calls=0;queries=0;assert(invoke(status.data(),10.0f,48)==20.0f && native_calls==1 && queries==0);
    std::puts("PASS: class48 skill70 integer parameter/division, all three perk slots, native other classes, stale generation and missing resource, real shared MinHook wrapper and original restoration");
}
