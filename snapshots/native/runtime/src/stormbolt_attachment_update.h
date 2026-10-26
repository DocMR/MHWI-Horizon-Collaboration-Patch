#pragma once
#include "stormbolt_attachment_blink.h"
#include "stormbolt_attachment_gate.h"
#include "stormbolt_attachment_color.h"
#include "stormbolt_material_color.h"
#include "stormbolt_instance_store.h"

namespace stormslinger {
// Selected PS70BE continuation after category/model/material prerequisites.
// Caller retains the first material pointer captured before updating blink state.
// Colors are supplied explicitly so verified defaults do not erase runtime overrides.
template<std::size_t Capacity>
bool stormbolt_update_attachment_color(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,void* material,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa,const StormboltMaterialColorApi& api) {
    if (!material || !api.write_vector) return false;
    std::array<float,4> color{};
    if (!store.transact(token,[&](auto& state) noexcept {
        float step{};
        std::memcpy(&step,reinterpret_cast<const std::byte*>(token.address)+0x68,4);
        const float multiplier=advance_stormbolt_attachment_blink(state,step);
        color=stormbolt_attachment_rgba(state.counter<6 ? low_hlsa : high_hlsa,multiplier);
        return true;
    })) return false;
    // Commit before native dispatch; do not run native calls under the store lock.
    if (!stormbolt_write_material_color(material,color,api)) return false;
    StormboltStageState after{};
    return store.snapshot(token,after); // Detect retirement without replaying writes.
}
// Common attachment tail only; caller must reach PS70B0-equivalent continuation.
// A valid sidecar token is required even for an inactive material selection.
template<std::size_t Capacity>
bool stormbolt_run_attachment_tail(StormboltInstanceStore<Capacity>& store,
    StormboltInstanceToken token,const std::array<float,4>& low_hlsa,
    const std::array<float,4>& high_hlsa,const StormboltMaterialColorApi& api) {
    StormboltStageState current{};
    if (!store.snapshot(token,current)) return false;
    void* material{};
    const auto gate=stormbolt_select_attachment_material(reinterpret_cast<void*>(token.address),material);
    if (gate==StormboltAttachmentGate::unavailable) return false;
    if (gate==StormboltAttachmentGate::inactive) return true;
    return stormbolt_update_attachment_color(store,token,material,low_hlsa,high_hlsa,api);
}
} // namespace stormslinger
