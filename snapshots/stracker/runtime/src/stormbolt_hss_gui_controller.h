#pragma once
#include "stormbolt_hss_native_callbacks.h"
#include "stormbolt_hss_native_input.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
struct StormboltHssGuiBindings {
    StormboltHssNodeBindings nodes{};
    StormboltHssCallbackBindings callbacks{};
    StormboltHssIdentity identity{};
    StormboltHssInputBindings input{};
    StormboltHssReticleApi animation{};
    void (*hide_gui)(void*,bool){}; // PC1FB7CC0
    void (*hide_node)(void*,bool){}; // PC2343310
    void** text_manager_slot{}; // PC5012DF0
    const char* (*text_at)(void*,std::uint32_t){}; // PC2244930
    void (*fit_warning_text)(void*,void*,const char*){}; // PC18C1C50
};
// One controller belongs to one native HSS GUI, not to the ordinary bowgun GUI.
// Native virtual callbacks must own its creation and retire it before base GUI
// destruction. Nodes never outlive that GUI and callback captures do not use us.
class StormboltHssGuiController {
    StormboltHssBoundNodes nodes_{};
    StormboltHssGuiBindings api_{};
    bool initialized_{};
    std::uint8_t reload_{};
    bool warning_text_ready_{};
    bool prepare_warning_text() {
        if(warning_text_ready_) return true;
        void* children{};
        std::memcpy(&children,static_cast<const unsigned char*>(nodes_.reload_out_of_range)+0xF0,sizeof(children));
        if(!children || !api_.text_manager_slot || !*api_.text_manager_slot) return false;
        auto* message=api_.nodes.find_child(children,"msg_sign");
        auto* base=api_.nodes.find_child(children,"null_bace");
        if(!message || !base) return false;
        const char* text=api_.text_at(*api_.text_manager_slot,0x1A);
        if(!text || !*text) return false;
        api_.fit_warning_text(message,base,text);
        warning_text_ready_=true;
        return true;
    }
    template<std::size_t Capacity>
    static std::uint32_t stage(StormboltInstanceStore<Capacity>& store,std::uintptr_t weapon) {
        StormboltInstanceToken token{};StormboltStageState state{};
        if(!store.token_for_live_address(weapon,token) || !store.snapshot(token,state))
            return 0xFFFFFFFFu;
        return state.counter;
    }
public:
    bool initialize(void* gui,const StormboltHssGuiBindings& api) {
        if(initialized_ || !api.hide_gui || !api.hide_node ||
            !api.input.action_filter || !api.animation.animation || !api.animation.play ||
            !api.text_manager_slot || !*api.text_manager_slot || !api.text_at || !api.fit_warning_text)
            return false;
        StormboltHssBoundNodes bound{};
        if(!bind_stormbolt_hss_nodes(gui,api.nodes,bound) ||
            !install_stormbolt_hss_callbacks(bound,api.callbacks)) return false;
        nodes_=bound;api_=api;initialized_=true;reload_=0;warning_text_ready_=false;
        // External children/localized entries may become available after the
        // native tree's first initialization. Do not publish an empty fit as done.
        (void)prepare_warning_text();
        api_.hide_node(nodes_.reload_out_of_range,true);
        return true;
    }
    template<std::size_t Capacity>
    bool show(std::uintptr_t actor,StormboltInstanceStore<Capacity>& store) {
        if(!initialized_) return false;
        StormboltHssNativeInput input{};
        if(!stormbolt_hss_native_input(actor,api_.identity,api_.input,input)) return false;
        (void)prepare_warning_text();
        reload_=0;api_.hide_node(nodes_.reload_out_of_range,true);
        return refresh_stormbolt_hss_reticle(nodes_.stages,stage(store,input.weapon),true,
            api_.animation);
    }
    template<std::size_t Capacity>
    bool refresh_stage(void* gui,std::uintptr_t actor,StormboltInstanceStore<Capacity>& store) {
        if(!initialized_ || !gui) return false;
        StormboltHssNativeInput input{};
        if(!stormbolt_hss_native_input(actor,api_.identity,api_.input,input)) {
            api_.hide_gui(gui,true);
            return false;
        }
        if(!input.visible) api_.hide_gui(gui,true);
        // PS BDD460 (288/290/2A8): refresh marks only. The reload byte is
        // sampled by BDC8A0 at its own update phase, not by these callbacks.
        return refresh_stormbolt_hss_reticle(nodes_.stages,stage(store,input.weapon),false,
            api_.animation);
    }
    template<std::size_t Capacity>
    bool update(void* gui,std::uintptr_t actor,StormboltInstanceStore<Capacity>&) {
        if(!initialized_ || !gui) return false;
        std::uint8_t context{};
        if(!stormbolt_hss_warning_context(actor,api_.identity,context)) return false;
        const bool warning_ready=prepare_warning_text();
        // BDC8A0: transition only when reload/out-of-range context changes.
        if(warning_ready && context!=reload_) {
            if(context) api_.hide_node(nodes_.reload_out_of_range,false);
            api_.animation.play(nodes_.reload_out_of_range,
                context?0x2711u:0x2712u,nodes_.stages.context);
            reload_=context;
        }
        return true;
    }
    void retire() noexcept {nodes_={};api_={};initialized_=false;reload_=0;warning_text_ready_=false;}
};
} // namespace stormslinger
