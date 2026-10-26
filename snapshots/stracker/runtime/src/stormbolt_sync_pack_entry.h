#pragma once
#include <cstdint>
extern "C" {
extern std::uint32_t (*stormbolt_sync_pack_dispatch_target)(void*,std::uintptr_t,std::uint32_t);
void stormbolt_sync_pack_entry();
}
