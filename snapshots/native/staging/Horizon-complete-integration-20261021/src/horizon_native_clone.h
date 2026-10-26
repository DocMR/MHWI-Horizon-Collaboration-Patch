#pragma once
#include "horizon_native_integration.h"

namespace horizon_other {
struct NativeRelocation { std::uint32_t offset, next_offset, target_rva; };
struct NativeCloneSpec {
    std::uint32_t rva;
    std::size_t code_bytes;
    const std::uint8_t* payload;
    std::size_t payload_bytes;
    const NativeRelocation* relocations;
    std::size_t relocation_count;
    std::uint32_t function_table_offset, function_count;
};
const NativeCloneSpec* native_clone_spec(std::uint32_t owner_rva) noexcept;
// A successfully prepared allocation and registered unwind table are retained
// for process lifetime. The containing DLL must not be unloaded after use.
class PreparedNativeClone {
    void* allocation_ = nullptr;
public:
    PreparedNativeClone() = default;
    PreparedNativeClone(const PreparedNativeClone&) = delete;
    PreparedNativeClone& operator=(const PreparedNativeClone&) = delete;
    void* entry() const noexcept { return allocation_; }
    bool prepare(const NativeCloneSpec& spec, const std::uint8_t* certified_image,
                 CloneComposer module_edits, CloneComposer shared_edits = nullptr) noexcept;
};
}
