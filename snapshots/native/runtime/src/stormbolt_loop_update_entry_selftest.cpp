#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include "stormbolt_loop_audio_dispatch.h"
extern "C" {
void stormbolt_loop_test_dispatch(void*);
void stormbolt_loop_update_entry();
void (*stormbolt_loop_test_entry)()=&stormbolt_loop_update_entry;
void stormbolt_loop_test_call(void*,void*);
void (*stormbolt_loop_update_dispatch_target)(void*)=&stormbolt_loop_test_dispatch;
void* stormbolt_loop_test_actor=nullptr;
unsigned char stormbolt_loop_test_flag=0;
unsigned char stormbolt_loop_test_use_site=0;
extern unsigned char stormbolt_loop_test_patch_site;
}
int main() {
    {
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        assert(page);
        const std::array<unsigned char,6> original{{0x8B,0x87,0x98,0x59,0,0}};
        std::memcpy(page,original.data(),6);page[6]=0xA5;
        stormslinger::StormboltLoopCallPatch<stormslinger::CodeWordApi,6> six;
        assert(six.install(page,reinterpret_cast<std::uintptr_t>(page+64)));
        assert(page[0]==0xE8 && page[5]==0x90 && page[6]==0xA5);
        std::int32_t displacement=0;std::memcpy(&displacement,page+1,4);
        assert(displacement==59);
        const auto owned_byte=page[5];page[5]=0xCC;
        assert(!six.restore() && six.active() && page[5]==0xCC);
        page[5]=owned_byte;
        assert(six.restore() && !six.active() && !std::memcmp(page,original.data(),6) && page[6]==0xA5);
        page[0]=0xCC;
        assert(!six.install(page,reinterpret_cast<std::uintptr_t>(page+64)) && !six.active());
        assert(VirtualFree(page,0,MEM_RELEASE));
    }
    {
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        assert(page);
        const std::array<unsigned char,8> original{{0x4C,0x8B,0xAC,0x24,0xB8,0,0,0}};
        std::memcpy(page,original.data(),8);page[8]=0xA5;
        stormslinger::StormboltLoopCallPatch<stormslinger::CodeWordApi,8> completion;
        assert(completion.install(page,reinterpret_cast<std::uintptr_t>(page+64)));
        assert(page[0]==0xE8 && page[5]==0x90 && page[6]==0x90 && page[7]==0x90 && page[8]==0xA5);
        assert(completion.restore() && !std::memcmp(page,original.data(),8) && page[8]==0xA5);
        // The original instruction must fit wholly within one region.
        auto* edge=page+4090;
        assert(!completion.install(edge,reinterpret_cast<std::uintptr_t>(page+64)) && !completion.active());
        assert(VirtualFree(page,0,MEM_RELEASE));
    }
    {
        DWORD64 image=0;
        const auto entry=reinterpret_cast<DWORD64>(&stormbolt_loop_update_entry);
        auto* function=RtlLookupFunctionEntry(entry,&image,nullptr);
        assert(function);
        alignas(16) std::array<std::uint64_t,32> stack{};
        const auto base=reinterpret_cast<DWORD64>(stack.data());
        stack[24]=0x12345678; // Saved RBP at frame+0xC0.
        stack[25]=0x87654321; // Normal CALL return address.
        auto* unwind=reinterpret_cast<const unsigned char*>(image+function->UnwindData);
        CONTEXT context{};
        context.Rip=entry+unwind[1]; // Complete prologue / start of body.
        context.Rsp=base;
        context.Rbp=base+0xC0;
        void* handler=nullptr;
        DWORD64 establisher=0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,
                         &handler,&establisher,nullptr);
        assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xD0);
        const std::array<unsigned char,6> epilogue{{0x48,0x8D,0x65,0,0x5D,0xC3}};
        const auto* start=reinterpret_cast<const unsigned char*>(entry);
        const auto* end=reinterpret_cast<const unsigned char*>(image+function->EndAddress);
        const auto* found=std::search(start,end,epilogue.begin(),epilogue.end());
        assert(found!=end);
        for (unsigned offset: {0u,4u,5u}) {
            context={};
            context.Rip=reinterpret_cast<DWORD64>(found+offset);
            context.Rsp=base+(offset==0 ? 0 : offset==4 ? 0xC0 : 0xC8);
            context.Rbp=offset==5 ? 0x12345678 : base+0xC0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,
                             &handler,&establisher,nullptr);
            assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xD0);
        }
    }
    {
        std::array<unsigned char,7> encoded{{1,2,3,4,5,6,7}};
        const auto before=encoded;
        assert(!stormslinger::build_stormbolt_loop_call(0x1000,0x90000000,encoded));
        assert(encoded==before);
        assert(stormslinger::build_stormbolt_loop_call(0x1000,0x2000,encoded));
        assert(encoded[0]==0xE8 && encoded[5]==0x90 && encoded[6]==0x90);
        auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        assert(page);
        const std::array<unsigned char,7> original{{0x80,0xBF,0xB8,0x09,0,0,0}};
        std::memcpy(page,original.data(),7);
        DWORD old=0;
        assert(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old));
        stormslinger::StormboltLoopCallPatch<> patch;
        assert(patch.install(page,reinterpret_cast<std::uintptr_t>(page+128)) && patch.active());
        assert(page[0]==0xE8 && page[5]==0x90 && page[6]==0x90);
        assert(patch.restore() && !patch.active());
        assert(!std::memcmp(page,original.data(),7));
        assert(VirtualFree(page,0,MEM_RELEASE));
    }
    std::array<unsigned char,0x9D0> actor{};
    stormslinger::StormboltLoopRelay relay;
    assert(relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_loop_test_call),
                         reinterpret_cast<std::uintptr_t>(&stormbolt_loop_update_entry)));
    for (auto entry: {reinterpret_cast<void*>(&stormbolt_loop_update_entry),relay.entry()}) {
      stormbolt_loop_test_entry=reinterpret_cast<void (*)()>(entry);
      for (unsigned char flag: {0,1}) {
        actor[0x9B8]=static_cast<unsigned char>(!flag);
        stormbolt_loop_test_flag=flag;
        std::array<unsigned char,160> result{};
        stormbolt_loop_test_call(actor.data(),result.data());
        assert(stormbolt_loop_test_actor==actor.data());
        for (std::size_t i=0;i<7;++i) {
            std::uint64_t value{};
            std::memcpy(&value,result.data()+8*i,8);
            assert(value==0x1234);
        }
        for (std::size_t i=56;i<152;++i) assert(result[i]==0xFF);
        assert(result[152]==static_cast<unsigned char>(flag==0));
      }
    }
    stormbolt_loop_test_use_site=1;
    stormslinger::StormboltLoopCallPatch<> installed;
    assert(relay.prepare(reinterpret_cast<std::uintptr_t>(&stormbolt_loop_test_patch_site),
                         reinterpret_cast<std::uintptr_t>(&stormbolt_loop_update_entry)));
    assert(installed.install(&stormbolt_loop_test_patch_site,
                             reinterpret_cast<std::uintptr_t>(relay.entry())));
    for (unsigned char flag: {0,1}) {
        actor[0x9B8]=static_cast<unsigned char>(!flag);
        stormbolt_loop_test_actor=nullptr;
        stormbolt_loop_test_flag=flag;
        std::array<unsigned char,160> result{};
        stormbolt_loop_test_call(actor.data(),result.data());
        assert(stormbolt_loop_test_actor==actor.data());
        for (std::size_t i=0;i<7;++i) {
            std::uint64_t value{};
            std::memcpy(&value,result.data()+8*i,8);
            assert(value==0x1234);
        }
        for (std::size_t i=56;i<152;++i) assert(result[i]==0xFF);
        assert(result[152]==static_cast<unsigned char>(flag==0));
    }
    assert(installed.restore() && !installed.active());
    const std::array<unsigned char,7> original{{0x80,0xBF,0xB8,0x09,0,0,0}};
    assert(!std::memcmp(&stormbolt_loop_test_patch_site,original.data(),7));
    actor[0x9B8]=0;
    stormbolt_loop_test_actor=nullptr;
    std::array<unsigned char,160> restored{};
    stormbolt_loop_test_call(actor.data(),restored.data());
    assert(!stormbolt_loop_test_actor && restored[152]==1);
    {
        using namespace stormslinger;
        const auto put=[](auto& buffer,std::size_t offset,const auto& value) {
            std::memcpy(buffer.data()+offset,&value,sizeof(value));
        };
        std::array<unsigned char,0x126E0> owner{};
        std::array<unsigned char,0x15A8> skills{};
        std::array<unsigned char,0xA10> status{};
        std::array<unsigned char,0xA98> resource{};
        std::array<unsigned char,0x448> material{};
        put(actor,0,std::uintptr_t{4});put(actor,0x60,static_cast<void*>(owner.data()));
        put(owner,0x60,std::uint8_t{1});put(owner,0x68,1.0f);
        put(owner,0x126D8,static_cast<void*>(skills.data()));
        put(owner,0x7D20,static_cast<void*>(status.data()));
        put(owner,0x2B8,std::uint32_t{1});put(owner,0x2B0,static_cast<void*>(material.data()));
        put(skills,0,std::uintptr_t{1});put(skills,0x141C,std::int32_t{3});
        put(status,0,std::uintptr_t{2});put(resource,0,std::uintptr_t{3});
        put(resource,0xA8C,std::uint32_t{9});put(resource,0xA90,std::uint32_t{2});
        const auto slot=reinterpret_cast<std::uintptr_t>(resource.data());
        StormboltPowerCellStore<1> statuses;
        const auto status_token=statuses.initialize(reinterpret_cast<std::uintptr_t>(status.data()));
        assert(statuses.transact(status_token,[](auto& s) noexcept {s.count=7;return true;}));
        StormboltPowerCellLoopStore<1> loops;
        const auto token=loops.initialize(reinterpret_cast<std::uintptr_t>(actor.data()));
        assert(loops.transact(token,[](auto& s) noexcept {s.growth=true;return true;}));
        static unsigned parameters=0;
        StormboltLoopComponentUpdateBindings bindings{};
        bindings.audio.component_vtable=4;
        bindings.audio.inputs={1,2,reinterpret_cast<std::uintptr_t>(&slot),3};
        bindings.audio.query=+[](void*,std::uint32_t,std::uint32_t,std::int32_t) {return 1;};
        bindings.material_at=+[](void* p,std::uint32_t)->void* {void* r=nullptr;std::memcpy(&r,static_cast<unsigned char*>(p)+0x2B0,8);return r;};
        bindings.convert=+[](const float* a,float* b)->float* {std::memcpy(b,a,16);return b;};
        bindings.scalar=+[](void*,std::uint32_t,float) {++parameters;};
        bindings.material.write_vector=+[](void*,std::uint32_t,const float*) {++parameters;};
        StormboltLoopComponentDispatch<1,1> dispatch(loops,statuses,bindings);
        assert(dispatch.publish());
        assert(installed.install(&stormbolt_loop_test_patch_site,reinterpret_cast<std::uintptr_t>(relay.entry())));
        std::array<unsigned char,160> result{};
        stormbolt_loop_test_call(actor.data(),result.data());
        StormboltPowerCellLoopState state{};
        assert(loops.snapshot(token,state) && state.timer==-1.0f && state.visual_phase==1 && !state.growth);
        assert(parameters==3 && result[152]==1);
        for (std::size_t i=0;i<7;++i) {std::uint64_t v=0;std::memcpy(&v,result.data()+i*8,8);assert(v==0x1234);}
        for (std::size_t i=56;i<152;++i) assert(result[i]==0xFF);
        // Equipment loss must stop our retained loop without querying another
        // weapon's skills, changing its flags, or touching a material.
        {
            static unsigned stops=0;
            std::array<unsigned char,0xA00> other{};
            put(other,0,std::uintptr_t{99});put(other,0x9F8,std::uint32_t{1});
            put(other,0x9FC,std::uint32_t{238});
            put(owner,0x76B0,other.data());put(owner,0xC,std::uint8_t{14});
            put(owner,0xA18,owner.data());put(actor,0xF0,owner.data());
            const std::array<float,3> defaults{};
            auto cleanup=bindings;
            cleanup.audio.inputs.target_weapon_vtable=99;
            cleanup.audio.query=nullptr;
            cleanup.audio.container_at=+[](void* p)->void* {return p;};
            cleanup.audio.native_first=defaults.data();cleanup.audio.native_second=defaults.data();
            cleanup.audio.request=+[](void*,std::int32_t,const char* event,StormboltNamedEventParameters* p) {
                assert(!std::strcmp(event,"STOP_PL_BODY_068_DMG_R_LP") && p->flags==0x12);
                ++stops;return true;
            };
            assert(loops.transact(token,[](auto& s) noexcept {s.active=true;return true;}));
            const auto before=owner;const auto other_before=other;
            assert(update_stormbolt_loop_from_component(actor.data(),loops,statuses,cleanup));
            assert(loops.snapshot(token,state) && !state.active && stops==1 && parameters==3);
            assert(owner==before && other==other_before);
            assert(update_stormbolt_loop_from_component(actor.data(),loops,statuses,cleanup.audio));
            assert(stops==1); // No duplicate STOP once the latch is cleared.
            put(owner,0x76B0,static_cast<void*>(nullptr));
            assert(loops.transact(token,[](auto& s) noexcept {s.active=true;return true;}));
            assert(update_stormbolt_loop_from_component(actor.data(),loops,statuses,cleanup.audio));
            assert(stops==2 && loops.snapshot(token,state) && !state.active);
            StormboltPowerCellState preserved{};
            assert(statuses.snapshot(status_token,preserved) && preserved.count==7);
        }
        assert(installed.restore() && dispatch.withdraw());
        assert(!std::memcmp(&stormbolt_loop_test_patch_site,original.data(),7));
        stormbolt_loop_test_call(actor.data(),result.data());
        assert(parameters==3); // Restored original site bypasses combined callback.
    }
    std::puts("PASS: installed CALL/relay/entry, GPR/XMM/flags, exact restore and original execution");
}
