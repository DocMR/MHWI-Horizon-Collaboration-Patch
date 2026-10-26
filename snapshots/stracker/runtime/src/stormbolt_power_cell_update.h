#pragma once
#include "stormbolt_power_cell_loop_adapter.h"
#include "stormbolt_power_cell_visual_color.h"

namespace stormslinger {
// Native caller supplies the selected material and component570 disable flag.
// APIs run outside the sidecar lock, with synchronous borrowed lifetimes.
template<std::size_t Capacity>
bool update_stormbolt_power_cell_from_inputs(
    StormboltPowerCellLoopStore<Capacity>& store,StormboltPowerCellLoopToken token,
    const StormboltLoopNativeContext& context,const StormboltPowerCellLoopUpdateInputs& input,
    void* material,bool materialDisabled,StormboltVisualConvert convert,
    StormboltMaterialScalar scalar,const StormboltMaterialColorApi& materialApi,
    StormboltNamedEventRequest sound) {
    if (!input.owner_flag || !input.resource_index_present || input.resource_count<=0)
        return process_stormbolt_power_cell_loop_audio(store,token,context,input,sound);
    if (material && !materialDisabled && (!convert || !scalar || !materialApi.write_vector)) return false;
    const bool equal=input.count==input.threshold;
    // Native unsigned32 SUB/CMP; subtraction deliberately wraps.
    const auto band=equal?StormboltPowerCellVisualBand::threshold:
        (input.count<std::uint32_t(input.threshold-input.related_count)
            ?StormboltPowerCellVisualBand::lower:StormboltPowerCellVisualBand::upper);
    if (!equal && !process_stormbolt_power_cell_loop_audio(store,token,context,input,sound)) return false;
    if (!store.transact(token,[&](auto& state) noexcept {
        if (equal) reset_stormbolt_power_cell_threshold_visual(state);
        else update_stormbolt_power_cell_non_threshold_visual(state,context.step);
        return true;
    })) return false;
    StormboltPowerCellLoopState snapshot{};
    if (!store.snapshot(token,snapshot)) return false;
    if (material && !materialDisabled) {
        if (equal) stormbolt_write_power_cell_threshold_material(material,snapshot,convert,scalar,materialApi);
        else stormbolt_write_power_cell_non_threshold_material(material,band,snapshot,convert,scalar,materialApi);
    }
    if (equal && !process_stormbolt_power_cell_loop_audio(store,token,context,input,sound)) return false;
    // PS350C6: resource-failure branch above bypasses this normal exit.
    return store.transact(token,[](auto& state) noexcept { state.growth=false;return true; });
}
} // namespace stormslinger
