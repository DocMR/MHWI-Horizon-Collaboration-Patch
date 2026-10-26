#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_growth_dispatch.h"
#include "stormbolt_growth_entry.h"
#include "stormbolt_loop_call_patch.h"
#include "stormbolt_loop_relay.h"
#include "stormbolt_owned_call_site.h"
extern "C" void stormbolt_growth_negative_entry_span();
extern "C" void stormbolt_growth_post_effect_entry_span();
extern "C" void stormbolt_growth_remaining_entry_span();
#include <cassert>
#include <cstdio>
using namespace stormslinger;
extern "C" {
void stormbolt_growth_test_path(void*,const void*,const void*,int,void*);
int stormbolt_growth_test_negative(void*,const void*,const void*,int);
int stormbolt_growth_test_post_effect(void*,const void*,const void*,int);
int stormbolt_growth_test_remaining(void*,const void*,const void*,int);
}
static void* expected_component;static const void* expected_event;static const void* expected_result;
static unsigned calls;
static unsigned* native_effect=nullptr;
static unsigned expected_native=0;
extern "C" {
std::uint64_t stormbolt_growth_seen[20]{};
void stormbolt_growth_clobber_callback(void*,const void*,const void*);
void stormbolt_growth_observe(void* c,const void* e,const void* r) {
    assert(c==expected_component && e==expected_event && r==expected_result);
    if(native_effect) assert(*native_effect==expected_native);
    ++calls;
}
}
static void verify(int value,unsigned kind) {
    for(unsigned n=0;n<7;++n) assert(stormbolt_growth_seen[n]==n+11);
    for(unsigned n=0;n<6;++n) assert(stormbolt_growth_seen[8+2*n]==11 && stormbolt_growth_seen[9+2*n]==0);
    std::uint32_t a=static_cast<std::uint32_t>(value),b=0;
    const bool compare=kind==1 && value!=0;
    if(compare) b=100;
    std::uint32_t result=compare?a-b:a;
    unsigned flags=(result==0?0x40:0)|(result>>31?0x80:0);
    unsigned low=result&255,ones=0;for(unsigned n=0;n<8;++n) ones+=(low>>n)&1;
    if(!(ones&1)) flags|=4;
    if(compare) {if(a<b) flags|=1;if(((a^b)&(a^result))>>31) flags|=0x800;}
    if(kind==2 && value>=0) flags=0x44; // Native CMP stack byte0,0.
    assert((stormbolt_growth_seen[7]&0x8C5)==flags);
}
template<std::size_t Size>
static void verify_patch(const std::array<unsigned char,Size>& expected,void (*entry)()) {
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(page);
    std::memcpy(page+32,expected.data(),Size);page[31]=0xA5;page[32+Size]=0x5A;
    StormboltLoopRelay relay;assert(relay.prepare(reinterpret_cast<std::uintptr_t>(page+32),reinterpret_cast<std::uintptr_t>(entry)));
    StormboltLoopCallPatch<CodeWordApi,Size> patch;
    assert(patch.install(page+32,reinterpret_cast<std::uintptr_t>(relay.entry())));
    assert(page[32]==0xE8);for(std::size_t n=5;n<Size;++n) assert(page[32+n]==0x90);
    assert(page[31]==0xA5 && page[32+Size]==0x5A);
    assert(patch.restore() && !std::memcmp(page+32,expected.data(),Size));
    page[32]=0xCC;
    assert(!patch.install(page+32,reinterpret_cast<std::uintptr_t>(relay.entry())) && page[32]==0xCC);
    assert(VirtualFree(page,0,MEM_RELEASE));
}
static void verify_full_path(void* component,const void* event,const void* result) {
    auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(page);
    std::memset(page,0x90,4096);
    auto emit=[&](std::size_t at,std::initializer_list<unsigned char> bytes) {for(auto b:bytes) page[at++]=b;};
    auto relative=[&](std::size_t at,std::size_t destination) {
        auto offset=static_cast<std::int32_t>(destination-at-4);std::memcpy(page+at,&offset,4);
    };
    // Exact relative positions of PC121B37F / B39E / B3A7 / B41D.
    constexpr std::size_t first=32,post=first+0x1F,remaining=first+0x28,epilogue=first+0x9E;
    for(auto pair:{std::array<std::size_t,2>{0,first},{256,remaining},{288,epilogue}}) {
        emit(pair[0],{0x48,0x83,0xEC,8,0xE9});relative(pair[0]+5,pair[1]);
    }
    const std::array<unsigned char,8> a{{0x85,0xF6,0x0F,0x88,0x96,0,0,0}};
    const std::array<unsigned char,9> b{{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76}};
    const std::array<unsigned char,12> c{{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0}};
    std::memcpy(page+first,a.data(),8);std::memcpy(page+post,b.data(),9);std::memcpy(page+remaining,c.data(),12);
    emit(first+8,{0xFF,0x05});relative(first+10,512); // Stand-in for native effect900.
    emit(remaining+12,{0xFF,0x05});relative(remaining+14,516); // Later result handling.
    emit(epilogue,{0x48,0x83,0xC4,8,0xC3});
    HookTransaction transaction;assert(transaction.begin()==MH_OK);
    StormboltOwnedReplayCallSite<8> p0(&transaction);
    StormboltOwnedReplayCallSite<9> p1(&transaction);
    StormboltOwnedReplayCallSite<12> p2(&transaction);
    assert(p0.prepare(page+first,reinterpret_cast<std::uintptr_t>(&stormbolt_growth_negative_entry_span),a));
    assert(p1.prepare(page+post,reinterpret_cast<std::uintptr_t>(&stormbolt_growth_post_effect_entry_span),b));
    assert(p2.prepare(page+remaining,reinterpret_cast<std::uintptr_t>(&stormbolt_growth_remaining_entry_span),c));
    assert(!std::memcmp(page+first,a.data(),8) && !std::memcmp(page+post,b.data(),9) && !std::memcmp(page+remaining,c.data(),12));
    assert(transaction.commit_pending_without_rollback()==MH_OK);
    assert(p0.activate() && p1.activate() && p2.activate());
    assert(!std::memcmp(page+first+5,a.data()+5,3) && !std::memcmp(page+post+5,b.data()+5,4) && !std::memcmp(page+remaining+5,c.data()+5,7));
    native_effect=reinterpret_cast<unsigned*>(page+512);auto* downstream=reinterpret_cast<unsigned*>(page+516);
    for(unsigned mode=0;mode<3;++mode) for(int value:{-1,0,1,99,100,101}) {
        *native_effect=0;*downstream=0;calls=0;
        expected_native=unsigned(mode==0 && value>=0);
        stormbolt_growth_test_path(component,event,result,value,page+(mode==0?0:mode==1?256:288));
        assert(*native_effect==expected_native && calls==unsigned(mode!=2));
        assert(*downstream==unsigned(mode!=2 && value>=0 && (mode==1 || (value!=0 && value!=100))));
    }
    assert(p2.restore() && p1.restore() && p0.restore());
    for(unsigned mode=0;mode<3;++mode) for(int value:{-1,0,1,99,100,101}) {
        *native_effect=0;*downstream=0;calls=0;
        stormbolt_growth_test_path(component,event,result,value,page+(mode==0?0:mode==1?256:288));
        assert(calls==0 && *native_effect==unsigned(mode==0 && value>=0));
    }
    assert(transaction.stop()==MH_OK);
    assert(p0.release_after_quiescence() && p1.release_after_quiescence() && p2.release_after_quiescence());
    native_effect=nullptr;assert(VirtualFree(page,0,MEM_RELEASE));
}
int main() {
    for(auto entry:{&stormbolt_growth_negative_entry,&stormbolt_growth_post_effect_entry,&stormbolt_growth_remaining_entry}) {
        auto address=reinterpret_cast<DWORD64>(entry);DWORD64 image=0;
        auto* function=RtlLookupFunctionEntry(address,&image,nullptr);assert(function);
        alignas(16) std::array<std::uint64_t,32> stack{};
        auto base=reinterpret_cast<DWORD64>(stack.data());stack[26]=0x12345678;stack[27]=0x87654321;
        auto* unwind=reinterpret_cast<const unsigned char*>(image+function->UnwindData);
        CONTEXT context{};context.Rip=address+unwind[1];context.Rsp=base;context.Rbp=base+0xD0;
        void* handler=nullptr;DWORD64 frame=0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,image,context.Rip,function,&context,&handler,&frame,nullptr);
        assert(context.Rip==0x87654321 && context.Rbp==0x12345678 && context.Rsp==base+0xE0);
    }
    verify_patch<9>({{0x85,0xF6,0x74,0x7B,0x83,0xFE,0x64,0x74,0x76}},&stormbolt_growth_post_effect_entry);
    verify_patch<12>({{0x85,0xF6,0x78,0x72,0x80,0xBC,0x24,0x28,0x01,0,0,0}},&stormbolt_growth_remaining_entry);
    int component=0,event=0,result=0;
    expected_component=&component;expected_event=&event;expected_result=&result;
    stormbolt_growth_dispatch_target=&stormbolt_growth_clobber_callback;
    for(int value:{-1,0,1,99,100,101}) {
        calls=0;assert(stormbolt_growth_test_negative(&component,&event,&result,value)==(value<0));assert(calls==unsigned(value<0));verify(value,0);
        calls=0;assert(stormbolt_growth_test_post_effect(&component,&event,&result,value)==(value==0 || value==100));assert(calls==unsigned(value==0 || value==100));verify(value,1);
        calls=0;assert(stormbolt_growth_test_remaining(&component,&event,&result,value)==(value<0));assert(calls==1);verify(value,2);
    }
    verify_full_path(&component,&event,&result);
    std::puts("PASS: three growth entry conditions, component/event/result ABI and original branch destinations, volatile GPR/XMM preservation, native flags, unwind and 9/12-byte CALL restoration, patched three-site paths and native-effect order");
}
