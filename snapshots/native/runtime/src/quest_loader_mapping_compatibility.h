#pragma once
#include "hooks.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <iterator>
namespace stormslinger {
// Host has authenticated the installed QuestLoader image. Its two copied LEAs
// are the active save-table consumers; their overwritten native bytes are not.
class QuestLoaderMappingCompatibility {
 struct Patch {unsigned char* site=nullptr;unsigned size=0,offset=0;unsigned char before[7]{};std::int32_t after=0;};
 std::array<Patch,64> patches_{};
 const std::int32_t* table_=nullptr;
 const unsigned char* image_=nullptr;
 std::size_t count_=0;
 static bool read(std::uintptr_t address,void* data,std::size_t size) {
  SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),data,size,&got) && got==size;
 }
 static bool moved_lea(const unsigned char* image,std::uintptr_t loader,unsigned function,unsigned offset,
     unsigned hook,unsigned char*& output) {
  std::uintptr_t hooks=0;unsigned count=0;
  if(!read(loader+0x776e0,&hooks,8) || !read(loader+0x776ec,&count,4) || !hooks || count>256)return false;
  std::uintptr_t trampoline=0,detour=0;unsigned found=0;
  for(unsigned i=0;i<count;++i) {
   unsigned char entry[0x38]{};if(!read(hooks+i*0x38,entry,sizeof(entry)))return false;
   std::uintptr_t target=0;std::memcpy(&target,entry,8);
   if(target!=reinterpret_cast<std::uintptr_t>(image)+function)continue;
   if(!(entry[0x20]&2))return false;
   std::memcpy(&detour,entry+8,8);std::memcpy(&trampoline,entry+16,8);++found;
  }
  if(found!=1 || !trampoline)return false;
  unsigned char native[5]{},relay[14]{},copy[24]{};
  if(!read(reinterpret_cast<std::uintptr_t>(image)+function,native,5) || native[0]!=0xe9 ||
     !read(detour,relay,14) || !read(trampoline,copy,sizeof(copy)))return false;
  std::int32_t displacement=0;std::memcpy(&displacement,native+1,4);
  if(static_cast<std::intptr_t>(reinterpret_cast<std::uintptr_t>(image)+function+5)+displacement!=static_cast<std::intptr_t>(detour))return false;
  const unsigned char indirect[]={0xff,0x25,0,0,0,0};std::uintptr_t destination=0;
  std::memcpy(&destination,relay+6,8);
  if(std::memcmp(relay,indirect,6) || destination!=loader+hook)return false;
  const unsigned char atPrefix[]={0x48,0x63,0xc2},countPrefix[]={0x33,0xc0},lea[]={0x48,0x8d,0x0d};
  if((offset==3 && std::memcmp(copy,atPrefix,3)) || (offset==2 && std::memcmp(copy,countPrefix,2)) ||
     std::memcmp(copy+offset,lea,3) || std::memcmp(copy+offset+7,indirect,6))return false;
  std::uintptr_t back=0;std::memcpy(&back,copy+offset+13,8);
  if(back!=reinterpret_cast<std::uintptr_t>(image)+function+offset+7)return false;
  std::memcpy(&displacement,copy+offset+3,4);
  if(static_cast<std::intptr_t>(trampoline+offset+7)+displacement!=reinterpret_cast<std::intptr_t>(image+0x324fac0))return false;
  output=reinterpret_cast<unsigned char*>(trampoline+offset);return true;
 }
public:
 template<class References>
 bool prepare(const unsigned char* image,std::uintptr_t loader,const References& references,
     horizon_other::QuestMappingCandidate candidate) {
  if(table_)return image==image_;
  if(!image || !loader || std::size(references)>patches_.size() || !candidate.entries || candidate.count>=1024)return false;
  std::array<std::int32_t,1024> original{};
  if(candidate.original_count>=original.size() || !read(reinterpret_cast<std::uintptr_t>(image)+0x324fac0,original.data(),(candidate.original_count+1)*4) ||
     std::memcmp(original.data(),candidate.entries,candidate.original_count*4) || original[candidate.original_count]!=-1)return false;
  unsigned char *at=nullptr,*count=nullptr;
  if(!moved_lea(image,loader,0x1355c40,3,0x15ed0,at) || !moved_lea(image,loader,0x1355c50,2,0x15f20,count))return false;
  const auto origin=(reinterpret_cast<std::uintptr_t>(image)+0x324fac0)&~std::uintptr_t(0xffff);
  void* allocation=nullptr;
  for(std::uintptr_t distance=0;distance<0x4000000 && !allocation;distance+=0x10000) {
   allocation=VirtualAlloc(reinterpret_cast<void*>(origin+distance),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
   if(!allocation && origin>=distance)allocation=VirtualAlloc(reinterpret_cast<void*>(origin-distance),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
  }
  if(!allocation)return false;
  std::memcpy(allocation,candidate.entries,(candidate.count+1)*4);
  auto fail=[&] {VirtualFree(allocation,0,MEM_RELEASE);return false;};
  std::size_t n=0;
  for(const auto& ref:references) {
   auto& patch=patches_[n++];patch.site=const_cast<unsigned char*>(image)+ref.rva;patch.size=ref.size;patch.offset=ref.displacement_offset;
   const bool moved=ref.rva==0x1355c43 || ref.rva==0x1355c52;
   if(moved)patch.site=ref.rva==0x1355c43?at:count;
   if(!read(reinterpret_cast<std::uintptr_t>(patch.site),patch.before,patch.size))return fail();
   if(moved) {
    if(patch.size!=7 || patch.offset!=3 || std::memcmp(patch.before,ref.expected,3))return fail();
   } else if(std::memcmp(patch.before,ref.expected,patch.size))return fail();
   std::int32_t old=0;std::memcpy(&old,patch.before+patch.offset,4);
   const auto next=reinterpret_cast<std::intptr_t>(patch.site)+patch.size;
   if(next+old!=reinterpret_cast<std::intptr_t>(image+0x324fac0)+ref.table_index*4)return fail();
   const auto delta=reinterpret_cast<std::intptr_t>(allocation)+ref.table_index*4-next;
   if(delta<(std::numeric_limits<std::int32_t>::min)() || delta>(std::numeric_limits<std::int32_t>::max)())return fail();
   patch.after=static_cast<std::int32_t>(delta);
  }
  DWORD previous=0;if(!VirtualProtect(allocation,4096,PAGE_READONLY,&previous))return fail();
  table_=static_cast<const std::int32_t*>(allocation);image_=image;count_=n;return true;
 }
 bool prepared() const noexcept{return table_!=nullptr;}
 bool prepared_for(const unsigned char* image) const noexcept{return table_ && image_==image;}
 MH_STATUS queue(HookTransaction& transaction) {
  if(!prepared() || !transaction.can_add_hooks())return MH_ERROR_NOT_INITIALIZED;
  for(std::size_t i=0;i<count_;++i) {
   const auto& p=patches_[i];auto status=transaction.add_rip_patch(p.site,p.before,p.size,p.offset,p.after);
   if(status!=MH_OK)return status;
  }
  return MH_OK;
 }
 bool active() const {
  if(!prepared())return false;
  for(std::size_t i=0;i<count_;++i) {std::int32_t value=0;const auto& p=patches_[i];
   if(!read(reinterpret_cast<std::uintptr_t>(p.site)+p.offset,&value,4) || value!=p.after)return false;
  }
  return true;
 }
};
}
