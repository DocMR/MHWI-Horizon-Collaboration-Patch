#pragma once
#include <windows.h>
#include <vector>
#include <new>
#include "MinHook.h"
namespace stormslinger {
struct HookResult { MH_STATUS operation=MH_OK; MH_STATUS rollback=MH_OK; size_t owned=0; bool active=false; };
// The library is statically linked into our DLL, with its own private state.
// Never target another module's hooks or assume queued activation succeeded.
class HookTransaction {
 std::vector<void*> targets;
 bool initialized=false;
 bool active=false;
 HookTransaction* library_owner=nullptr;
 size_t borrowers=0;
 bool coordinated_rollback=false;
 void rollback_add_failure() {if(!coordinated_rollback) stop();}
public:
 // Coordinator must serialize begin/commit/stop and keep the owner alive.
 // Borrow only a committed parent, so its enable queue is already drained.
 // One child per parent permits a serial startup -> quest -> shooting chain;
 // each child retains its direct parent until it has stopped.
 // Mixed virtual-slot/instruction sessions opt in to retaining failed add()
 // state until their coordinator restores those publications and calls stop().
 MH_STATUS begin_shared_with(HookTransaction& owner,bool coordinator_owns_rollback=false) {
  if(initialized || &owner==this || owner.borrowers)
   return last.operation=MH_ERROR_ALREADY_INITIALIZED;
  if(!owner.initialized || !owner.active) return last.operation=MH_ERROR_NOT_INITIALIZED;
  library_owner=&owner;++owner.borrowers;initialized=true;
  coordinated_rollback=coordinator_owns_rollback;
  return last.operation=MH_OK;
 }
 HookResult last;
 bool can_add_hooks() const noexcept {return initialized && !active;}
 bool has_library_state() const noexcept {return initialized;}
 // Root and borrowed coordinators must opt in BEFORE queuing mixed-module
 // hooks. Failed add retains trampolines until their direct patches are restored.
 bool retain_failed_adds_for_coordinator() noexcept {
  if(!can_add_hooks()) return false;
  coordinated_rollback=true;return true;
 }
 MH_STATUS begin() {
  if(initialized) return last.operation=MH_ERROR_ALREADY_INITIALIZED;
  auto code=MH_Initialize();
  initialized=(code==MH_OK);return last.operation=code;
 }
 MH_STATUS add(void* target,void* replacement,void** original) {
  return add_impl<false>(target,replacement,original,nullptr);
 }
 // Native mapping references join the same queue as detours. The caller
 // authenticates the containing instruction and prepares the rel32 target.
 MH_STATUS add_rip_patch(void* target,const void* expected,UINT size,UINT offset,INT32 displacement) {
  if(!initialized || borrowers || active) return last.operation=MH_ERROR_NOT_INITIALIZED;
  try {targets.reserve(targets.size()+1);}
  catch(const std::bad_alloc&) {last.operation=MH_ERROR_MEMORY_ALLOC;rollback_add_failure();return last.operation;}
  auto code=MH_CreateRipRelativePatch(target,expected,size,offset,displacement);
  if(code!=MH_OK) {last.operation=code;rollback_add_failure();return code;}
  targets.push_back(target);last.owned=targets.size();
  code=MH_QueueEnableHook(target);
  if(code!=MH_OK) {last.operation=code;rollback_add_failure();return code;}
  return last.operation=MH_OK;
 }
 // A committed parent cannot publish while its child owns the global queue.
 // CALL sites join the same prepare/commit batch as the owner's other hooks.
 MH_STATUS add_call(void* target,void* replacement,void** relay) {
  return add_impl<true>(target,replacement,nullptr,relay);
 }
 MH_STATUS add_replay_call(void* target,void* replacement,void** relay,UINT span) {
  return add_impl<true>(target,replacement,nullptr,relay,span);
 }
private:
 template<bool Call> MH_STATUS add_impl(void* target,void* replacement,void** original,void** relay,UINT replay_span=5) {
  if(!initialized || borrowers || active) return last.operation=MH_ERROR_NOT_INITIALIZED;
  // Allocate ownership bookkeeping before creating a hook. An allocation
  // failure after MH_CreateHook would otherwise leave an untracked trampoline.
  try { targets.reserve(targets.size()+1); }
  catch(const std::bad_alloc&) {
   last.operation=MH_ERROR_MEMORY_ALLOC;rollback_add_failure();return MH_ERROR_MEMORY_ALLOC;
  }
  MH_STATUS code;
  if constexpr(Call) {
   if(replay_span>5) code=MH_CreateReplayCallHook(target,replacement,relay,replay_span);
   else code=MH_CreateCallHook(target,replacement,original,relay);
  }
  else code=MH_CreateHook(target,replacement,original);
  if(code!=MH_OK) {last.operation=code;rollback_add_failure();return code;}
  targets.push_back(target);last.owned=targets.size();
  code=MH_QueueEnableHook(target);
  if(code!=MH_OK) {last.operation=code;rollback_add_failure();return code;}
  return last.operation=MH_OK;
 }
public:
 // A coordinator with non-MinHook state must choose the rollback order itself.
 // On failure some queued hooks may already be enabled; retain all ownership.
 MH_STATUS commit_pending_without_rollback() {
  if(!initialized || targets.empty()) return last.operation=MH_ERROR_NOT_INITIALIZED;
  // A repeated commit belongs to this transaction, not the library-wide queue.
  // A borrower may now have pending hooks that only its coordinator can enable.
  if(active) return last.operation=MH_OK;
  auto code=MH_ApplyQueued();last.operation=code;
  if(code!=MH_OK) return code;
  active=true;last.active=true;return MH_OK;
 }
 MH_STATUS commit() {
  if(!initialized || targets.empty()) return last.operation=MH_ERROR_NOT_INITIALIZED;
  auto code=commit_pending_without_rollback();
  if(code!=MH_OK) stop();
  return code;
 }
 MH_STATUS stop() {
  if(!initialized) return MH_OK;
  if(borrowers) return last.rollback=MH_ERROR_ENABLED;
  MH_STATUS failure=MH_OK;
  for(void* target:targets) {
   auto code=MH_DisableHook(target);
   if(code!=MH_OK && code!=MH_ERROR_DISABLED && failure==MH_OK) failure=code;
  }
  // A failed disable must retain trampolines and DLL lifetime. Never remove an
  // enabled hook and pretend rollback succeeded.
  if(failure!=MH_OK) {last.rollback=failure;return failure;}
  for(void* target:targets) {
   auto code=MH_RemoveHook(target);
   if(code!=MH_OK && failure==MH_OK) failure=code;
  }
  if(failure!=MH_OK) {last.rollback=failure;return failure;}
  if(library_owner) {
   --library_owner->borrowers;library_owner=nullptr;
  } else {
   auto code=MH_Uninitialize();
   if(code!=MH_OK) {last.rollback=code;return code;}
  }
  targets.clear();initialized=false;active=false;coordinated_rollback=false;
  last.owned=0;last.active=false;last.rollback=MH_OK;
  return MH_OK;
 }
};
}

