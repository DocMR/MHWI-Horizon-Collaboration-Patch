#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_timer_update_entry.h"
#include "stormbolt_timer_update_dispatch.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include <cassert>
#include <cstdio>
extern "C" {
void stormbolt_timer_test_dispatch(void*,float);
bool stormbolt_timer_test_query(void*);
void stormbolt_timer_test_call(void*,void*);
extern unsigned char stormbolt_timer_test_patch_site;
void* stormbolt_timer_test_seen=nullptr;
void* stormbolt_timer_test_query_owner=nullptr;
std::uintptr_t stormbolt_timer_test_query_value=0,stormbolt_timer_test_next_query_value=0;
unsigned stormbolt_timer_test_query_calls=0;
unsigned char stormbolt_timer_test_flag=0;
float stormbolt_timer_test_step=0.25f,stormbolt_timer_test_step_seen=0;
void (*stormbolt_timer_update_dispatch_target)(void*,float)=&stormbolt_timer_test_dispatch;
bool (*stormbolt_timer_update_native_query)(void*)=&stormbolt_timer_test_query;
}
int main() {
    const auto address=reinterpret_cast<DWORD64>(&stormbolt_timer_update_entry);
    DWORD64 image=0;auto* function=RtlLookupFunctionEntry(address,&image,nullptr);assert(function);
    alignas(16) std::array<std::uint64_t,32> stack{};
    const auto base=reinterpret_cast<DWORD64>(stack.data());stack[26]=0x12345678;stack[27]=0x87654321;
    const auto* unwind=reinterpret_cast<const unsigned char*>(image+function->UnwindData);
    CONTEXT context{};context.Rip=address+unwind[1];context.Rsp=base;context.Rbp=base+0xD0;
    void* handler=nullptr;DWORD64 frame=0;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,&handler,&frame,nullptr);
    assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xE0);
    std::array<unsigned char,7> original{};std::memcpy(original.data(),&stormbolt_timer_test_patch_site,7);
    assert(original[0]==0xE8 && original[5]==0x84 && original[6]==0xC0);
    stormslinger::StormboltLoopRelay relay;
    assert(relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_timer_test_patch_site),address));
    stormslinger::StormboltLoopCallPatch<> patch;
    assert(patch.install(&stormbolt_timer_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
    int owner=0;struct Status {std::uintptr_t vtable=1;void* owner;} status{1,&owner};
    for (unsigned flag:{0,1}) {
        stormbolt_timer_test_next_query_value=0x12340000u+flag;
        stormbolt_timer_test_query_value=0;stormbolt_timer_test_seen=nullptr;
        std::array<std::uint64_t,24> result{};
        stormbolt_timer_test_call(&status,result.data());
        assert(stormbolt_timer_test_seen==&status && stormbolt_timer_test_step_seen==0.25f);
        assert(stormbolt_timer_test_query_owner==&owner && result[0]==0x12340000u+flag);
        assert(result[1]==reinterpret_cast<std::uintptr_t>(&owner));
        for (unsigned i:{2,3,4,5,6}) assert(result[i]==0x1234);
        const auto* bytes=reinterpret_cast<const unsigned char*>(result.data());
        for (unsigned i=56;i<152;++i) assert(bytes[i]==0xFF);
        assert(bytes[152]==(flag==0));
        assert((result[21]&0x8C5)==(flag==0?0x44u:0u));
    }
    assert(stormbolt_timer_test_query_calls==2 && patch.restore());
    assert(!std::memcmp(original.data(),&stormbolt_timer_test_patch_site,7));
    stormbolt_timer_test_seen=nullptr;stormbolt_timer_test_query_value=0x12340001;
    std::array<std::uint64_t,24> result{};stormbolt_timer_test_call(&status,result.data());
    assert(!stormbolt_timer_test_seen && stormbolt_timer_test_query_calls==3 && result[0]==0x12340001);
    stormslinger::StormboltPowerCellStore<2> store;
    stormslinger::StormboltTimerUpdateBindings bindings{};
    bindings.inputs.status_vtable=1;
    bindings.query=+[](void*,std::uint32_t,std::uint32_t,std::int32_t)->std::int32_t {assert(false);return 0;};
    bindings.prompt=+[](void*,std::uint32_t,bool,std::int32_t) {assert(false);};
    bindings.native_query=&stormbolt_timer_test_query;
    stormslinger::StormboltTimerUpdateDispatch<2> dispatch(store,bindings);
    std::array<unsigned char,0x8940> actor{};
    status.owner=actor.data();actor[0x8938]=1;
    const auto token=store.initialize(reinterpret_cast<std::uintptr_t>(&status));
    assert(store.transact(token,[](auto& state) noexcept {state.count=3;state.timer=1;return true;}));
    assert(dispatch.publish() && !dispatch.publish());
    assert(patch.install(&stormbolt_timer_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
    stormbolt_timer_test_call(&status,result.data());
    stormslinger::StormboltPowerCellState snapshot{};
    assert(store.snapshot(token,snapshot) && snapshot.count==3 && snapshot.timer==0.75f);
    assert(stormbolt_timer_test_query_owner==actor.data());
    assert(patch.restore() && dispatch.withdraw());
    assert(stormbolt_timer_update_dispatch_target==&stormbolt_timer_test_dispatch &&
        stormbolt_timer_update_native_query==&stormbolt_timer_test_query);
    std::puts("PASS: timer CALL/relay/entry, RSI/XMM9 arguments, original query/TEST replay, registers, unwind and restore");
}
