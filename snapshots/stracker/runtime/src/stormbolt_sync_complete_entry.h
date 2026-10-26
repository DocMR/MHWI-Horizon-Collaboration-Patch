#pragma once
#include <cstdint>
extern "C" {
extern void (*stormbolt_sync_complete_dispatch_target)(void*,std::uintptr_t);
void stormbolt_sync_complete_entry();
}
