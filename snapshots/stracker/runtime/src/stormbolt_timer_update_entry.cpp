#include "stormbolt_timer_update_entry.h"
extern "C" {
void (*stormbolt_timer_update_dispatch_target)(void*,float)=nullptr;
bool (*stormbolt_timer_update_native_query)(void*)=nullptr;
}
