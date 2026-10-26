#pragma once
#include "stormbolt_hss_reticle.h"
#include <functional>

namespace stormslinger {
// PC614C2D/614C41: child-node callback at +158, installed by 24B980.
// Its six virtual operations and +38 active pointer match the MSVC function
// ABI used by this runtime. The native swap also disposes the replaced callback.
using StormboltHssNativeFunction=std::function<void(void* const&,const std::uint32_t&)>;
static_assert(sizeof(StormboltHssNativeFunction)==0x40);
struct StormboltHssCallbackBindings {
    void (*swap_function)(void*,void*){}; // PC24B980
    void (*play_terminal)(void*,std::uint32_t){}; // PC2342AF0, PS4B7A310 node operation
};
inline bool install_stormbolt_hss_callbacks(const StormboltHssBoundNodes& nodes,
    const StormboltHssCallbackBindings& api) {
    if(!api.swap_function || !api.play_terminal || !nodes.reload_out_of_range) return false;
    for(auto* mark:nodes.stages.marks) if(!mark) return false;
    for(unsigned index=0;index<9;++index) {
        const bool stage_mark=index<8;
        auto* node=stage_mark?nodes.stages.marks[index]:nodes.reload_out_of_range;
        // Capture only immutable values. The native tree can destroy this
        // function without consulting a GUI adapter that has already retired.
        StormboltHssNativeFunction callback=[play=api.play_terminal,stage_mark](
            void* const& event_node,const std::uint32_t& event) {
            stormbolt_hss_animation_event(event_node,event,stage_mark,play);
        };
        api.swap_function(&callback,static_cast<unsigned char*>(node)+0x158);
        // callback now owns the previous native callable and releases it using
        // its own original virtual destructor, including its native allocator.
    }
    return true;
}
} // namespace stormslinger
