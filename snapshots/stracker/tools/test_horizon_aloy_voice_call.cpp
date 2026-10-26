#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_aloy_voice_call.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed "<<__LINE__<<"\n";return 1;}} while(0)
unsigned calls=0;
char* capture(char* output,const char* pattern,...) {
    va_list args;va_start(args,pattern);vsnprintf(output,80,pattern,args);va_end(args);
    ++calls;return output;
}
int main() {
    using namespace horizon_other;
    constexpr std::size_t size=0x3400000;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
    for(const auto& owner:aloy_voice_detail::kAloyVoiceOwners) {
        char path[120]{};sprintf_s(path,"research/horizon-aloy-voice-owner-%x.bin",owner.rva);
        std::ifstream file(path,std::ios::binary);CHECK(file.read(reinterpret_cast<char*>(image+owner.rva),owner.bytes));
    }
    strcpy_s(reinterpret_cast<char*>(image+0x33feb28),32,"pl\\voice\\voice_c%02d_snd_bk");
    strcpy_s(reinterpret_cast<char*>(image+0x33feb08),32,"pl\\voice\\voice_%c%02d_snd_bk");
    stormslinger::HookTransaction tx;CHECK(tx.begin()==MH_OK);
    AloyVoiceCall voice(image);CHECK(voice.prepare_while_quiescent(tx));
    // Authentication uses original spans. Replace only formatter implementation
    // afterward; the genuine original CALL and MinHook CALL relay still execute.
    const auto target=reinterpret_cast<std::uintptr_t>(&capture);
    image[0x24c380]=0x48;image[0x24c381]=0xb8;
    std::memcpy(image+0x24c382,&target,8);image[0x24c38a]=0xff;image[0x24c38b]=0xe0;
    // Native caller normally owns its shadow space. A bounded fixture thunk
    // recreates that ABI and jumps into the actual authenticated CALL site.
    auto* thunk=image+0x1b47ee0;
    const unsigned char begin[]={0x48,0x83,0xec,0x28,0xe9};
    std::memcpy(thunk,begin,sizeof(begin));
    const std::int32_t delta=static_cast<std::int32_t>((image+0x1b47ff8)-(thunk+9));
    std::memcpy(thunk+5,&delta,4);
    const unsigned char end[]={0x48,0x83,0xc4,0x28,0xc3};
    std::memcpy(image+0x1b47ffd,end,sizeof(end));
    CHECK(FlushInstructionCache(GetCurrentProcess(),image,size));
    CHECK(tx.commit_pending_without_rollback()==MH_OK);CHECK(voice.activate_after_shared_commit_while_quiescent());
    using Caller=char*(*)(char*,const char*,int);
    const auto run=reinterpret_cast<Caller>(thunk);
    for(int id=0;id<16;++id) {
        char output[80]{},expected[80]{};
        if(id==11)strcpy_s(expected,"pl\\voice\\voice_f10_snd_bk");
        else sprintf_s(expected,"pl\\voice\\voice_c%02d_snd_bk",id);
        CHECK(run(output,reinterpret_cast<const char*>(image+0x33feb28),id)==output);
        CHECK(!std::strcmp(output,expected));
    }
    char output[80]{};run(output,"ordinary_%02d",11);CHECK(!std::strcmp(output,"ordinary_11"));
    CHECK(calls==17);CHECK(voice.restore_while_quiescent());
    CHECK(!std::memcmp(image+0x1b47ff8,"\xe8\x83\x43\x70\xfe",5));
    run(output,reinterpret_cast<const char*>(image+0x33feb28),11);
    CHECK(!std::strcmp(output,"pl\\voice\\voice_c11_snd_bk"));
    CHECK(tx.stop()==MH_OK);CHECK(voice.release_after_shared_stop());
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    std::cout<<"PASS original CALL relay ABI: only character11 at the owned pattern selects PS female10; other voices and patterns preserved; original CALL restored. No game writes.\n";
}
