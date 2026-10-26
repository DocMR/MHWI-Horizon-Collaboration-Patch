#include "horizon_native_clone.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <limits>
#include <initializer_list>

namespace horizon_other {
namespace {
#include "generated_native_clone_specs.inc"
bool displacement(const std::uint8_t* image, std::uintptr_t allocation,
                  const NativeRelocation& r, std::int32_t* out) noexcept {
    const auto delta = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(image) + r.target_rva)
                     - static_cast<std::int64_t>(allocation + r.next_offset);
    if (delta < (std::numeric_limits<std::int32_t>::min)() ||
        delta > (std::numeric_limits<std::int32_t>::max)()) return false;
    *out = static_cast<std::int32_t>(delta);
    return true;
}
bool accepted(TreeEditResult r) noexcept { return r == TreeEditResult::applied || r == TreeEditResult::unchanged; }
}
const NativeCloneSpec* native_clone_spec(std::uint32_t rva) noexcept {
    for (const auto& spec : kNativeClones) if (spec.rva == rva) return &spec;
    return nullptr;
}
bool PreparedNativeClone::prepare(const NativeCloneSpec& spec, const std::uint8_t* image,
                                 CloneComposer edits, CloneComposer shared) noexcept {
    if (allocation_ || !image || !edits || spec.payload_bytes > 4096 || spec.code_bytes > spec.payload_bytes)
        return false;
    std::uint8_t original[4096]{};
    SIZE_T read = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), image + spec.rva, original, spec.code_bytes, &read) ||
        read != spec.code_bytes || std::memcmp(original, spec.payload, spec.code_bytes)) return false;
    const auto source = reinterpret_cast<std::uintptr_t>(image) + spec.rva;
    const auto aligned = source & ~std::uintptr_t(0xffff);
    std::uint8_t* memory = nullptr;
    for (std::uintptr_t distance = 0; distance < 0x4000000 && !memory; distance += 0x10000) {
        for (int direction : {1, -1}) {
            if (direction < 0 && aligned < distance) continue;
            const auto candidate = direction > 0 ? aligned + distance : aligned - distance;
            bool fits = true;
            for (std::size_t i = 0; i < spec.relocation_count; ++i) {
                std::int32_t ignored = 0;
                if (!displacement(image, candidate, spec.relocations[i], &ignored)) { fits = false; break; }
            }
            if (fits) memory = static_cast<std::uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(candidate),
                4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            if (memory) break;
        }
    }
    if (!memory) return false;
    std::memcpy(memory, spec.payload, spec.payload_bytes);
    if (!accepted(edits(spec.rva, memory, spec.code_bytes)) ||
        (shared && !accepted(shared(spec.rva, memory, spec.code_bytes)))) {
        VirtualFree(memory, 0, MEM_RELEASE); return false;
    }
    for (std::size_t i = 0; i < spec.relocation_count; ++i) {
        const auto& r = spec.relocations[i];
        std::int32_t value = 0;
        if (!displacement(image, reinterpret_cast<std::uintptr_t>(memory), r, &value)) {
            VirtualFree(memory, 0, MEM_RELEASE); return false;
        }
        std::memcpy(memory + r.offset, &value, sizeof(value));
    }
    DWORD previous = 0;
    if (!VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &previous) ||
        !FlushInstructionCache(GetCurrentProcess(), memory, 4096) ||
        !RtlAddFunctionTable(reinterpret_cast<PRUNTIME_FUNCTION>(memory + spec.function_table_offset),
                             spec.function_count, reinterpret_cast<DWORD64>(memory))) {
        VirtualFree(memory, 0, MEM_RELEASE); return false;
    }
    allocation_ = memory;
    return true;
}
}
