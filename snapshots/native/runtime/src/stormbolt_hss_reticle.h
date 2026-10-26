#pragma once
#include <array>
#include <cstdint>
#include <cstring>
namespace stormslinger {
struct StormboltHssReticleNodes {std::array<void*,8> marks{};void* context{};};
struct StormboltHssReticleApi {
    std::uint32_t (*animation)(void*){};
    void (*play)(void*,std::uint32_t,void*){};
};
struct StormboltHssNodeBindings {
    // PC23537A0 and PC2340E20 respectively. Both borrow native GUI objects.
    void* (*find_root)(void*,const char*){};
    void* (*find_child)(void*,const char*){};
};
struct StormboltHssBoundNodes {
    StormboltHssReticleNodes stages{};
    void* reload_out_of_range{};
};
// Source BDBF70 searches a_hss_sight, ammo01..08 and reload_outOfRenge.
// PC roots expose their child collection at +1C0 (PS uses +1B0).
// Publish only a complete binding; the GUI owner controls its lifetime.
inline bool bind_stormbolt_hss_nodes(void* gui,const StormboltHssNodeBindings& api,
    StormboltHssBoundNodes& output) {
    if(!gui || !api.find_root || !api.find_child) return false;
    StormboltHssBoundNodes bound{};
    bound.stages.context=api.find_root(gui,"a_hss_sight");
    if(!bound.stages.context) return false;
    void* collection{};
    std::memcpy(&collection,static_cast<const unsigned char*>(bound.stages.context)+0x1C0,
        sizeof(collection));
    if(!collection) return false;
    constexpr const char* names[]={"ammo01","ammo02","ammo03","ammo04",
        "ammo05","ammo06","ammo07","ammo08"};
    for(unsigned i=0;i<8;++i) {
        bound.stages.marks[i]=api.find_child(collection,names[i]);
        if(!bound.stages.marks[i]) return false;
    }
    bound.reload_out_of_range=api.find_child(collection,"reload_outOfRenge");
    if(!bound.reload_out_of_range) return false;
    output=bound;
    return true;
}
// PS BDD5A0 / BDD650 completion callbacks use the node-only play operation
// (4B7A310), distinct from BDCC80's context-aware stage-animation operation.
inline void stormbolt_hss_animation_event(void* node,std::uint32_t event,
    bool stage_mark,void (*play_terminal)(void*,std::uint32_t)) {
    if(event==0x2711u) play_terminal(node,1u);
    else if(stage_mark && event==0x2712u) play_terminal(node,0x2713u);
}
// PS BDCC80: eight mark animation choices. Native UI owner supplies live nodes;
// this layer neither creates a GUI nor stores fields in PC native allocations.
inline bool refresh_stormbolt_hss_reticle(const StormboltHssReticleNodes& nodes,
    std::uint32_t stage,bool immediate,const StormboltHssReticleApi& api) {
    if(!api.animation || !api.play) return false;
    for(auto* mark:nodes.marks) if(!mark) return false;
    for(std::uint32_t index=0;index<8;++index) {
        const bool filled=stage>index;
        const auto terminal=filled?1u:0x2713u;
        if(api.animation(nodes.marks[index])==terminal) continue;
        api.play(nodes.marks[index],immediate?terminal:(filled?0x2711u:0x2712u),nodes.context);
    }
    return true;
}
} // namespace stormslinger
