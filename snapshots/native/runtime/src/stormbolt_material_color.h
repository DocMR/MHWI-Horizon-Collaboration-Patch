#pragma once
#include <array>
#include <cstdint>

namespace stormslinger {
inline constexpr std::uint32_t stormbolt_emissive_parameter_hash=0x361F6075u;
struct StormboltMaterialColorApi {
    // PC23D82A0; native material-specific query/absent-parameter behavior retained.
    void (*write_vector)(void*,std::uint32_t,const float*)=nullptr;
};
// Caller supplies the selected material and converted/scaled RGBA. No field
// offset guesses, fixed constant-buffer selection, or success claim for drawing.
inline bool stormbolt_write_material_color(void* material,
    const std::array<float,4>& color,const StormboltMaterialColorApi& api) {
    if (!material || !api.write_vector) return false;
    api.write_vector(material,stormbolt_emissive_parameter_hash,color.data());
    return true; // Dispatch only: native absent parameter is a valid no-op.
}
} // namespace stormslinger
