#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_callback.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace fs=std::filesystem;
struct NativeRequest {
    std::array<unsigned char,0x48> header{};
    horizon_other::NoticeNativeFunction callback;
    std::array<unsigned char,8> tail{};
};
static_assert(offsetof(NativeRequest,callback)==0x48 && sizeof(NativeRequest)==0x90);
unsigned observed=0;horizon_other::NoticeCompletion last{};
void observer(const horizon_other::NoticeCompletion& value) noexcept {++observed;last=value;}
int main(int argc,char** argv) {
    using namespace horizon_other;
    const fs::path root=argc>1?fs::path(argv[1]):fs::path(L"research/horizon-complete-notice-callback-fixture-20261021");
    CHECK(!fs::exists(root));CHECK(fs::create_directory(root));
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1ad0000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
    std::ifstream native("research/horizon-complete-notice-copy-owner.bin",std::ios::binary);
    CHECK(native.read(reinterpret_cast<char*>(image+0x1acf140),169));
    CHECK(FlushInstructionCache(GetCurrentProcess(),image+0x1acf140,169));
    const auto copy=reinterpret_cast<void*(*)(void*,const void*)>(image+0x1acf140);
    {
        const auto path=root/L"confirmed.version";
        auto state=std::make_shared<NoticeAcknowledgement>(path.wstring(),"v1");
        std::weak_ptr<NoticeAcknowledgement> weak=state;
        NativeRequest original,queued,active;
        original.callback=make_notice_callback(state,&observer);CHECK(original.callback && state->pending());
        CHECK(!make_notice_callback(state,&observer) && state->pending());
        CHECK(copy(&queued,&original)==&queued);CHECK(copy(&active,&queued)==&active);
        original.callback={};queued.callback={};state.reset();
        CHECK(!weak.expired() && !fs::exists(path));
        const int selected=0,result=0;
        active.callback(selected,result);
        CHECK(observed==1 && last.confirmed && last.persisted && fs::exists(path));
        active.callback(selected,result);CHECK(observed==1);
        active.callback={};CHECK(weak.expired());
        NoticeAcknowledgement loaded(path.wstring(),"v1");CHECK(loaded.seen());
    }
    {
        const auto path=root/L"dropped.version";
        auto state=std::make_shared<NoticeAcknowledgement>(path.wstring(),"v1");
        NativeRequest original,queued;
        original.callback=make_notice_callback(state,&observer);CHECK(copy(&queued,&original)==&queued);
        original.callback={};CHECK(state->pending() && observed==1);
        queued.callback={};CHECK(!state->pending() && !state->seen() && !state->should_offer());
        CHECK(observed==2 && !last.confirmed && !fs::exists(path));
        NoticeAcknowledgement next(path.wstring(),"v1");CHECK(next.should_offer());
    }
    {
        const auto path=root/L"cancelled.version";
        auto state=std::make_shared<NoticeAcknowledgement>(path.wstring(),"v1");
        NativeRequest original,queued;
        original.callback=make_notice_callback(state,&observer);CHECK(copy(&queued,&original)==&queued);
        const int selected=1,result=0;queued.callback(selected,result);
        CHECK(observed==3 && !last.confirmed && !fs::exists(path));
        original.callback={};queued.callback={};CHECK(observed==3 && !state->pending());
    }
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    std::cout<<"PASS native request-copy callback ABI, two deferred copies, producer lifetime release, exact-once observation, cancellation and final-copy discard. Native copy constructor executed; windows, input and real user profile are not used.\n";
}
