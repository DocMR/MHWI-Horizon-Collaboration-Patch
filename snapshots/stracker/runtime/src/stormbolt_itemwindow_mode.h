#pragma once
#include "stormbolt_owned_call_site.h"
extern "C" void stormbolt_itemwindow_mode_entry();
namespace stormslinger {
class StormboltItemWindowMode {
    StormboltOwnedReplayCallSite<8> site_;
public:
    explicit StormboltItemWindowMode(HookTransaction* shared):site_(shared){}
    bool prepare(void* site) {
        return site_.prepare(site,reinterpret_cast<std::uintptr_t>(&stormbolt_itemwindow_mode_entry),
            std::array<unsigned char,8>{0x41,0x83,0xB8,0xF8,0x09,0,0,0x0C});
    }
    bool activate(){return site_.activate();}
    bool restore(){return site_.restore();}
    bool release_after_shared_stop(){return site_.release_after_quiescence();}
};
}
