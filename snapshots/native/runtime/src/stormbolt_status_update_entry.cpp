#include "stormbolt_status_update_entry.h"
extern "C" {
void (*stormbolt_status_update_dispatch_target)(void*)=nullptr;
std::uintptr_t stormbolt_status_update_manager_slot=0;
}
