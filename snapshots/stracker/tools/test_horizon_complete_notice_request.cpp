#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_request.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
std::array<unsigned char,0x90> captured{};
void capture(void*,const void* request) {std::memcpy(captured.data(),request,captured.size());}
int main() {
    using namespace horizon_other;
    constexpr std::size_t size=0x5040000;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
    std::ifstream source("research/horizon-complete-notice-request-defaults-owner.bin",std::ios::binary);
    CHECK(source.read(reinterpret_cast<char*>(image+0x108c870),196));
    image[0x1ae2050]=0x48;image[0x1ae2051]=0xb8;
    const auto target=reinterpret_cast<std::uintptr_t>(&capture);
    std::memcpy(image+0x1ae2052,&target,8);image[0x1ae205a]=0xff;image[0x1ae205b]=0xe0;
    CHECK(FlushInstructionCache(GetCurrentProcess(),image,size));
    // After SUB RSP,B8, [RSP+E8] is the sixth caller argument, not fifth.
    const auto native=reinterpret_cast<void(*)(void*,int,unsigned,const char*,std::uintptr_t,bool)>(image+0x108c870);
    const char* text="source-backed localized text";
    for(const auto dimensions: {std::pair<int,int>{1920,1080},{2560,1600},{3440,1440},{1279,719}}) {
        NoticeRequestDefaults defaults{dimensions.first,dimensions.second,1.25f,2.5f,3.75f};
        std::memcpy(image+0x3edb520,&defaults.logical_width,4);
        std::memcpy(image+0x3edb524,&defaults.logical_height,4);
        std::memcpy(image+0x503c430,&defaults.parameter20,4);
        std::memcpy(image+0x503c434,&defaults.parameter24,4);
        std::memcpy(image+0x503c438,&defaults.parameter28,4);
        native(nullptr,1,3,text,0,false);
        auto request=make_notice_request(defaults,text,{},3);
        const auto* bytes=reinterpret_cast<const unsigned char*>(&request);
        // The ordinary helper requests the small layout. C6 intentionally
        // changes only this independently traced native layout-selector byte.
        CHECK(captured[0x30]==0 && request.flag30==1);
        // Only original initialized/copied fields: never compare stack padding.
        for(const auto field: {std::pair<std::size_t,std::size_t>{0,4},{8,16},
                {0x20,16},{0x31,1},{0x34,16},{0x88,1}}) {
            if(std::memcmp(captured.data()+field.first,bytes+field.first,field.second)) {
                std::cerr<<"field mismatch "<<std::hex<<field.first<<"\n";
                for(std::size_t n=field.first;n<field.first+field.second;++n)
                    std::cerr<<n<<":"<<unsigned(captured[n])<<"/"<<unsigned(bytes[n])<<" ";
                std::cerr<<"\n";return 1;
            }
        }
    }
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    std::cout<<"PASS request fields match executed original initialization helper at four logical sizes; native large-window flag, single-button kind and native defaults preserved. Submission is a capture stub; no game GUI.\n";
}
