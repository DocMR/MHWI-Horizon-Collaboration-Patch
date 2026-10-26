#pragma once
#include <cstdint>
extern "C" {
extern void (*stormbolt_status_update_dispatch_target)(void*);
extern std::uintptr_t stormbolt_status_update_manager_slot;
void stormbolt_status_update_entry();
}
