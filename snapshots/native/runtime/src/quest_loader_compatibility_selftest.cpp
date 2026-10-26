#ifdef NDEBUG
#undef NDEBUG
#endif
#include "quest_loader_compatibility.h"
#include "hooks.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>

namespace {
struct Relay {std::uintptr_t address;std::array<unsigned char,14> data;};
std::vector<Relay> relays;
bool read(std::uintptr_t address,void* output,std::size_t bytes,void*) {
 for(const auto& relay:relays)if(address==relay.address && bytes==relay.data.size()) {std::memcpy(output,relay.data.data(),bytes);return true;}
 return false;
}
using Fn=int(*)();Fn original=nullptr,nativeBody=nullptr;
int foreignHook(){return nativeBody()+7;}
int projectHook(){return original()+3;}
}
int main() {
 constexpr std::uintptr_t image=0x140000000,loader=0x180000000;
 std::vector<unsigned char> native(0x1b66000,0xcc);
 for(const auto& signature:stormslinger::questSignatures) {
  const char* bit=signature.bits;std::size_t n=0;
  while(*bit) {while(*bit==' ')++bit;if(!*bit)break;unsigned value=0;
   for(unsigned b=0;b<8;++b,++bit)value=(value<<1)|(*bit=='1');
   native[signature.rva+n++]=static_cast<unsigned char>(value);
  }
 }
 auto copy=native;unsigned count=99;
 assert(stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,0,&read,nullptr,count) && count==0 && copy==native);
 struct Site {unsigned signature,hook;};
 constexpr Site sites[]={{0,0x15fb0},{1,0x15e90},{2,0x15f60},{6,0x16050},{7,0x16000}};
 for(unsigned i=0;i<5;++i) {
  auto rva=stormslinger::questSignatures[sites[i].signature].rva;
  Relay relay{image+0x2000000+i*32,{0xff,0x25,0,0,0,0}};
  const auto destination=loader+sites[i].hook;std::memcpy(relay.data.data()+6,&destination,8);relays.push_back(relay);
  copy[rva]=0xe9;const auto displacement=static_cast<std::int32_t>(relay.address-image-rva-5);std::memcpy(copy.data()+rva+1,&displacement,4);
 }
 const auto detoured=copy;
 assert(stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,loader,&read,nullptr,count) && count==5 && copy==native);
 copy=detoured;assert(!stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,0,&read,nullptr,count));
 copy=detoured;copy[stormslinger::questSignatures[0].rva+6]^=1;
 assert(!stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,loader,&read,nullptr,count));
 copy=detoured;relays[0].data[6]^=1;
 assert(!stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,loader,&read,nullptr,count));
 relays[0].data[6]^=1;relays[0].data[0]=0x90;copy=detoured;
 assert(!stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),0,image,loader,&read,nullptr,count));
 // A separately owned existing detour must run underneath the new hook and
 // remain byte-exact when our transaction stops; never rewrite its relay.
 auto* page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));assert(page);
 std::memset(page,0x90,4096);page[64]=0xb8;page[65]=11;std::memset(page+66,0,3);page[69]=0xc3;nativeBody=reinterpret_cast<Fn>(page+64);
 page[0]=0xe9;const std::int32_t displacement=128-5;std::memcpy(page+1,&displacement,4);
 const unsigned char branch[]={0xff,0x25,0,0,0,0};std::memcpy(page+128,branch,6);
 const auto destination=reinterpret_cast<std::uintptr_t>(&foreignHook);std::memcpy(page+134,&destination,8);
 std::array<unsigned char,160> before{};std::memcpy(before.data(),page,before.size());
 stormslinger::HookTransaction transaction;assert(transaction.begin()==MH_OK);
 assert(transaction.add(page,reinterpret_cast<void*>(&projectHook),reinterpret_cast<void**>(&original))==MH_OK);
 assert(transaction.commit()==MH_OK);Fn volatile entry=reinterpret_cast<Fn>(page);assert(entry()==21);
 assert(transaction.stop()==MH_OK && entry()==18 && !std::memcmp(page,before.data(),before.size()));
 assert(VirtualFree(page,0,MEM_RELEASE));
 std::puts("PASS known five QuestLoader validation detours; unknown/tail/relay mismatch rejection; chained original and external hook restoration");
}
