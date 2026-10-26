#ifdef NDEBUG
#undef NDEBUG
#endif
#include <Windows.h>
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
#include "generated_source_command_fixture.h"
#include "generated_source_command_ps_cases.h"
#include "stormbolt_source_transition_predicate.h"
using namespace stormslinger;
template<class T> void put(void* p,std::size_t o,T v) {std::memcpy(static_cast<unsigned char*>(p)+o,&v,sizeof(v));}
template<class T> T get(void* p,std::size_t o) {T v;std::memcpy(&v,static_cast<unsigned char*>(p)+o,sizeof(v));return v;}
int main() {
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,command_fixture_image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    assert(image);
    for(const auto& span:command_fixture_spans) std::memcpy(image+span.rva,span.data,span.size);
    std::memcpy(image+0x3EEABF0,command_fixture_mapping,sizeof(command_fixture_mapping));
    for(const auto& span:command_fixture_spans) {
        DWORD old{};assert(VirtualProtect(image+span.rva,span.size,PAGE_EXECUTE_READ,&old));
    }
    assert(FlushInstructionCache(GetCurrentProcess(),image,command_fixture_image_size));
    // Correct paired SetBomb native placement, including its original base call.
    std::array<unsigned char,0x58> placement_storage{};
    placement_storage.fill(0xA5);
    auto place=reinterpret_cast<void*(*)(void*,void*)>(image+0xF03D20);
    auto* placed=placement_storage.data()+16;
    assert(place(nullptr,placed)==placed);
    assert(get<void*>(placed,0)==image+0x31C92B0);
    assert(get<std::uint32_t>(placed,0x28)==0x20);
    assert(get<std::uint32_t>(placed,0x2C)==0xFFFF && !placed[0x30]);
    for(unsigned n=0;n<16;++n) assert(placement_storage[n]==0xA5 && placement_storage[0x48+n]==0xA5);
    assert(!place(nullptr,nullptr));
    std::array<unsigned char,0xE488> actor{},other{};
    std::array<unsigned char,0x5000> owner{};
    std::array<unsigned char,0xAE48> manager{};
    std::array<unsigned char,0x146F0> system{};
    std::array<unsigned char,0xA8> component{};
    std::array<unsigned char,0x668> status{};
    std::array<unsigned char,0x100> physical{};
    std::array<unsigned char,0x38> transition{};
    auto* input=owner.data()+0x10;
    put(image,0x51C4640,system.data());put(image,0x500ECA0,manager.data());
    put(actor.data(),0x14F8,owner.data());put(actor.data(),0x7658,component.data());
    put(actor.data(),0x7D20,status.data());put(actor.data(),0xE480,physical.data());
    actor[0xC]=other[0xC]=2;
    put(manager.data(),0x48,std::uint64_t{123});put(manager.data(),0xAE40,std::uint64_t{123});
    put(transition.data(),0x10,actor.data());put(transition.data(),0x20,input);
    put(transition.data(),0x28,std::uint32_t{0x20});
    auto command=reinterpret_cast<int(*)(void*)>(image+0x17BE130);
    StormboltSourceTransitionPredicateApi gates{};
    gates.native_platform_blocked=reinterpret_cast<bool(*)()>(image+0x240720);
    gates.native_authority_query=reinterpret_cast<std::uint8_t(*)(void*,std::uint32_t)>(image+0x5DC320);
    void* authority_object=component.data();gates.native_authority_slot=&authority_object;
    assert(!gates.is_platform_blocked());
    for(unsigned value=0;value<256;++value) {
        component[0x38]=static_cast<unsigned char>(value);
        assert(gates.has_authority()==(value!=0));
    }
    authority_object=nullptr;assert(!gates.has_authority());
    unsigned cases=0;
    // Exhaust input combinations, both local/remote actors, both platform flags,
    // and each of the four native actor blockers. Expected decisions are a
    // disassembly-derived model; PS full helper execution is a separate check.
    for(unsigned bits=0;bits<8192;++bits) for(unsigned blocked=0;blocked<5;++blocked) {
        const bool flag=bits&1,option=bits&2,main=bits&4,alt=bits&8,mode=bits&16;
        const bool held=bits&32,pressed=bits&64,third=bits&128,fourth=bits&256;
        const bool early=bits&512,sys1=bits&1024,sys2=bits&2048,local=bits&4096;
        owner.fill(0);physical.fill(0);component.fill(0);status.fill(0);
        put(input,0x30,actor.data());put(owner.data(),0x4EF8,std::uint32_t{0xFFFFFFFF});
        put(input,0x780+4,std::uint32_t{held?1u:0u});
        put(input,0x780+16,std::uint32_t{early?0x2000u:0u});
        put(input,0x7C0+4,std::uint32_t{pressed?1u:0u});
        put(input,0x820+4,std::uint32_t{third?1u:0u});
        put(input,0x7E0+4,std::uint32_t{fourth?1u:0u});
        put(physical.data(),8,std::uint32_t{main?0x80000000u:0u});
        put(physical.data(),0x68,std::uint32_t{alt?0x80000000u:0u});
        actor[0xE44C]=mode;transition[8]=flag;transition[0x30]=option;
        system[0x146D5]=sys1;system[0x146EA]=sys2;
        if(blocked && blocked<4) component[0xA3+blocked]=1;
        if(blocked==4) status[0x661]=1;
        put(manager.data(),0x58,local?actor.data():other.data());
        const bool gate=!blocked && !(early&&(sys1||sys2));
        const bool alternate=!mode||alt;
        const bool decision=flag ? (option ? alternate&&main&&!held :
            alternate&&((fourth&&held)||(main&&(held||pressed||third)))) :
            alternate&&(held?!option:(option||pressed||third));
        const bool success=gate&&decision;
        const auto& ps=source_command_ps_cases[cases];
        assert(ps.bits==bits && ps.blocked==blocked);
        const auto actual=command(transition.data());
        assert(actual==(success?0:-1));
        assert(actual==ps.result);
        assert(get<std::uint32_t>(owner.data(),0x4EF8)==(success&&local?0x20u:0xFFFFFFFFu));
        // Command8D request is registered even when the system/actor blocks.
        assert(get<std::uint32_t>(input,0x860+16)==0x2000);
        assert(get<std::uint32_t>(input,0x8A0+16)==0x2000);
        const bool writes=gate&&!(flag&&option);
        assert(get<std::uint32_t>(input,0x860+4)==(writes?1u:0u));
        assert(get<std::uint32_t>(input,0x8A0+4)==(writes&&alternate?1u:0u));
        assert(get<std::uint32_t>(input,0x860+4)==ps.request);
        assert(get<std::uint32_t>(input,0x8A0+4)==ps.accepted);
        assert(get<std::uint32_t>(input,0x860+16)==ps.early_request);
        assert(get<std::uint32_t>(input,0x8A0+16)==ps.early_accepted);
        assert(get<std::uint32_t>(owner.data(),0x4EF8)==ps.record);
        ++cases;
    }
    assert(VirtualFree(image,0,MEM_RELEASE));
    std::printf("PASS native PC command closure vs actual PS210-instruction oracle: %u input/blocker/local cases, results/request bits/local record.\n",cases);
}
