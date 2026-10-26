#pragma once
extern "C" {
extern void (*stormbolt_timer_update_dispatch_target)(void*,float);
extern bool (*stormbolt_timer_update_native_query)(void*);
void stormbolt_timer_update_entry();
}
