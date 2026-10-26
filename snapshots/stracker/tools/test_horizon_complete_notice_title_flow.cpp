#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_title_flow.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace fs=std::filesystem;
using namespace horizon_other;
struct Queue {
    void*(*copy)(void*,const void*);
    NoticeRequest pending;
    unsigned calls=0;
};
void submit(void* context,const NoticeRequest& request) {
    auto& q=*static_cast<Queue*>(context);
    q.copy(&q.pending,&request);++q.calls;
}
int main(int argc,char** argv) {
    const fs::path root=argc>1?fs::path(argv[1]):fs::path("research/horizon-complete-notice-title-flow-fixture-20261021");
    CHECK(!fs::exists(root) && fs::create_directory(root));
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1ad0000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
    std::ifstream bytes("research/horizon-complete-notice-copy-owner.bin",std::ios::binary);
    CHECK(bytes.read(reinterpret_cast<char*>(image+0x1acf140),169));
    CHECK(FlushInstructionCache(GetCurrentProcess(),image+0x1acf140,169));
    Queue queue{reinterpret_cast<void*(*)(void*,const void*)>(image+0x1acf140),{},0};
    auto ack=std::make_shared<NoticeAcknowledgement>((root/"complete.version").wstring(),"v1");
    NoticeTitleFlow flow(std::make_shared<NoticePages>(ack));
    NoticeTitleSnapshot snapshot{false,true,8,{2560,1600,1,2,3}};
    CHECK(!flow.hold_title(snapshot,&submit,&queue) && queue.calls==0);
    snapshot.before_title_request=true;snapshot.message_queue_ready=false;
    CHECK(flow.hold_title(snapshot,&submit,&queue) && queue.calls==0);
    snapshot.message_queue_ready=true;
    for(unsigned page=0;page<6;++page) {
        CHECK(flow.hold_title(snapshot,&submit,&queue));
        CHECK(queue.calls==page+1 && queue.pending.kind==1);
        CHECK(queue.pending.text==notice_page_text("chS",page));
        CHECK(queue.pending.x==1280 && queue.pending.y==800);
        CHECK(flow.hold_title(snapshot,&submit,&queue) && queue.calls==page+1);
        const int selected=0,result=0;queue.pending.callback(selected,result);
        queue.pending.callback={};
        CHECK(fs::exists(root/"complete.version")== (page==5));
    }
    CHECK(!flow.hold_title(snapshot,&submit,&queue) && queue.calls==6 && ack->seen());
    auto cancel_ack=std::make_shared<NoticeAcknowledgement>((root/"cancel.version").wstring(),"v1");
    NoticeTitleFlow cancelled(std::make_shared<NoticePages>(cancel_ack));
    CHECK(cancelled.hold_title(snapshot,&submit,&queue));
    const int selected=1,result=0;queue.pending.callback(selected,result);queue.pending.callback={};
    CHECK(!cancelled.hold_title(snapshot,&submit,&queue) && !fs::exists(root/"cancel.version"));
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    std::cout<<"PASS composed title hold, original native request copies,14-language content selection, six pages, queue wait/no duplicates, final-only acknowledgement and cancellation release. No native window or live hook.\n";
}
