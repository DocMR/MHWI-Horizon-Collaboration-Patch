#include "hooks.h"
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
using Function=int(*)();
static Function original=nullptr;
static Function sharedOriginal=nullptr;
static Function nestedOriginal=nullptr;
static int nestedReplacement() {return nestedOriginal()+5;}
static int replacement() { return original()+7; }
static int sharedReplacement() { return sharedOriginal()+3; }
static unsigned invalid_snapshot_calls=0;
static HANDLE WINAPI invalid_snapshot(DWORD,DWORD) {++invalid_snapshot_calls;SetLastError(ERROR_INVALID_HANDLE);return NULL;}
int main(int argc,char**) {
 const bool invalid_toolhelp=argc>1;
 auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
 if(!memory) return 1;
 memset(memory,0x90,4096);memory[0]=0xB8;memory[1]=11;memset(memory+2,0,3);memory[5]=0xC3;
 unsigned char before[32];memcpy(before,memory,32);
 auto function=reinterpret_cast<Function>(memory);
 if(function()!=11) return 2;
 stormslinger::HookTransaction good;
 stormslinger::HookTransaction premature;
 if(premature.begin_shared_with(good)!=MH_ERROR_NOT_INITIALIZED) return 18;
 if(good.begin()!=MH_OK) return 3;
 void* snapshot_original=nullptr;
 if(invalid_toolhelp && good.add(reinterpret_cast<void*>(&CreateToolhelp32Snapshot),
     reinterpret_cast<void*>(&invalid_snapshot),&snapshot_original)!=MH_OK) return 52;
 if(good.add(memory,reinterpret_cast<void*>(&replacement),reinterpret_cast<void**>(&original))!=MH_OK) return 4;
 if(function()!=11) return 5;
 if(good.commit()!=MH_OK || function()!=18 || original()!=11) return 6;
 stormslinger::HookTransaction shared;
 if(shared.begin_shared_with(good)!=MH_OK) return 12;
 auto second=memory+128;memcpy(second,before,32);second[1]=22;
 const auto secondFunction=reinterpret_cast<Function>(second);
 if(shared.add(second,reinterpret_cast<void*>(&sharedReplacement),reinterpret_cast<void**>(&sharedOriginal))!=MH_OK) return 13;
 if(secondFunction()!=22 || function()!=18) return 14;
 if(good.commit_pending_without_rollback()!=MH_OK || secondFunction()!=22 || function()!=18) return 23;
 if(good.commit()!=MH_OK || secondFunction()!=22 || function()!=18) return 24;
 if(shared.commit()!=MH_OK || secondFunction()!=25 || function()!=18) return 15;
 stormslinger::HookTransaction nested;
 auto third=memory+256;memcpy(third,before,32);third[1]=33;
 const auto thirdFunction=reinterpret_cast<Function>(third);
 if(nested.begin_shared_with(shared)!=MH_OK) return 31;
 if(nested.add(third,reinterpret_cast<void*>(&nestedReplacement),reinterpret_cast<void**>(&nestedOriginal))!=MH_OK) return 32;
 if(nested.commit()!=MH_OK || thirdFunction()!=38) return 33;
 if(shared.stop()!=MH_ERROR_ENABLED || good.stop()!=MH_ERROR_ENABLED || thirdFunction()!=38) return 34;
 if(nested.stop()!=MH_OK || thirdFunction()!=33 || secondFunction()!=25 || function()!=18) return 35;
 if(good.stop()!=MH_ERROR_ENABLED || function()!=18 || secondFunction()!=25) return 16;
 if(shared.stop()!=MH_OK || secondFunction()!=22 || function()!=18 || !good.has_library_state()) return 17;
 stormslinger::HookTransaction sharedFailure;
 if(sharedFailure.begin_shared_with(good)!=MH_OK) return 19;
 if(sharedFailure.add(second,reinterpret_cast<void*>(&sharedReplacement),reinterpret_cast<void**>(&sharedOriginal))!=MH_OK) return 20;
 if(sharedFailure.add(second,reinterpret_cast<void*>(&sharedReplacement),reinterpret_cast<void**>(&sharedOriginal))!=MH_ERROR_ALREADY_CREATED) return 21;
 if(sharedFailure.has_library_state() || !good.has_library_state() || function()!=18 || secondFunction()!=22) return 22;
 stormslinger::HookTransaction coordinatedFailure;
 if(coordinatedFailure.begin_shared_with(good,true)!=MH_OK) return 25;
 if(coordinatedFailure.add(second,reinterpret_cast<void*>(&sharedReplacement),reinterpret_cast<void**>(&sharedOriginal))!=MH_OK) return 26;
 if(coordinatedFailure.add(second,reinterpret_cast<void*>(&sharedReplacement),reinterpret_cast<void**>(&sharedOriginal))!=MH_ERROR_ALREADY_CREATED) return 27;
 // A mixed-state coordinator must restore its other patches before releasing
 // these trampolines. The root must still see this live borrower meanwhile.
 if(!coordinatedFailure.has_library_state() || coordinatedFailure.last.owned!=1 || sharedOriginal()!=22) return 28;
 if(good.stop()!=MH_ERROR_ENABLED || function()!=18 || secondFunction()!=22) return 29;
 if(coordinatedFailure.stop()!=MH_OK || coordinatedFailure.has_library_state() || function()!=18 || secondFunction()!=22) return 30;
 // An exact CALL joins its owner's queue without changing stack semantics
 // to a JMP detour. A committed parent cannot enable its child's queue.
 auto caller=memory+512;auto callee=memory+608;auto detour=memory+672;
 const unsigned char stackIn[]={0x48,0x83,0xEC,0x28},stackOut[]={0x48,0x83,0xC4,0x28,0xC3};
 memcpy(caller,stackIn,4);caller[4]=0xE8;
 const auto displacement=static_cast<std::int32_t>(callee-(caller+9));
 memcpy(caller+5,&displacement,4);memcpy(caller+9,stackOut,5);
 memcpy(callee,before,32);memcpy(detour,before,32);detour[1]=73;
 unsigned char callBefore[5];memcpy(callBefore,caller+4,5);
 const auto callFunction=reinterpret_cast<Function>(caller);void* callRelay=nullptr;
 stormslinger::HookTransaction calls;
 if(calls.begin_shared_with(good)!=MH_OK) return 36;
 if(callFunction()!=11 || calls.add_call(caller+4,detour,&callRelay)!=MH_OK || !callRelay || callFunction()!=11) return 36;
 stormslinger::HookTransaction pendingChild;
 if(pendingChild.begin_shared_with(calls)!=MH_ERROR_NOT_INITIALIZED) return 37;
 if(good.commit_pending_without_rollback()!=MH_OK || callFunction()!=11) return 38;
 if(calls.commit_pending_without_rollback()!=MH_OK || callFunction()!=73 || caller[4]!=0xE8 || function()!=18) return 38;
 if(MH_DisableHook(caller+4)!=MH_OK || callFunction()!=11 || memcmp(callBefore,caller+4,5)) return 39;
 if(MH_CreateCallHook(memory+800,detour,nullptr,nullptr)!=MH_ERROR_UNSUPPORTED_FUNCTION) return 40;
 // Freeze must NOT rewind a thread already at a CALL relay: its return
 // address is already pushed. Exercise the real suspended-thread machinery.
 detour[0]=0xEB;detour[1]=0xFE;memcpy(detour+2,before,32);detour[3]=73;
 FlushInstructionCache(GetCurrentProcess(),memory,4096);
 if(MH_EnableHook(caller+4)!=MH_OK) return 41;
 HANDLE worker=CreateThread(nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(caller),nullptr,0,nullptr);
 if(!worker) return 42;
 CONTEXT context{};context.ContextFlags=CONTEXT_CONTROL;bool atDetour=false;
 for(unsigned attempt=0;attempt<100;++attempt) {
  if(SuspendThread(worker)==DWORD(-1) || !GetThreadContext(worker,&context)) return 43;
  if(context.Rip==reinterpret_cast<DWORD64>(detour)) {atDetour=true;break;}
  ResumeThread(worker);Sleep(1);
 }
 if(!atDetour) return 44;
 const auto pushedStack=context.Rsp;
 context.Rip=reinterpret_cast<DWORD64>(callRelay);
 if(!SetThreadContext(worker,&context) || MH_DisableHook(caller+4)!=MH_OK) return 45;
 if(!GetThreadContext(worker,&context) || context.Rip!=reinterpret_cast<DWORD64>(callRelay) || context.Rsp!=pushedStack) return 46;
 context.Rip=reinterpret_cast<DWORD64>(detour+2);
 if(!SetThreadContext(worker,&context) || ResumeThread(worker)==DWORD(-1)) return 47;
 if(WaitForSingleObject(worker,3000)!=WAIT_OBJECT_0) return 48;
 DWORD exitCode=0;if(!GetExitCodeThread(worker,&exitCode) || exitCode!=73) return 49;
 CloseHandle(worker);
 if(memcmp(callBefore,caller+4,5) || callFunction()!=11) return 50;
 if(calls.stop()!=MH_OK || function()!=18) return 51;

 if(good.stop()!=MH_OK || function()!=11 || memcmp(before,memory,32)) return 7;
 stormslinger::HookTransaction failure;
 if(failure.begin()!=MH_OK) return 8;
 if(failure.add(memory,reinterpret_cast<void*>(&replacement),reinterpret_cast<void**>(&original))!=MH_OK) return 9;
 auto result=failure.add(memory,reinterpret_cast<void*>(&replacement),reinterpret_cast<void**>(&original));
 if(result!=MH_ERROR_ALREADY_CREATED || failure.last.rollback!=MH_OK || failure.last.owned || function()!=11 || memcmp(before,memory,32)) return 10;
 if(failure.stop()!=MH_OK) return 11;
 if(invalid_toolhelp && !invalid_snapshot_calls) return 53;
 VirtualFree(memory,0,MEM_RELEASE);
 puts("PASS: queued state, trampoline, activation, byte-exact restore, failed transaction rollback");
 return 0;
}
