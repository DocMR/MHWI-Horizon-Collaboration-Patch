#pragma once
#include "owned_vtable_pointer.h"
#include "stormbolt_coefficient_adapter.h"
#include "stormbolt_shell_hook.h"
#include <intrin.h>

namespace stormslinger {
using StormboltCoefficientCallback = void (*)(void* weapon, std::uint32_t ammo, void* shell_info);
struct StormboltCoefficientBindings {
    void* slot = nullptr; // image+34D6C88+3D8, not shared stub23D430.
    StormboltCoefficientCallback original = nullptr;
    std::uintptr_t caller_return = 0; // image+214CAC4
    std::uintptr_t weapon_vtable = 0, info_vtable = 0, parameter_vtable = 0;
    bool (*dispatch_ready)() = nullptr; // Optional coordinator publication control.
};

// Reads the same live cWp13Param chain as PC LBG recharge; no frozen PS table
// replaces runtime parameters. Bootstrap validates these native type bindings.
inline bool read_stormbolt_native_rows(std::uintptr_t weapon, std::uintptr_t expected_vtable,
                                      std::array<StormboltRawRow, 9>& rows) noexcept {
    std::uintptr_t actor = 0, parameter = 0, vtable = 0;
    std::uint8_t flags = 0;
    if (!read_stormbolt_field(weapon, 0x9B0, actor) || !read_stormbolt_field(actor, 0xC, flags) ||
        !(flags & 0xE) || !read_stormbolt_field(actor, 0x7810, parameter) ||
        !read_stormbolt_field(parameter, 0, vtable) || vtable != expected_vtable) return false;
    std::array<std::array<unsigned char, 40>, 9> native{};
    if (!read_stormbolt_field(parameter, 0x1600, native)) return false;
    for (std::size_t row = 0; row < rows.size(); ++row) {
        std::memcpy(rows[row].fields.data(), native[row].data() + 8, 7 * sizeof(float));
        rows[row].flag = native[row][36];
    }
    return true;
}

inline bool stormbolt_coefficient_fields_writable(std::uintptr_t info) noexcept {
    if (!info || info > (std::numeric_limits<std::uintptr_t>::max)() - 0x70) return false;
    auto current = info + 0x60;
    const auto end = info + 0x70;
    while (current < end) {
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(reinterpret_cast<void*>(current), &memory, sizeof(memory)) ||
            memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD)) return false;
        const auto access = memory.Protect & 0xFF;
        if (access != PAGE_READWRITE && access != PAGE_WRITECOPY &&
            access != PAGE_EXECUTE_READWRITE && access != PAGE_EXECUTE_WRITECOPY) return false;
        const auto region_base = reinterpret_cast<std::uintptr_t>(memory.BaseAddress);
        if (memory.RegionSize > (std::numeric_limits<std::uintptr_t>::max)() - region_base) return false;
        const auto next = region_base + memory.RegionSize;
        if (next <= current) return false;
        current = next;
    }
    return true;
}

// Process-lifetime hook. Native callback borrows a live writable cpPlShellInfo;
// readability/protection checks alone do not establish lifetime or exclusion.
// Install/restore only while native execution and other patch writers quiesce.
template<std::size_t Capacity>
class StormboltCoefficientHook {
    StormboltInstanceStore<Capacity>& store_;
    StormboltCoefficientBindings bindings_{};
    OwnedVtablePointer<> pointer_;
    inline static StormboltCoefficientHook* active_ = nullptr;
public:
    struct Diagnostics {
        std::atomic<std::uint64_t> applied{0}, rejected{0}, unrelated{0}, exceptions{0};
    } diagnostics;
private:
    void apply(void* native_weapon, std::uint32_t ammo, void* info) {
        if (bindings_.dispatch_ready && !bindings_.dispatch_ready()) return;
        if (ammo != 40) { ++diagnostics.unrelated; return; }
        const auto address = reinterpret_cast<std::uintptr_t>(native_weapon);
        const auto parameter_object = reinterpret_cast<std::uintptr_t>(info);
        struct Identity { std::uint32_t category, id; } identity{};
        std::uintptr_t vtable = 0, bound_weapon = 0;
        std::uint8_t alive = 0;
        if (!read_stormbolt_field(address, 0, vtable) || vtable != bindings_.weapon_vtable ||
            !read_stormbolt_field(address, 0xC, alive) || !(alive & 0xE) ||
            !read_stormbolt_field(address, 0x9F8, identity)) { ++diagnostics.rejected; return; }
        if (identity.category != 13 || (identity.id != 238 && identity.id != 239)) {
            ++diagnostics.unrelated; return;
        }
        std::uint32_t source_count=0;
        if (!read_stormbolt_field(address,0x2388,source_count) || !source_count) {
            ++diagnostics.unrelated; return;
        }
        if (!read_stormbolt_field(parameter_object, 0, vtable) || vtable != bindings_.info_vtable ||
            !read_stormbolt_field(parameter_object, 0x80, bound_weapon) || bound_weapon != address ||
            !stormbolt_coefficient_fields_writable(parameter_object)) { ++diagnostics.rejected; return; }
        std::array<StormboltRawRow, 9> rows{};
        StormboltInstanceToken token{};
        if (!read_stormbolt_native_rows(address, bindings_.parameter_vtable, rows) ||
            !store_.token_for_live_address(address, token)) { ++diagnostics.rejected; return; }
        const bool applied = store_.transact(token, [&](auto& state) noexcept {
            return apply_stormbolt_coefficients(identity.category, identity.id, ammo, true,
                static_cast<unsigned char*>(info), 0x6B0, state, rows) == CoefficientApplyResult::applied;
        });
        if (applied) ++diagnostics.applied; else ++diagnostics.rejected;
    }
    __declspec(noinline) static void callback(void* native_weapon, std::uint32_t ammo, void* info) {
        auto& hook_state = *active_;
        if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) == hook_state.bindings_.caller_return) {
            try { hook_state.apply(native_weapon, ammo, info); }
            catch (...) { ++hook_state.diagnostics.exceptions; }
        } else { ++hook_state.diagnostics.unrelated; }
        // Apply our base content before any later callback. Original native
        // slot is a ret stub; preserve its call for all target/unrelated paths.
        hook_state.bindings_.original(native_weapon, ammo, info);
    }
public:
    explicit StormboltCoefficientHook(StormboltInstanceStore<Capacity>& instance_store) : store_(instance_store) { }
    StormboltCoefficientHook(const StormboltCoefficientHook&) = delete;
    StormboltCoefficientHook& operator=(const StormboltCoefficientHook&) = delete;
    bool active() const noexcept { return pointer_.active(); }
    bool install(const StormboltCoefficientBindings& bindings) {
        if (active_ || !bindings.slot || !bindings.original || !bindings.caller_return ||
            !bindings.weapon_vtable || !bindings.info_vtable || !bindings.parameter_vtable) return false;
        bindings_ = bindings;
        active_ = this;
        return pointer_.install(bindings.slot, reinterpret_cast<void*>(bindings.original),
                                reinterpret_cast<void*>(&callback));
    }
    bool restore() { return pointer_.restore(); }
};
} // namespace stormslinger
