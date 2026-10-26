#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace {
int selection=-1,result=-1,calls=0,releases=0,closes=0,widget_updates=0;
void record(void*,const int* selected,const int* completed) {
 selection=*selected;result=*completed;++calls;
}
void release(void*,bool) {++releases;}
void close_window(void*,bool) {++closes;}
void update_widget(void*,bool) {++widget_updates;}
template<class T>void put(std::vector<std::uint8_t>& v,std::size_t offset,T value) {
 std::memcpy(v.data()+offset,&value,sizeof(value));
}
}
int main() {
 constexpr std::size_t image_size=0x51c5000;
 auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
 struct Body {std::uintptr_t rva;std::size_t bytes;const char* path;};
 const Body bodies[]={
  {0x1d8dca0,102,"research/horizon-complete-notice-forced-close-owner.bin"},
  {0x1d8dd10,302,"research/horizon-complete-notice-secondary-selection-owner.bin"},
  {0x1d8de40,396,"research/horizon-complete-notice-primary-selection-owner.bin"},
  {0xb82070,109,"research/horizon-complete-notice-button-primary-owner.bin"}};
 for(const auto& body:bodies) {
  std::ifstream file(body.path,std::ios::binary);
  CHECK(file.read(reinterpret_cast<char*>(image+body.rva),body.bytes));
 }
 image[0x1fb7cc0]=0x48;image[0x1fb7cc1]=0xb8;
 const auto close=reinterpret_cast<std::uintptr_t>(&close_window);
 std::memcpy(image+0x1fb7cc2,&close,8);image[0x1fb7cca]=0xff;image[0x1fb7ccb]=0xe0;
 image[0x234fa30]=0x48;image[0x234fa31]=0xb8;
 const auto update=reinterpret_cast<std::uintptr_t>(&update_widget);
 std::memcpy(image+0x234fa32,&update,8);image[0x234fa3a]=0xff;image[0x234fa3b]=0xe0;
 CHECK(FlushInstructionCache(GetCurrentProcess(),image,image_size));
 std::array<void*,5> table{};
 table[2]=reinterpret_cast<void*>(&record);table[4]=reinterpret_cast<void*>(&release);
 void** callback=table.data();
 std::vector<std::uint8_t> gui(0x2d00);
 auto reset=[&]() {
  std::fill(gui.begin(),gui.end(),std::uint8_t{0});selection=result=-1;calls=releases=closes=0;
  put(gui,0x29e0,reinterpret_cast<std::uintptr_t>(&callback));
 };
 reset();reinterpret_cast<void(*)(void*)>(image+0x1d8dca0)(gui.data());
 CHECK(selection==0 && result==2 && calls==1 && releases==1 && closes==1);
 reset();reinterpret_cast<void(*)(void*)>(image+0x1d8dd10)(gui.data());
 CHECK(selection==1 && result==0 && calls==1 && releases==1 && closes==1);
 for(int input:{0,1}) {
  reset();reinterpret_cast<void(*)(void*,int)>(image+0x1d8de40)(gui.data(),input);
  CHECK(selection==0 && result==input && calls==1 && releases==1 && closes==1);
 }
 // Original button registration sets +120 receiver and +128 selection handler.
 // Exercise that dispatcher, not a direct call to the result handler.
 std::vector<std::uint8_t> global_gui(0x14800),button(0x300);
 auto* global_gui_ptr=global_gui.data();
 std::memcpy(image+0x51c4640,&global_gui_ptr,sizeof(global_gui_ptr));
 put(button,0x120,reinterpret_cast<std::uintptr_t>(gui.data()));
 put(button,0x128,reinterpret_cast<std::uintptr_t>(image+0x1d8de40));
 for(int input:{0,1}) {
  reset();put(button,0x240,input);
  reinterpret_cast<void(*)(void*)>(image+0xb82070)(button.data());
  CHECK(selection==0 && result==input && calls==1 && releases==1 && closes==1);
 }
 reset();put(button,0x173,std::uint8_t(1));
 reinterpret_cast<void(*)(void*)>(image+0xb82070)(button.data());
 CHECK(calls==0 && releases==0 && closes==0);
 // The existing keep-open flag must not be mistaken for callback completion:
 // native code can invoke a result while retaining the visible request.
 reset();put(gui,0x29a0,std::uint32_t(1));
 std::vector<std::uint8_t> widget(0x210);
 put(gui,0x2908,reinterpret_cast<std::uintptr_t>(widget.data()));
 reinterpret_cast<void(*)(void*,int)>(image+0x1d8de40)(gui.data(),0);
 CHECK(selection==0 && result==0 && calls==1 && releases==0 && closes==0 && widget_updates==1);
 CHECK(VirtualFree(image,0,MEM_RELEASE));
 std::cout<<"PASS native notice handlers and original button dispatcher: forced-close(0,2), secondary(1,0), primary(0,input), disabled button, callback release and keep-open flag. Synthetic widgets and close only; physical input and native window not tested.\n";
}
