#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_status_update_entry.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include "stormbolt_status_update_dispatch.h"
#include <cassert>
#include <cstdio>
extern "C" {
void stormbolt_status_test_dispatch(void*);
void stormbolt_status_test_call(void*,void*);
void* stormbolt_status_test_seen=nullptr;
std::uintptr_t stormbolt_status_test_manager=0,stormbolt_status_test_next_manager=0;
unsigned char stormbolt_status_test_flag=0;
extern unsigned char stormbolt_status_test_patch_site;
void (*stormbolt_status_update_dispatch_target)(void*)=&stormbolt_status_test_dispatch;
std::uintptr_t stormbolt_status_update_manager_slot=reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_manager);
}
int main() {
    DWORD64 image=0;
    const auto address=reinterpret_cast<DWORD64>(&stormbolt_status_update_entry);
    auto* function=RtlLookupFunctionEntry(address,&image,nullptr);assert(function);
    alignas(16) std::array<std::uint64_t,32> stack{};
    const auto base=reinterpret_cast<DWORD64>(stack.data());
    stack[26]=0x12345678;stack[27]=0x87654321;
    auto* unwind=reinterpret_cast<const unsigned char*>(image+function->UnwindData);
    CONTEXT context{};context.Rip=address+unwind[1];context.Rsp=base;context.Rbp=base+0xD0;
    void* handler=nullptr;DWORD64 establisher=0;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,&handler,&establisher,nullptr);
    assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xE0);
    std::array<unsigned char,7> original{};
    std::memcpy(original.data(),&stormbolt_status_test_patch_site,7);
    assert(original[0]==0x48 && original[1]==0x8B && original[2]==0x0D);
    stormslinger::StormboltLoopRelay relay;
    assert(relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_patch_site),address));
    stormslinger::StormboltLoopCallPatch<> patch;
    assert(patch.install(&stormbolt_status_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
    int status=0;
    for (unsigned char flag:{0,1,2}) {
        stormbolt_status_test_flag=flag;stormbolt_status_test_manager=0xAAAA;
        stormbolt_status_test_next_manager=0xBBBB;stormbolt_status_test_seen=nullptr;
        std::array<std::uint64_t,24> result{};
        stormbolt_status_test_call(&status,result.data());
        assert(stormbolt_status_test_seen==&status && result[1]==0xBBBB);
        for (unsigned i:{0,2,3,4,5,6}) assert(result[i]==0x1234);
        const auto* bytes=reinterpret_cast<const unsigned char*>(result.data());
        for (unsigned i=56;i<152;++i) assert(bytes[i]==0xFF);
        assert(result[20]==result[21]);
    }
    assert(patch.restore());assert(!std::memcmp(original.data(),&stormbolt_status_test_patch_site,7));
    stormbolt_status_test_manager=0xCCCC;stormbolt_status_test_seen=nullptr;
    std::array<std::uint64_t,24> result{};stormbolt_status_test_call(&status,result.data());
    assert(!stormbolt_status_test_seen && result[1]==0xCCCC && result[20]==result[21]);
    {
        using namespace stormslinger;
        const auto put=[](auto& b,std::size_t o,const auto& v) {std::memcpy(b.data()+o,&v,sizeof(v));};
        std::array<std::byte,0x126E0> owner{};
        std::array<std::byte,0x15A8> skills{};
        std::array<std::byte,0xA10> native_status{};
        std::array<std::byte,0xA98> resource{};
        put(native_status,0,std::uintptr_t{2});put(native_status,8,static_cast<void*>(owner.data()));
        put(owner,0x60,std::uint8_t{1});put(owner,0x126D8,static_cast<void*>(skills.data()));
        put(owner,0x7D20,static_cast<void*>(native_status.data()));
        put(skills,0,std::uintptr_t{1});put(skills,0x141C,std::int32_t{3});
        put(resource,0,std::uintptr_t{3});put(resource,0xA8C,std::uint32_t{4});
        const auto slot=reinterpret_cast<std::uintptr_t>(resource.data());
        StormboltPowerCellStore<1> store;
        const auto token=store.initialize(reinterpret_cast<std::uintptr_t>(native_status.data()));
        assert(store.transact(token,[](auto& s) noexcept {s.count=1;return true;}));
        static unsigned flag_calls=0;
        const StormboltStatusUpdateBindings bindings{
            {1,2,reinterpret_cast<std::uintptr_t>(&slot),3},
            +[](void*,std::uint32_t,std::uint32_t,std::int32_t) {return 1;},
            +[](void*,std::uint32_t flag) {assert(flag==0x98);++flag_calls;},
            reinterpret_cast<std::uintptr_t>(&stormbolt_status_test_manager)};
        const auto previous=stormbolt_status_update_dispatch_target;
        const auto previous_slot=stormbolt_status_update_manager_slot;
        StormboltStatusUpdateDispatch<1> dispatch(store,bindings);
        assert(dispatch.publish());
        assert(patch.install(&stormbolt_status_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
        stormbolt_status_test_call(native_status.data(),result.data());
        StormboltPowerCellState snapshot{};
        assert(store.snapshot(token,snapshot) && snapshot.event_eligible && flag_calls==1);
        assert(result[1]==0xCCCC && result[20]==result[21]);
        assert(store.transact(token,[](auto& s) noexcept {s.count=4;return true;}));
        stormbolt_status_test_call(native_status.data(),result.data());
        assert(store.snapshot(token,snapshot) && !snapshot.event_eligible && flag_calls==1);
        assert(store.retire(token));stormbolt_status_test_call(native_status.data(),result.data());
        assert(flag_calls==1);
        assert(patch.restore() && dispatch.withdraw());
        assert(stormbolt_status_update_dispatch_target==previous && stormbolt_status_update_manager_slot==previous_slot);
    }
    std::puts("PASS: status CALL/relay/entry, RSI argument, live MOV replay, GPR/XMM/RFLAGS, unwind and exact restore");
}
