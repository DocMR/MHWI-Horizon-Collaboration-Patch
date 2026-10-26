#include "stormbolt_native_bindings.h"

// Explicit host entry, never called from DllMain. Status values are
// StormboltCodeStatus; mismatch_rva identifies the first failed native check.
// This checks loaded code/data only. Disk version authentication, native
// lifetime and resource readiness remain separate bootstrap requirements.
extern "C" __declspec(dllexport) int StormslingerCheckShootingBindings(
    const void* image,std::size_t image_bytes,std::uintptr_t* mismatch_rva) noexcept {
    if(!mismatch_rva) return static_cast<int>(stormslinger::StormboltCodeStatus::invalid_image);
    stormslinger::StormboltNativeBindings bindings{};
    const auto check=stormslinger::bind_stormbolt_native_code(
        reinterpret_cast<std::uintptr_t>(image),image_bytes,bindings);
    *mismatch_rva=check.rva;
    return static_cast<int>(check.status);
}
