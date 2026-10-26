#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_quest_mapping.h"
#include <cassert>
#include <cstdio>
#include <vector>
namespace {
using Count=int(*)();using At=int(*)(void*,int);
Count nativeCount=nullptr;At nativeAt=nullptr;
int foreignCount(){return nativeCount()+1;}
int foreignAt(void* context,int index){return index>=nativeCount()?90001:nativeAt(context,index);}
void jump(unsigned char* at,std::uintptr_t destination) {
 const unsigned char opcode[]={0xff,0x25,0,0,0,0};std::memcpy(at,opcode,6);std::memcpy(at+6,&destination,8);
}
void relative(unsigned char* at,const void* destination) {
 at[0]=0xe9;const auto delta=reinterpret_cast<std::intptr_t>(destination)-reinterpret_cast<std::intptr_t>(at+5);
 assert(delta>=INT32_MIN && delta<=INT32_MAX);const auto value=static_cast<std::int32_t>(delta);std::memcpy(at+1,&value,4);
}
}
int main() {
 auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x5400000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
 auto* loader=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x80000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(image && loader);
 const auto candidate=horizon_other::quest_mapping_candidate();assert(candidate.original_count==352 && candidate.count==358);
 std::memcpy(image+0x324fac0,candidate.entries,candidate.original_count*4);*reinterpret_cast<int*>(image+0x324fac0+352*4)=-1;
 using namespace stormslinger::quest_mapping_detail;
 for(const auto& ref:kMappingReferences)std::memcpy(image+ref.rva,ref.expected,ref.size);
 // Executable native getters with the authentic overwritten prefixes/LEAs.
 const unsigned char atPrefix[]={0x48,0x63,0xc2},countPrefix[]={0x33,0xc0};
 std::memcpy(image+0x1355c40,atPrefix,3);std::memcpy(image+0x1355c50,countPrefix,2);
 const unsigned char atBody[]={0x8b,0x04,0x81,0xc3};std::memcpy(image+0x1355c4a,atBody,sizeof(atBody));
 const unsigned char countBody[]={0x83,0x3c,0x81,0xff,0x74,0x04,0xff,0xc0,0xeb,0xf6,0xc3};std::memcpy(image+0x1355c59,countBody,sizeof(countBody));
 auto* table=loader+0x77000;*reinterpret_cast<void**>(loader+0x776e0)=table;*reinterpret_cast<unsigned*>(loader+0x776ec)=2;
 jump(loader+0x15ed0,reinterpret_cast<std::uintptr_t>(&foreignAt));jump(loader+0x15f20,reinterpret_cast<std::uintptr_t>(&foreignCount));
 struct Site {unsigned function,offset,hook;};constexpr Site sites[]={{0x1355c40,3,0x15ed0},{0x1355c50,2,0x15f20}};
 unsigned char* trampolines[2]{};
 for(unsigned i=0;i<2;++i) {
  const auto& site=sites[i];auto* trampoline=image+0x5001000+i*64;auto* relay=image+0x5000000+i*32;trampolines[i]=trampoline;
  std::memcpy(trampoline,image+site.function,site.offset+7);
  const auto displacement=static_cast<std::int32_t>((image+0x324fac0)-(trampoline+site.offset+7));std::memcpy(trampoline+site.offset+3,&displacement,4);
  jump(trampoline+site.offset+7,reinterpret_cast<std::uintptr_t>(image+site.function+site.offset+7));
  jump(relay,reinterpret_cast<std::uintptr_t>(loader+site.hook));relative(image+site.function,relay);
  auto* entry=table+i*0x38;*reinterpret_cast<void**>(entry)=image+site.function;*reinterpret_cast<void**>(entry+8)=relay;*reinterpret_cast<void**>(entry+16)=trampoline;entry[0x20]=2;
 }
 nativeAt=reinterpret_cast<At>(trampolines[0]);nativeCount=reinterpret_cast<Count>(trampolines[1]);
 At volatile get=reinterpret_cast<At>(image+0x1355c40);Count volatile count=reinterpret_cast<Count>(image+0x1355c50);
 assert(nativeCount()==352 && count()==353 && get(nullptr,352)==90001);
 std::vector<unsigned char> beforeImage(image,image+0x5400000),beforeLoader(loader,loader+0x80000);
 stormslinger::StormboltQuestMapping mapping;
 assert(mapping.prepare(image,reinterpret_cast<std::uintptr_t>(loader)) && !mapping.references_active());
 assert(!mapping.prepare(image+1,reinterpret_cast<std::uintptr_t>(loader)));
 assert(!std::memcmp(image,beforeImage.data(),beforeImage.size()) && !std::memcmp(loader,beforeLoader.data(),beforeLoader.size()));
 stormslinger::HookTransaction transaction;assert(transaction.begin()==MH_OK && mapping.queue(transaction)==MH_OK && transaction.commit()==MH_OK);
 assert(mapping.references_active() && nativeCount()==358 && count()==359);
 assert(get(nullptr,355)==67802 && get(nullptr,356)==67803 && get(nullptr,358)==90001);
 assert(!std::memcmp(loader,beforeLoader.data(),beforeLoader.size()));
 assert(transaction.stop()==MH_OK && !mapping.references_active());
 assert(count()==353 && get(nullptr,352)==90001 && !std::memcmp(image,beforeImage.data(),beforeImage.size()));
 // A changed trampoline target must not be accepted as a relocated native LEA.
 trampolines[0][6]^=1;stormslinger::StormboltQuestMapping rejected;
 assert(!rejected.prepare(image,reinterpret_cast<std::uintptr_t>(loader)));trampolines[0][6]^=1;
 image[0x324fac0]^=1;stormslinger::StormboltQuestMapping changedTable;
 assert(!changedTable.prepare(image,reinterpret_cast<std::uintptr_t>(loader)));image[0x324fac0]^=1;
 assert(VirtualFree(image,0,MEM_RELEASE) && VirtualFree(loader,0,MEM_RELEASE));
 std::puts("PASS QuestLoader 2 relocated LEAs + 19 native references; native352 preserved, project355/356 and external quest coexist; exact restore and changed target/table rejection");
}
