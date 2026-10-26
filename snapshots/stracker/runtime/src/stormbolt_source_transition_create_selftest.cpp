#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_source_transition_create.h"
#include "stormbolt_source_transition_lifecycle.h"
#include <Windows.h>
#include <array>
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
std::array<std::byte,0x40> type{};
alignas(16) std::array<unsigned char,0x58> allocation{};
alignas(16) std::array<unsigned char,0x58> second_allocation{};
void* next_storage=allocation.data()+16;
unsigned heap_calls=0,allocate_calls=0;
bool fail=false;
StormboltSourceTransitionLifecycle<1>* active_lifecycle=nullptr;
unsigned fallback_creates=0,destroys=0;
__declspec(noinline) void* original_create(void*) {++fallback_creates;return nullptr;}
__declspec(noinline) void* original_destroy(void* object,std::uint32_t flags) {
    StormboltInstanceToken token{};
    assert(active_lifecycle && !active_lifecycle->instances().token_for_live_address(
        reinterpret_cast<std::uintptr_t>(object),token));
    assert(flags==5);++destroys;return object;
}
void* label=reinterpret_cast<void*>(0x11223344);
void* allocator_for(void* t) {
    assert(t==type.data());++heap_calls;
    // Native creation reads DTI metadata AFTER resolving its allocator.
    const std::uint32_t tag=0x12345678;
    std::memcpy(type.data()+0x34,&tag,4);
    std::memcpy(type.data()+8,&label,8);
    return &heap_calls;
}
void* allocate(void* heap,std::size_t bytes,std::uint32_t alignment,std::uint32_t tag,
    std::uint32_t a,std::uint32_t b,const void* name) {
    assert(heap==&heap_calls && bytes==0x38 && alignment==16);
    assert(tag==0x12345678 && !a && !b && name==label);
    ++allocate_calls;
    return fail ? nullptr : next_storage;
}
template<class T> T read(const void* p,std::size_t o) {
    T v{};std::memcpy(&v,static_cast<const std::byte*>(p)+o,sizeof(v));return v;
}
}
int main() {
    // Complete native F03D20 and base17B7790; only CALL/LEA relocations change.
    const unsigned char native_place[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xda,0x48,0x85,0xd2,0x74,0x22,0x48,0x8b,0xca,0xe8,0xea,0x01,0x00,0x00,0x48,0x8d,0x05,0xe3,0x00,0x00,0x00,0xc7,0x43,0x28,0x20,0x00,0x00,0x00,0x48,0x89,0x03,0x48,0x8b,0xc3,0x48,0x83,0xc4,0x20,0x5b,0xc3,0x33,0xc0,0x48,0x83,0xc4,0x20,0x5b,0xc3};
    const unsigned char native_base[]={0x33,0xc0,0xc6,0x41,0x08,0x00,0x48,0x89,0x41,0x10,0x48,0x89,0x41,0x18,0x48,0x89,0x41,0x20,0x48,0x8d,0x05,0x67,0xff,0xff,0xff,0x48,0x89,0x01,0x48,0x8b,0xc1,0xc7,0x41,0x28,0xff,0xff,0x00,0x00,0xc7,0x41,0x2c,0xff,0xff,0x00,0x00,0xc6,0x41,0x30,0x00,0xc3};
    // Actual native context initializer11CB480; no address relocations.
    const unsigned char native_context[]={0x48,0x89,0x51,0x10,0x48,0x8b,0x82,0xf8,0x14,0x00,0x00,0x48,0x89,0x41,0x18,0x48,0x83,0xc0,0x10,0x48,0x89,0x41,0x20,0xc3};
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    assert(page);
    std::memcpy(page,native_place,sizeof(native_place));
    std::memcpy(page+0x80,native_context,sizeof(native_context));
    std::memcpy(page+0x200,native_base,sizeof(native_base));
    DWORD old{};assert(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
    StormboltSourceTransitionCreateApi api{type.data(),allocator_for,allocate,
        reinterpret_cast<decltype(api.place)>(page)};
    allocation.fill(0xA5);
    assert(!stormbolt_place_source_transition(api,allocation.data()+16,0x28));
    for(auto b:allocation) assert(b==0xA5);
    auto* object=stormbolt_create_source_transition(api);
    assert(object==allocation.data()+16 && heap_calls==1 && allocate_calls==1);
    assert(read<void*>(object,0)==page+0x100 && read<std::uint8_t>(object,8)==0);
    for(auto o:{0x10u,0x18u,0x20u}) assert(read<std::uintptr_t>(object,o)==0);
    assert(read<std::uint32_t>(object,0x28)==32 && read<std::uint32_t>(object,0x2C)==0xFFFF);
    assert(read<std::uint8_t>(object,0x30)==0);
    for(std::size_t n=0;n<16;++n) assert(allocation[n]==0xA5);
    for(std::size_t n=16+0x31;n<allocation.size();++n) assert(allocation[n]==0xA5);
    std::array<std::byte,0x1500> actor{};
    std::array<std::byte,0x30> input_owner{};
    void* input=input_owner.data();std::memcpy(actor.data()+0x14F8,&input,8);
    reinterpret_cast<void (*)(void*,void*)>(page+0x80)(object,actor.data());
    assert(read<void*>(object,0x10)==actor.data());
    assert(read<void*>(object,0x18)==input_owner.data());
    assert(read<void*>(object,0x20)==input_owner.data()+16);
    assert(read<std::uint32_t>(object,0x28)==32 && read<std::uint8_t>(object,0x30)==0);
    allocation.fill(0xB6);fail=true;
    assert(!stormbolt_create_source_transition(api));
    assert(heap_calls==2 && allocate_calls==2);
    for(auto b:allocation) assert(b==0xB6);
    fail=false;
    {
        StormboltSourceTransitionLifecycle<1> lifecycle;
        active_lifecycle=&lifecycle;
        std::array<void*,4> dti_table{};
        dti_table[1]=reinterpret_cast<void*>(&original_create);
        StormboltSourceTransitionLifecycleBindings bindings{
            reinterpret_cast<std::uintptr_t>(dti_table.data()),&original_create,api.place,&original_destroy};
        HookTransaction transaction;
        assert(transaction.begin()==MH_OK);
        assert(lifecycle.prepare(transaction,api,bindings)==MH_OK);
        assert(transaction.commit()==MH_OK);
        assert(lifecycle.activate_factory());
        auto invoke=[&](void* t) {
            return reinterpret_cast<void* (*)(void*)>(dti_table[1])(t);
        };
        // Observed native placement gets a sidecar; lookup alone never does.
        allocation.fill(0xC7);
        assert(api.place(type.data(),allocation.data()+16)==allocation.data()+16);
        StormboltInstanceToken token{},fresh{};
        assert(lifecycle.instances().token_for_live_address(
            reinterpret_cast<std::uintptr_t>(allocation.data()+16),token));
        object=invoke(type.data());
        assert(object && read<std::uint32_t>(object,0x28)==32);
        assert(lifecycle.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object),token));
        next_storage=second_allocation.data()+16;
        auto* overflow=invoke(type.data());
        assert(overflow==next_storage && read<std::uint32_t>(overflow,0x28)==32);
        assert(!lifecycle.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(overflow),fresh));
        StormboltSourceTransitionState state_for_capacity{};
        assert(lifecycle.instances().snapshot(token,state_for_capacity));
        assert(original_destroy(overflow,5)==overflow && destroys==1);
        next_storage=allocation.data()+16;
        assert(api.place(type.data(),object)==object);
        StormboltSourceTransitionState state{};
        assert(!lifecycle.instances().snapshot(token,state));
        assert(lifecycle.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object),fresh));
        assert(fresh.generation!=token.generation);
        token=fresh;
        assert(invoke(type.data())==object);
        assert(lifecycle.instances().token_for_live_address(reinterpret_cast<std::uintptr_t>(object),fresh));
        assert(fresh.generation!=token.generation);
        assert(original_destroy(object,5)==object && destroys==2);
        assert(!lifecycle.instances().snapshot(fresh,state));
        fail=true;assert(!invoke(type.data()));fail=false;
        assert(!invoke(nullptr) && fallback_creates==1);
        const auto installed_factory=dti_table[1];
        dti_table[1]=reinterpret_cast<void*>(&original_destroy);
        assert(!lifecycle.restore_factory() && lifecycle.factory_active());
        assert(!lifecycle.release_after_stop());
        dti_table[1]=installed_factory;
        assert(lifecycle.restore_factory());
        assert(dti_table[1]==reinterpret_cast<void*>(&original_create));
        assert(transaction.stop()==MH_OK);
        assert(lifecycle.release_after_stop());
        assert(!invoke(type.data()) && fallback_creates==2);
    }
    assert(VirtualFree(page,0,MEM_RELEASE));
    std::puts("PASS: source transition38-byte allocation, complete native placement/base/context, padding/failure; owned DTI factory, observed placement registration, generation reuse, retire-before-destroy and restoration");
}
