#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_native.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace fs=std::filesystem;
using namespace horizon_other;
NoticeRequest captured;
unsigned submitted=0,title_requests=0;
const void* title_descriptor=nullptr;
void capture(void*,const NoticeRequest* request) {captured=*request;++submitted;}
void title_request(void*,const void* descriptor) {++title_requests;title_descriptor=descriptor;}
unsigned label_calls=0;
void* labeled_node=nullptr;
const char* labeled_text=nullptr;
void* body_node=nullptr;
unsigned body_reflows=0;
bool body_reflow_order=true;
void set_label(void* node,const char* text) {
    if(node==body_node) {
        ++body_reflows;
        auto* b=static_cast<unsigned char*>(node);
        float x=0,y=0;std::uint32_t cp=0;
        std::memcpy(&x,b+0xa0,4);std::memcpy(&y,b+0xa4,4);std::memcpy(&cp,b+0xc0,4);
        std::array<float,2> size{};std::memcpy(size.data(),b+0xf8,8);
        body_reflow_order=body_reflow_order && b[0x150]==0 && (cp&15)==0 && x==-330.0f && y==-260.0f &&
            size[0]==660.0f && size[1]==460.0f && text==captured.text;
    } else {++label_calls;labeled_node=node;labeled_text=text;}
}
void* large_frame=nullptr;
bool allow_frame=true;
unsigned invalidated=0;
void invalidate(void*) {++invalidated;}
void* find_frame(const void*,const char* name) {
    return allow_frame && !std::strcmp(name,"a_window_subG") ? large_frame : nullptr;
}
template<class T>void put(void* base,std::size_t offset,T value) {
    std::memcpy(static_cast<unsigned char*>(base)+offset,&value,sizeof(value));
}
int main(int argc,char** argv) {
    const fs::path directory=argc>1?fs::path(argv[1]):fs::path("research/horizon-complete-notice-native-fixture-20261021");
    CHECK(!fs::exists(directory) && fs::create_directory(directory));
    CHECK(stormslinger::runtime_log::initialize_directory((directory/L"logs").c_str()));
    constexpr std::size_t size=0x51c6000;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
    const char* sources[]={"24c740","1ae2050","1acf140","1ad5430","1d8e120","18c48a0","2343240","2340980",
        "23537a0","234f3c0","234f3e0","234f7d0","234f7f0","2345170","2343bc0",
        "23445f0","2344610","2342cd0"};
    std::size_t n=0;
    for(const auto& owner:notice_detail::kNoticeNativeOwners) {
        std::ifstream file(std::string("research/horizon-complete-notice-native-span-")+sources[n++]+".bin",std::ios::binary);
        CHECK(file.read(reinterpret_cast<char*>(image+owner.rva),owner.bytes));
    }
    std::vector<unsigned char> logo(0x200),gui(0x15000),message(0x2d00),settings(0x20000),app_state(0x7c000);
    put(logo.data(),0,reinterpret_cast<std::uintptr_t>(image+0x2f0f3b0));
    put(logo.data(),0x50,std::uint32_t(22));put(logo.data(),0xa0,std::uint32_t(2));
    put(logo.data(),0x54,std::uint8_t(1)); // Original optional audio-setting branch skipped.
    put(image,0x51c4640,gui.data());put(gui.data(),0x4c,std::uint32_t(8));
    put(gui.data(),0x13d60,message.data());put(message.data(),0xc,std::uint8_t(2));
    std::array<std::array<unsigned char,0x200>,5> nodes{};
    const std::size_t slots[]={0x2910,0x28f0,0x2938,0x2908,0x2940};
    for(std::size_t i=0;i<nodes.size();++i) {
        put(message.data(),slots[i],nodes[i].data());
        put(nodes[i].data(),0xc0,1.0f);put(nodes[i].data(),0xc4,1.0f);
        put(nodes[i].data(),0xb4,78.0f);
        put(nodes[i].data(),0x14c,std::uint32_t(4));
        put(nodes[i].data(),0xf8,std::array<float,2>{580.0f,380.0f});
        put(nodes[i].data(),0x150,std::uint8_t(17));
    }
    std::array<unsigned char,0x200> frame{};
    body_node=nodes[2].data();
    std::array<void*,20> text_table{};text_table[19]=reinterpret_cast<void*>(&invalidate);
    put(nodes[2].data(),0,text_table.data());
    put(nodes[2].data(),0xc0,std::uint32_t(0xabc5));
    put(nodes[2].data(),0xa0,12.0f);put(nodes[2].data(),0xa4,-7.0f);
    large_frame=frame.data();
    put(frame.data(),0xb0,-319.0f);put(frame.data(),0xb4,-250.0f);
    put(frame.data(),0xc0,38.3f);put(frame.data(),0xc4,12.4f);
    put(image,0x5013950,settings.data());put(image,0x51c4480,app_state.data());
    std::array<void*,11> app_table{};app_table[10]=reinterpret_cast<void*>(&title_request);
    auto* app_table_ptr=app_table.data();put(image,0x51c42b8,&app_table_ptr);
    put(image,0x3edb520,std::int32_t(2560));put(image,0x3edb524,std::int32_t(1600));
    CHECK(FlushInstructionCache(GetCurrentProcess(),image,size));
    stormslinger::HookTransaction transaction;CHECK(transaction.begin()==MH_OK);
    NoticeNativeSession notice;
    CHECK(notice.prepare_while_quiescent(image,transaction,fs::absolute(directory).wstring()));
    // Guard authentication is exercised against originals above. Only the
    // native queue submit is replaced after preparation, to avoid engine heaps.
    image[0x1ae2050]=0x48;image[0x1ae2051]=0xb8;
    const auto target=reinterpret_cast<std::uintptr_t>(&capture);
    std::memcpy(image+0x1ae2052,&target,8);image[0x1ae205a]=0xff;image[0x1ae205b]=0xe0;
    CHECK(FlushInstructionCache(GetCurrentProcess(),image+0x1ae2050,12));
    const auto label_target=reinterpret_cast<std::uintptr_t>(&set_label);
    image[0x18c48a0]=0x48;image[0x18c48a1]=0xb8;
    std::memcpy(image+0x18c48a2,&label_target,8);image[0x18c48aa]=0xff;image[0x18c48ab]=0xe0;
    CHECK(FlushInstructionCache(GetCurrentProcess(),image+0x18c48a0,12));
    image[0x23537a0]=0x48;image[0x23537a1]=0xb8;
    const auto find_target=reinterpret_cast<std::uintptr_t>(&find_frame);
    std::memcpy(image+0x23537a2,&find_target,8);image[0x23537aa]=0xff;image[0x23537ab]=0xe0;
    CHECK(FlushInstructionCache(GetCurrentProcess(),image+0x23537a0,12));
    CHECK(transaction.commit_pending_without_rollback()==MH_OK);
    CHECK(notice.activate_after_shared_commit());
    const auto update=reinterpret_cast<void(*)(void*)>(image+0x24c740);
    put(gui.data(),0x8f60,std::uint64_t(1));update(logo.data());CHECK(submitted==0 && title_requests==0);
    put(gui.data(),0x8f60,std::uint64_t(0));
    for(unsigned page=0;page<6;++page) {
        update(logo.data());CHECK(submitted==page+1 && title_requests==0);
        CHECK(captured.text==notice_page_text("chS",page));CHECK(captured.flag30==1);
        CHECK(!notice.restore_while_quiescent()); // Deferred callback pins binding.
        put(message.data(),0x2968,"ordinary native dialog");
        put(message.data(),0x29e0,std::uintptr_t(1));
        update(logo.data());CHECK(label_calls==page);
        put(message.data(),0x2968,captured.text);
        put(message.data(),0x2990,std::uint8_t(1));
        void* expected_node=message.data()+0x100;
        put(expected_node,0x14c,std::uint32_t(0));
        put(message.data(),0x2920,expected_node);
        if(page==0) {
            allow_frame=false;update(logo.data());CHECK(label_calls==0);
            CHECK(frame[0x58]==0);allow_frame=true;
        }
        update(logo.data());CHECK(submitted==page+1 && title_requests==0);
        CHECK(label_calls==page+1 && labeled_node==expected_node);
        CHECK(body_reflows==page+1 && body_reflow_order);
        CHECK(labeled_text==notice_close_text("chS"));
        std::uint32_t font=0;std::memcpy(&font,static_cast<unsigned char*>(expected_node)+0x14c,4);CHECK(font==4);
        float scale=0,y=0;
        std::memcpy(&scale,nodes[0].data()+0xc4,4);CHECK(scale==1.0f);
        std::memcpy(&y,nodes[3].data()+0xb4,4);CHECK(y==78.0f);
        std::memcpy(&y,nodes[4].data()+0xb4,4);CHECK(y==250.0f);
        std::array<float,2> body_size{};
        std::memcpy(body_size.data(),nodes[2].data()+0xf8,8);CHECK(body_size[0]==660.0f && body_size[1]==460.0f);
        CHECK(nodes[2][0x150]==0);
        float body_x=0,body_y=0;
        std::memcpy(&body_x,nodes[2].data()+0xa0,4);CHECK(body_x==-330.0f);
        std::memcpy(&body_y,nodes[2].data()+0xa4,4);CHECK(body_y==-260.0f);
        std::uint32_t anchor=0;std::memcpy(&anchor,nodes[2].data()+0xc0,4);CHECK(anchor==0xabc0);
        CHECK(invalidated==page*2+1);
        std::memcpy(&y,frame.data()+0xb4,4);CHECK(y==-310.0f);
        std::memcpy(&scale,frame.data()+0xc4,4);CHECK(scale==12.4f*(620.0f/500.0f));
        // An animation writing its old row cannot undo the owned layout, and
        // repeated application never compounds the saved frame scale.
        put(nodes[4].data(),0xb4,198.0f);
        update(logo.data());CHECK(label_calls==page+1);
        CHECK(body_reflows==page+1); // Reflow once per page, not each GUI tick.
        std::memcpy(&y,nodes[4].data()+0xb4,4);CHECK(y==250.0f);
        std::memcpy(&scale,frame.data()+0xc4,4);CHECK(scale==12.4f*(620.0f/500.0f));
        put(message.data(),0x29e0,std::uintptr_t(0));
        const int selected=0,result=0;captured.callback(selected,result);captured.callback={};
        // Native completion is earlier than the close animation. Neither the
        // font nor geometry may snap back while the old page is still fading.
        put(message.data(),0x2298,std::uint32_t(0x10));
        for(unsigned tick=0;tick<3;++tick) {
            update(logo.data());CHECK(title_requests==0 && submitted==page+1);
            CHECK(!notice.restore_while_quiescent());
            std::memcpy(&font,static_cast<unsigned char*>(expected_node)+0x14c,4);CHECK(font==4);
            std::memcpy(&y,frame.data()+0xb4,4);CHECK(y==-310.0f);
            CHECK(body_reflows==page+1);
        }
        // Only the stock idle signal permits restore and the next request (or
        // title on the last page); no arbitrary frame count or delay is used.
        put(message.data(),0x2298,std::uint32_t(0));update(logo.data());
        std::memcpy(&font,static_cast<unsigned char*>(expected_node)+0x14c,4);CHECK(font==0);
        std::memcpy(&scale,nodes[0].data()+0xc4,4);CHECK(scale==1.0f);
        std::memcpy(&y,nodes[3].data()+0xb4,4);CHECK(y==78.0f);
        std::memcpy(&y,nodes[4].data()+0xb4,4);CHECK(y==78.0f);
        std::memcpy(body_size.data(),nodes[2].data()+0xf8,8);CHECK(body_size[0]==580.0f && body_size[1]==380.0f);
        CHECK(nodes[2][0x150]==17);
        std::memcpy(&body_x,nodes[2].data()+0xa0,4);CHECK(body_x==12.0f);
        std::memcpy(&body_y,nodes[2].data()+0xa4,4);CHECK(body_y==-7.0f);
        std::memcpy(&anchor,nodes[2].data()+0xc0,4);CHECK(anchor==0xabc5);
        CHECK(invalidated==(page+1)*2);
        std::memcpy(&y,frame.data()+0xb4,4);CHECK(y==-250.0f);
        std::memcpy(&scale,frame.data()+0xc4,4);CHECK(scale==12.4f);
        CHECK(fs::exists(directory/"horizon-content.version")== (page==5));
    }
    CHECK(title_requests==1 && title_descriptor==image+0x4c981a8);
    CHECK(notice.restore_while_quiescent());CHECK(transaction.stop()==MH_OK);
    CHECK(notice.release_after_shared_stop());
    CHECK(!std::memcmp(image+0x24c740,"\x4c\x8b\xdc\x56",4));
    CHECK(VirtualFree(image,0,MEM_RELEASE));
    std::cout<<"PASS actual notice Hook transaction, original drawing-node setters and per-page reflow, compact frame, six closing phases retain layout/font until native idle, last page holds title through closing, exact restoration before next native request. Native submit/lookup/text generation/animation are fixtures; no GPU acceptance.\n";
}
