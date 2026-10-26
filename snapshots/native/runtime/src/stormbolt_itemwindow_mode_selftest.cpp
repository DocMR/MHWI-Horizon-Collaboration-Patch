#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_itemwindow_mode.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
int main(){
 std::array<unsigned char,0x2800> fixture{};
 auto put=[&](std::size_t offset,std::uint32_t value){std::memcpy(fixture.data()+offset,&value,4);};
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));assert(code);
 // Native shape: XOR EAX,EAX; site CMP; JNE common; MOV AL,1; JMP common;
 // common is site+1D, exactly the production continuation delta.
 std::array<unsigned char,32> original{};original.fill(0x90);
 original[0]=0x31;original[1]=0xC0;
 const unsigned char cmp[]={0x41,0x83,0xB8,0xF8,0x09,0,0,0x0C};
 std::memcpy(original.data()+2,cmp,8);
 original[10]=0x75;original[11]=0x13;original[12]=0xB0;original[13]=1;
 original[14]=0xEB;original[15]=0x0F;original[31]=0xC3;
 std::memcpy(code,original.data(),original.size());FlushInstructionCache(GetCurrentProcess(),code,original.size());
 using Classify=bool(*)(void*,void*,void*);auto classify=reinterpret_cast<Classify>(code);
 HookTransaction tx;assert(tx.begin()==MH_OK);StormboltItemWindowMode patch(&tx);
 assert(patch.prepare(code+2));assert(tx.commit()==MH_OK && patch.activate());
 for(unsigned category:{11u,12u,13u})for(unsigned id:{237u,238u,239u,240u})for(unsigned ammo:{1u,40u}){
  put(0x9F8,category);put(0x9FC,id);put(0x27C8,ammo);const auto before=fixture;
  const bool expected=category==12 || (category==13 && (id==238 || id==239) && ammo==40);
  assert(classify(nullptr,nullptr,fixture.data())==expected && fixture==before);
 }
 assert(patch.restore());assert(!std::memcmp(code,original.data(),original.size()));
 put(0x9F8,13);put(0x9FC,238);put(0x27C8,40);assert(!classify(nullptr,nullptr,fixture.data()));
 assert(tx.stop()==MH_OK && patch.release_after_shared_stop());assert(VirtualFree(code,0,MEM_RELEASE));
 std::puts("PASS ItemWindow native classifier:24 real CALL cases, target ammo40 branch, ordinary CMP flags/continuation, no weapon writes and exact restore");
}
