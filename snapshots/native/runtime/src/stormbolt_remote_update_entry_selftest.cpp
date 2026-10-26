#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include "stormbolt_remote_update_dispatch.h"
#include "stormbolt_sync_complete_entry.h"
#include "stormbolt_sync_complete_dispatch.h"
#include "stormbolt_sync_pack_entry.h"
#include <cassert>
#include <cstdio>
extern "C" {
void stormbolt_remote_update_entry();
void stormbolt_remote_test_dispatch(void*);
void stormbolt_remote_test_call(void*,void*);
extern unsigned char stormbolt_remote_test_patch_site;
void (*stormbolt_remote_update_dispatch_target)(void*)=&stormbolt_remote_test_dispatch;
void (*stormbolt_remote_test_entry)()=&stormbolt_remote_update_entry;
void* stormbolt_remote_test_actor=nullptr;
unsigned char stormbolt_remote_test_flag=0,stormbolt_remote_test_use_site=0;
void stormbolt_sync_complete_test_dispatch(void*,std::uintptr_t);
void (*stormbolt_sync_complete_dispatch_target)(void*,std::uintptr_t)=&stormbolt_sync_complete_test_dispatch;
std::uintptr_t stormbolt_sync_complete_test_frame=0,stormbolt_sync_complete_test_seen_frame=0;
void stormbolt_sync_complete_test_call(void*,void*);
extern unsigned char stormbolt_sync_complete_test_patch_site;
std::uint32_t stormbolt_sync_pack_test_dispatch(void*,std::uintptr_t,std::uint32_t);
std::uint32_t (*stormbolt_sync_pack_dispatch_target)(void*,std::uintptr_t,std::uint32_t)=&stormbolt_sync_pack_test_dispatch;
void stormbolt_sync_pack_test_call(void*,void*);
extern unsigned char stormbolt_sync_pack_test_patch_site;
std::uintptr_t stormbolt_sync_pack_test_frame=0,stormbolt_sync_pack_test_seen_frame=0;
void* stormbolt_sync_pack_test_seen_controller=nullptr;
std::uint32_t stormbolt_sync_pack_test_seen_word=0;
}
static unsigned decoded=0;
int main() {
    const auto address=reinterpret_cast<DWORD64>(&stormbolt_remote_update_entry);
    DWORD64 image=0;auto* function=RtlLookupFunctionEntry(address,&image,nullptr);assert(function);
    alignas(16) std::array<std::uint64_t,32> stack{};
    const auto base=reinterpret_cast<DWORD64>(stack.data());stack[26]=0x12345678;stack[27]=0x87654321;
    const auto* unwind=reinterpret_cast<const unsigned char*>(image+function->UnwindData);
    CONTEXT context{};context.Rip=address+unwind[1];context.Rsp=base;context.Rbp=base+0xD0;
    void* handler=nullptr;DWORD64 frame=0;
    RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,&handler,&frame,nullptr);
    assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xE0);
    const std::array<unsigned char,6> original{{0x8B,0x87,0x98,0x59,0,0}};
    assert(!std::memcmp(original.data(),&stormbolt_remote_test_patch_site,6));
    stormslinger::StormboltLoopRelay relay;
    assert(relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_remote_test_patch_site),address));
    stormslinger::StormboltLoopCallPatch<stormslinger::CodeWordApi,6> patch;
    assert(patch.install(&stormbolt_remote_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
    std::array<unsigned char,0x59C4> actor{};
    for (unsigned path:{0,1}) for (unsigned flag:{0,1}) {
        std::uint32_t word=0xAABBCC00;std::memcpy(actor.data()+0x5998,&word,4);
        stormbolt_remote_test_flag=static_cast<unsigned char>(flag);
        stormbolt_remote_test_use_site=static_cast<unsigned char>(path);
        stormbolt_remote_test_actor=nullptr;
        std::array<std::uint64_t,24> result{};
        stormbolt_remote_test_call(actor.data(),result.data());
        assert(stormbolt_remote_test_actor==actor.data() && result[0]==(word|flag));
        for (unsigned i=1;i<7;++i) assert(result[i]==0x1234);
        const auto* bytes=reinterpret_cast<const unsigned char*>(result.data());
        for (unsigned i=56;i<152;++i) assert(bytes[i]==0xFF);
        assert(bytes[152]==(path==0));
    }
    assert(patch.restore() && !std::memcmp(original.data(),&stormbolt_remote_test_patch_site,6));
    stormbolt_remote_test_actor=nullptr;stormbolt_remote_test_use_site=1;
    std::array<std::uint64_t,24> result{};stormbolt_remote_test_call(actor.data(),result.data());
    assert(!stormbolt_remote_test_actor);
    stormslinger::StormboltPowerCellStore<2> statuses;
    stormslinger::StormboltPowerCellLoopStore<2> loops;
    int owner=0;void* manager=&owner;
    const auto owner_address=reinterpret_cast<std::uintptr_t>(&owner);
    std::memcpy(actor.data()+0xDB0,&owner_address,sizeof(owner_address));
    stormslinger::StormboltRemoteUpdateBindings bindings{};
    bindings.manager_slot=&manager;
    bindings.local_player=+[](void*)->void* {assert(false);return nullptr;};
    bindings.equipment=+[](void*,std::uint32_t,std::uint32_t,std::uint32_t,bool)->void* {assert(false);return nullptr;};
    bindings.decode=+[](const stormslinger::StormboltPowerCellRemoteCache& cache,
        stormslinger::StormboltPowerCellRemoteFlags&,bool&) {
        assert(cache.current[0]==0xAABBCC01 && cache.previous[0]==0);++decoded;return false;
    };
    stormslinger::StormboltRemoteUpdateDispatch<2,2> dispatch(statuses,loops,bindings);
    assert(dispatch.publish() && !dispatch.publish());
    assert(patch.install(&stormbolt_remote_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry()),original));
    stormbolt_remote_test_call(actor.data(),result.data());
    assert(decoded==1 && result[0]==0xAABBCC01);
    assert(patch.restore());
    const auto published=stormbolt_remote_update_dispatch_target;
    stormbolt_remote_update_dispatch_target=&stormbolt_remote_test_dispatch;
    assert(!dispatch.withdraw());
    stormbolt_remote_update_dispatch_target=published;
    assert(dispatch.withdraw() && stormbolt_remote_update_dispatch_target==&stormbolt_remote_test_dispatch);
    stormslinger::StormboltLoopRelay complete_relay;
    assert(complete_relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_sync_complete_test_patch_site),
        reinterpret_cast<std::uintptr_t>(&stormbolt_sync_complete_entry)));
    stormslinger::StormboltLoopCallPatch<stormslinger::CodeWordApi,8> complete_patch;
    assert(complete_patch.install(&stormbolt_sync_complete_test_patch_site,
        reinterpret_cast<std::uintptr_t>(complete_relay.entry())));
    stormbolt_remote_test_actor=nullptr;
    stormbolt_sync_complete_test_call(actor.data(),result.data());
    assert(stormbolt_remote_test_actor==actor.data() && result[0]==0x12345678 && (result[1]&1));
    assert(stormbolt_sync_complete_test_seen_frame==stormbolt_sync_complete_test_frame && stormbolt_sync_complete_test_frame);
    assert(complete_patch.restore());
    stormbolt_remote_test_actor=nullptr;
    stormbolt_sync_complete_test_call(actor.data(),result.data());
    assert(!stormbolt_remote_test_actor && result[0]==0x12345678 && (result[1]&1));
    stormslinger::StormboltPowerCellSyncStore<2> completion_events;
    stormslinger::StormboltPowerCellSyncFrames<2> completion_frames;
    const auto completion_token=completion_events.initialize(reinterpret_cast<std::uintptr_t>(actor.data()));
    assert(stormslinger::mark_stormbolt_power_cell_sync_controller(completion_token,completion_events));
    stormslinger::StormboltPowerCellSyncSnapshot completion_snapshot{};
    completion_snapshot.controller_token=completion_token;
    assert(completion_frames.begin(stormbolt_sync_complete_test_frame,completion_snapshot));
    stormslinger::StormboltSyncCompleteDispatch<2,2> completion_dispatch(completion_frames,completion_events);
    assert(completion_dispatch.publish() && !completion_dispatch.publish() && !completion_dispatch.withdraw());
    assert(complete_patch.install(&stormbolt_sync_complete_test_patch_site,
        reinterpret_cast<std::uintptr_t>(complete_relay.entry())));
    stormbolt_sync_complete_test_call(actor.data(),result.data());
    stormslinger::StormboltPowerCellSyncEventState completed_event{};
    assert(completion_events.snapshot(completion_token,completed_event) && !completed_event.pending && !completion_frames.active());
    assert(result[0]==0x12345678 && (result[1]&1));
    assert(complete_patch.restore() && completion_dispatch.withdraw());
    assert(stormbolt_sync_complete_dispatch_target==&stormbolt_sync_complete_test_dispatch);
    const std::array<unsigned char,6> pack_original{{0x89,0x7D,0xA7,0x4C,0x2B,0xC0}};
    assert(!std::memcmp(pack_original.data(),&stormbolt_sync_pack_test_patch_site,6));
    stormslinger::StormboltLoopRelay pack_relay;
    assert(pack_relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_sync_pack_test_patch_site),
        reinterpret_cast<std::uintptr_t>(&stormbolt_sync_pack_entry)));
    stormslinger::StormboltLoopCallPatch<stormslinger::CodeWordApi,6> pack_patch;
    assert(pack_patch.install(&stormbolt_sync_pack_test_patch_site,
        reinterpret_cast<std::uintptr_t>(pack_relay.entry()),pack_original));
    stormbolt_sync_pack_test_call(actor.data(),result.data());
    assert(stormbolt_sync_pack_test_seen_controller==actor.data() && stormbolt_sync_pack_test_seen_word==0x11);
    assert(stormbolt_sync_pack_test_seen_frame==stormbolt_sync_pack_test_frame);
    assert(result[0]==0xAABBCCDD && result[1]==0x1234 && result[2]==0x1111 && result[3]==0x4567 && result[4]==0xAABBCCDD);
    assert(result[5]==UINT64_MAX && result[6]==UINT64_MAX);
    assert(pack_patch.restore());
    stormbolt_sync_pack_test_call(actor.data(),result.data());
    assert(result[0]==0x11 && result[4]==0x11 && result[2]==0x1111);
    std::puts("PASS: remote prehistory entry, six-byte CALL/relay, callback-before-MOV, volatile registers, flags, unwind and restore");
}
