#include "weapon_tree_clone.h"
#include "hooks.h"
#include <cstdio>
int main() {
 auto fixture=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1380000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!fixture)return 1;
 auto source=fixture+0x13732f0;std::memcpy(source,stormslinger::weaponTreeCloneBytes,sizeof(stormslinger::weaponTreeCloneBytes));
 const unsigned char branch[]={0x0f,0x86,0x46,1,0,0};std::memcpy(source+0x19e,branch,6);
 stormslinger::WeaponTreeClone clone;
 source[0]^=1;if(clone.prepare(fixture))return 2;source[0]^=1;
 if(!clone.prepare(fixture) || !clone.entry())return 3;
 DWORD64 base=0;auto pc=reinterpret_cast<DWORD64>(clone.entry())+0x40;
 auto function=RtlLookupFunctionEntry(pc,&base,nullptr);if(!function || base!=reinterpret_cast<DWORD64>(clone.entry()))return 4;
 std::array<DWORD64,32> stack{};
 stack[6]=15;stack[7]=14;stack[8]=13;stack[9]=12;stack[10]=7;
 stack[11]=0x12345678;stack[13]=3;stack[14]=5;stack[15]=6;
 CONTEXT context{};context.Rip=pc;context.Rsp=reinterpret_cast<DWORD64>(stack.data());
 void* handlerData=nullptr;DWORD64 frame=0;
 RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,pc,function,&context,&handlerData,&frame,nullptr);
 if(context.Rip!=0x12345678 || context.Rsp!=reinterpret_cast<DWORD64>(stack.data()+12) || context.Rbx!=3 || context.Rbp!=5 || context.Rsi!=6 || context.Rdi!=7 || context.R12!=12 || context.R13!=13 || context.R14!=14 || context.R15!=15)return 5;
 MEMORY_BASIC_INFORMATION info{};VirtualQuery(clone.entry(),&info,sizeof(info));if(info.Protect!=PAGE_EXECUTE_READ)return 6;
 // Hook the synthetic native entry without executing any game calls.
 unsigned char original[32];std::memcpy(original,source,sizeof(original));
 DWORD sourceProtection=0;
 if(!VirtualProtect(source,4096,PAGE_EXECUTE_READ,&sourceProtection))return 12;
 stormslinger::HookTransaction transaction;void* trampoline=nullptr;
 if(transaction.begin()!=MH_OK || transaction.add(source,clone.entry(),&trampoline)!=MH_OK)return 7;
 if(std::memcmp(source,original,sizeof(original)))return 8;
 if(transaction.commit()!=MH_OK || !trampoline || !std::memcmp(source,original,5))return 9;
 if(transaction.stop()!=MH_OK || std::memcmp(source,original,sizeof(original)))return 10;
 if(!RtlLookupFunctionEntry(pc,&base,nullptr))return 11;
 std::puts("PASS: mismatch rejection, near RX clone, register unwind, queued entry redirect and exact restore; clone lifetime retained");
}
