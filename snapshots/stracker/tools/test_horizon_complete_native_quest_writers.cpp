#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <fstream>
#include <cstring>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_quest_mapping.h"
namespace {
struct MappingReference {std::uint32_t rva;std::uint8_t size,displacement_offset;std::uint32_t table_index;std::uint8_t expected[7];};
#include "../staging/Horizon-complete-integration-20261021/src/generated_quest_mapping_references.inc"
}
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
int main() {
 using namespace horizon_other;
 auto* image=static_cast<std::uint8_t*>(VirtualAlloc(nullptr,0x55d3000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));CHECK(image);
 const auto candidate=quest_mapping_candidate();
 std::memcpy(image+0x324fac0,candidate.entries,candidate.original_count*4);
 const std::int32_t terminal=-1;std::memcpy(image+0x324fac0+candidate.original_count*4,&terminal,4);
 for(const auto& r:kMappingReferences)std::memcpy(image+r.rva,r.expected,r.size);
 std::ifstream complete_source("research/horizon-complete-native-complete-fixture.bin",std::ios::binary);
 std::ifstream open_source("research/horizon-complete-native-open-fixture.bin",std::ios::binary);
 CHECK(complete_source.read(reinterpret_cast<char*>(image+0x1369540),0x131));
 CHECK(open_source.read(reinterpret_cast<char*>(image+0x13697a0),0x114));
 auto enter=&EnterCriticalSection;auto leave=&LeaveCriticalSection;
 std::memcpy(image+0x2e13000,&enter,sizeof(enter));std::memcpy(image+0x2e13008,&leave,sizeof(leave));
 // The engine observer is outside this writer test. Returning false suppresses
 // its optional downstream action; native bit/counter paths stay unchanged.
 const unsigned char no_observer[]={0x31,0xc0,0xc3};std::memcpy(image+0x5dc320,no_observer,sizeof(no_observer));
 PreparedQuestMapping mapping;CHECK(mapping.prepare(image).status==MappingRedirectStatus::prepared);
 auto publish=[&](bool after) {
  for(std::size_t i=0;i<mapping.redirect_count();++i) {
   const auto& e=mapping.redirects()[i];std::memcpy(image+e.displacement_rva,after?e.after:e.before,4);
  }
  return FlushInstructionCache(GetCurrentProcess(),image,0x55d3000)!=FALSE;
 };
 CHECK(publish(true));CHECK(quest_mapping_references_active(mapping));
 std::vector<std::uint8_t> character(0x149000);
 auto* lock=reinterpret_cast<CRITICAL_SECTION*>(character.data()+8);InitializeCriticalSection(lock);
 using Writer=void(*)(void*,std::int32_t);
 auto write_open=reinterpret_cast<Writer>(image+0x13697a0);
 auto write_clear=reinterpret_cast<Writer>(image+0x1369540);
 auto word=[&](std::size_t offset){std::uint32_t v;std::memcpy(&v,character.data()+offset,4);return v;};
 auto bit=[&](std::size_t offset,std::size_t index){return (word(offset+(index/32)*4)&(std::uint32_t(1)<<(index%32)))!=0;};
 for(std::size_t index=352;index<358;++index) {
  const auto id=candidate.entries[index];
  CHECK(!bit(0x104bd8,index) && !bit(0x104c20,index) && word(0x1484b4+index*4)==0);
  write_open(character.data(),id);
  CHECK(bit(0x104bd8,index) && !bit(0x104c20,index) && word(0x1484b4+index*4)==0);
  write_clear(character.data(),id);
  CHECK(bit(0x104c20,index) && word(0x1484b4+index*4)==1);
  write_clear(character.data(),id);
  CHECK(bit(0x104c20,index) && word(0x1484b4+index*4)==2);
 }
 // Compare only persistent fields, not transient Windows lock bookkeeping.
 auto persistent=[&](){std::vector<std::uint8_t> v;
  for(const auto pair:{std::pair<std::size_t,std::size_t>{0x104bd8,64},{0x104c20,64},{0x1484b4,2048}})
   v.insert(v.end(),character.begin()+pair.first,character.begin()+pair.first+pair.second);
  return v;};
 const auto before_unknown=persistent();write_open(character.data(),999999);write_clear(character.data(),999999);CHECK(persistent()==before_unknown);
 write_open(character.data(),candidate.entries[0]);write_clear(character.data(),candidate.entries[0]);
 CHECK(bit(0x104bd8,0) && bit(0x104c20,0) && word(0x1484b4)==1);
 for(std::size_t i=1;i<352;++i)CHECK(!bit(0x104bd8,i) && !bit(0x104c20,i) && word(0x1484b4+i*4)==0);
 CHECK(publish(false));const auto restored=persistent();
 write_open(character.data(),67804);write_clear(character.data(),67804);CHECK(persistent()==restored);
 DeleteCriticalSection(lock);CHECK(VirtualFree(image,0,MEM_RELEASE));
 std::cout<<"PASS exact PC open/clear writer bodies: six appended slots, repeated clear count, original prefix, unrelated ID, mapping restoration. Engine observer stubbed; no disk save or quest-result caller tested.\n";
}
