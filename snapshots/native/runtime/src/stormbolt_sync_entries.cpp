#include "stormbolt_sync_pack_entry.h"
#include "stormbolt_sync_complete_entry.h"
extern "C" {
std::uint32_t (*stormbolt_sync_pack_dispatch_target)(void*,std::uintptr_t,std::uint32_t)=nullptr;
void (*stormbolt_sync_complete_dispatch_target)(void*,std::uintptr_t)=nullptr;
}
