#include <vector>
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
#include "quest_loader_compatibility.h"
static bool ReadQuestCompatibility(std::uintptr_t address,void* output,std::size_t bytes,void*) {
 SIZE_T read=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),output,bytes,&read) && read==bytes;
}
#endif
extern "C" int StormslingerCheckExecutable(const wchar_t*,char*,size_t);
extern "C" int StormslingerBuildQuestPlan(const unsigned char*,size_t,unsigned long long,QuestPlan*);
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
static std::uintptr_t AuthenticatedQuestLoader() {
 const auto loader=GetModuleHandleW(L"QuestLoader.dll");
 std::array<wchar_t,32768> path{};char hash[65]{};
 const DWORD used=loader?GetModuleFileNameW(loader,path.data(),static_cast<DWORD>(path.size())):0;
 if(!used || used>=path.size() || StormslingerCheckExecutable(path.data(),hash,sizeof(hash))<0 ||
     strcmp(hash,"97380a19c12822c318ebc7ef09df601823cbf33ec674e1aee9f8a690d5422c08"))return 0;
 return reinterpret_cast<std::uintptr_t>(loader);
}
#endif
static int ValidateCurrentGame(void* image, const QuestPlan* supplied) {
 std::array<wchar_t,32768> path{};char digest[65]{};
 DWORD used=GetModuleFileNameW(nullptr,path.data(),static_cast<DWORD>(path.size()));
 if(!used || used>=path.size() || StormslingerCheckExecutable(path.data(),digest,sizeof(digest))!=0)return -10;
 auto base=static_cast<unsigned char*>(image);
 auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>4096)return -11;
 auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt->OptionalHeader.SizeOfImage!=89993216)return -11;
 auto sections=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
  if(memcmp(sections[i].Name,".text\0\0\0",8))continue;
  size_t rva=sections[i].VirtualAddress,length=sections[i].Misc.VirtualSize;
  if(!length || rva>=nt->OptionalHeader.SizeOfImage || length>nt->OptionalHeader.SizeOfImage-rva)return -11;
  std::vector<unsigned char> copy(length,0);
  size_t pos=0;
  while(pos<length){
   MEMORY_BASIC_INFORMATION info{};auto current=base+rva+pos;
   if(!VirtualQuery(current,&info,sizeof(info)))return -12;
   auto regionEnd=static_cast<unsigned char*>(info.BaseAddress)+info.RegionSize;
   size_t chunk=static_cast<size_t>(regionEnd-current);if(!chunk)return -12;
   if(chunk>length-pos)chunk=length-pos;
   if(info.State==MEM_COMMIT && !(info.Protect&(PAGE_NOACCESS|PAGE_GUARD))){
    SIZE_T read=0;if(!ReadProcessMemory(GetCurrentProcess(),current,copy.data()+pos,chunk,&read) || read!=chunk)return -12;
   }
   pos+=chunk;
  }
  QuestPlan live{};
  if(StormslingerBuildQuestPlan(copy.data(),copy.size(),rva,&live)) {
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
   const auto loader=AuthenticatedQuestLoader();
   // The hashing API returns 1 for a readable file whose hash is not the game
   // executable; its digest is still complete. No unknown plugin is accepted.
   if(!loader)return -13;
   unsigned normalized=0;
   if(!stormslinger::normalize_known_quest_loader(copy.data(),copy.size(),rva,reinterpret_cast<std::uintptr_t>(image),
       loader,&ReadQuestCompatibility,nullptr,normalized) || !normalized ||
       StormslingerBuildQuestPlan(copy.data(),copy.size(),rva,&live))return -13;
#else
   return -13;
#endif
  }
  if(memcmp(&live,supplied,sizeof(live)))return -14;
  return 0;
 }
 return -11;
}
