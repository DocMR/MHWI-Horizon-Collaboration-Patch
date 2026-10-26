#pragma once
#include "hooks.h"
#include "stormbolt_instance_store.h"
#include "stormbolt_shot_core.h"
#include "stormbolt_native_memory.h"
#include <atomic>
#include <limits>
#include <type_traits>

namespace stormslinger {
using StormboltShellCreate = void* (*)(void* descriptor, void* weapon, void* owner,
                                      const void* context);
using StormboltShellRecordAt = const void* (*)(void* list, std::uint32_t index);
using StormboltShellDispatch = void* (*)(void* descriptor, void* weapon, void* owner,
    const void* context, std::uintptr_t caller_return, void* captured_list,
    std::uint32_t captured_ammo);
}
extern "C" {
extern stormslinger::StormboltShellDispatch stormbolt_shell_dispatch_target;
void stormbolt_shell_hook_entry();
}

namespace stormslinger {
struct StormboltShellHookBindings {
    void* create_entry = nullptr;          // image+1AA74A0
    std::uintptr_t caller_return = 0;      // image+214C5BE
    std::uintptr_t weapon_vtable = 0;      // image+34D6C88
    std::uintptr_t list_vtable = 0;        // image+33EBDF0
    StormboltShellRecordAt record_at = nullptr; // image+F7DC00
    bool (*dispatch_ready)() = nullptr; // Optional coordinator publication control.
    std::uintptr_t parameter_vtable = 0; // Live cWp13Param; production energy cost.
    bool (*before_energy)(void*,std::uint32_t) = nullptr;
    bool (*after_energy)(void*,std::uint32_t) = nullptr;
};

// Small read-only copies. Readability is not proof of native object lifetime;
// that must come from the verified synchronous native shooting boundary.
// Process-lifetime, bootstrap-only hook registration. Caller must verify live
// code, resource/EFX readiness, and native lifetime/thread exclusion before use.
// Activation belongs to the same transaction as instance lifecycle tracking.
// Source factory clamps its shot snapshot and consumes energy before creation;
// action advancement and other effects remain their respective callers' work.
template<std::size_t Capacity>
class StormboltShellHook {
public:
    struct Diagnostics {
        std::atomic<std::uint64_t> selected{0}, rejected{0}, unrelated{0},
            untracked{0}, native_null{0}, selection_exceptions{0},
            energy_applied{0}, energy_rejected{0}, self_damage_applied{0},self_damage_rejected{0};
    } diagnostics;
private:
    bool consume_energy(void* weapon) {
        const auto address=reinterpret_cast<std::uintptr_t>(weapon);
        std::uintptr_t actor{},parameter{},table{};
        std::uint8_t flags{};
        float gauge{};
        StormboltInstanceToken token{};StormboltStageState before{};
        if(!read_stormbolt_field(address,0x9B0,actor) ||
            !read_stormbolt_field(actor,0xC,flags) || !(flags&0xE) ||
            !read_stormbolt_field(actor,0x7810,parameter) ||
            !read_stormbolt_field(parameter,0,table) || table!=bindings_.parameter_vtable ||
            !read_stormbolt_field(address,0x2358,gauge) ||
            !stormbolt_native_span(address,0x2358,sizeof(float),true) ||
            !store_.token_for_live_address(address,token) || !store_.snapshot(token,before))return false;
        const auto row=(std::min)(before.shot_snapshot,std::uint32_t{8});
        float cost{};
        if(!read_stormbolt_field(parameter,0x1610+40*row,cost))return false;
        std::array<StormboltRawRow,9> rows{};rows[row].fields[2]=cost;
        auto selected=before;selected.shot_snapshot=row;
        if(!subtract_stormbolt_energy(gauge,selected,rows))return false;
        if(!store_.transact(token,[&](auto& state) noexcept {
            if(state.shot_snapshot!=before.shot_snapshot)return false;
            state.shot_snapshot=row;return true;
        }))return false;
        std::memcpy(static_cast<unsigned char*>(weapon)+0x2358,&gauge,sizeof(gauge));
        return true;
    }
    StormboltInstanceStore<Capacity>& store_;
    StormboltShellHookBindings bindings_{};
    StormboltShellCreate original_ = nullptr;
    inline static StormboltShellHook* active_ = nullptr;

    bool select(void* original_descriptor, void* weapon, void* captured_list,
                std::uint32_t captured_ammo, void*& descriptor) {
        if (bindings_.dispatch_ready && !bindings_.dispatch_ready()) return false;
        if (captured_ammo != 40) { ++diagnostics.unrelated; return false; }
        const auto address = reinterpret_cast<std::uintptr_t>(weapon);
        std::uintptr_t vtable = 0;
        std::uint8_t alive = 0;
        struct Identity { std::uint32_t category, equipment_id; } identity{};
        if (!read_stormbolt_field(address, 0, vtable) || vtable != bindings_.weapon_vtable ||
            !read_stormbolt_field(address, 0xC, alive) || !(alive & 0xE) ||
            !read_stormbolt_field(address, 0x9F8, identity)) {
            ++diagnostics.rejected; return false;
        }
        if (identity.category != 13 || (identity.equipment_id != 238 && identity.equipment_id != 239)) {
            ++diagnostics.unrelated; return false;
        }
        std::uint32_t source_count=0;
        if (!read_stormbolt_field(address,0x2388,source_count) || !source_count) {
            ++diagnostics.unrelated; return false;
        }
        StormboltInstanceToken token{};
        StormboltStageState stage{};
        if (!store_.token_for_live_address(address, token) || !store_.snapshot(token, stage)) {
            ++diagnostics.untracked; return false;
        }
        void* held_list = nullptr;
        const auto list_address = reinterpret_cast<std::uintptr_t>(captured_list);
        StormboltShellRecordAt method = nullptr;
        if (!captured_list || !read_stormbolt_field(address, 0x1D90, held_list) ||
            held_list != captured_list || !read_stormbolt_field(list_address, 0, vtable) ||
            vtable != bindings_.list_vtable || !read_stormbolt_field(vtable, 0x88, method) ||
            method != bindings_.record_at) {
            ++diagnostics.rejected; return false;
        }
        // Native getter has no writes. Execute it after releasing the store lock.
        // Preserve a descriptor already changed by a different participant.
        const void* base_record = method(captured_list, 40);
        void* base_descriptor = nullptr;
        if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(base_record), 8, base_descriptor) ||
            !base_descriptor || base_descriptor != original_descriptor) {
            ++diagnostics.rejected; return false;
        }
        const auto selector = select_stormbolt_resource(stage);
        const void* selected_record = method(captured_list, selector);
        void* selected_descriptor = nullptr;
        if (!read_stormbolt_field(reinterpret_cast<std::uintptr_t>(selected_record), 8, selected_descriptor) ||
            !selected_descriptor) {
            ++diagnostics.rejected; return false;
        }
        descriptor = selected_descriptor;
        ++diagnostics.selected;
        return true;
    }

    static void* dispatch(void* descriptor, void* weapon, void* owner, const void* context,
                         std::uintptr_t caller_return, void* captured_list,
                         std::uint32_t captured_ammo) {
        auto& hook_state = *active_;
        void* chosen = descriptor;
        bool selected = false;
        if (caller_return == hook_state.bindings_.caller_return) {
            try { selected = hook_state.select(descriptor, weapon, captured_list, captured_ammo, chosen); }
            catch (...) { ++hook_state.diagnostics.selection_exceptions; chosen = descriptor; }
        } else { ++hook_state.diagnostics.unrelated; }
        // Exactly one original wrapper call. Preserve optional owner/context and
        // return, including null. Never synthesize a null, retry, or skip native
        // post-creation processing at the return site (which assumes nonnull).
        if(selected && hook_state.bindings_.parameter_vtable) {
            if(hook_state.bindings_.before_energy) {
                StormboltInstanceToken token{};StormboltStageState shot{};
                const bool clamped=hook_state.store_.token_for_live_address(
                    reinterpret_cast<std::uintptr_t>(weapon),token) &&
                    hook_state.store_.transact(token,[](auto& state) noexcept {
                        state.shot_snapshot=(std::min)(state.shot_snapshot,std::uint32_t{8});return true;
                    }) && hook_state.store_.snapshot(token,shot);
                if(clamped && hook_state.bindings_.before_energy(weapon,shot.shot_snapshot))
                    ++hook_state.diagnostics.self_damage_applied;
                else ++hook_state.diagnostics.self_damage_rejected;
            }
            if(hook_state.consume_energy(weapon))++hook_state.diagnostics.energy_applied;
            else ++hook_state.diagnostics.energy_rejected;
            if(hook_state.bindings_.after_energy) {
                StormboltInstanceToken token{};StormboltStageState shot{};
                if(hook_state.store_.token_for_live_address(reinterpret_cast<std::uintptr_t>(weapon),token) &&
                    hook_state.store_.snapshot(token,shot))
                    hook_state.bindings_.after_energy(weapon,(std::min)(shot.shot_snapshot,std::uint32_t{8}));
            }
        }
        void* result = hook_state.original_(chosen, weapon, owner, context);
        if (selected && !result) ++hook_state.diagnostics.native_null;
        return result;
    }
public:
    explicit StormboltShellHook(StormboltInstanceStore<Capacity>& instance_store)
        : store_(instance_store) { }
    StormboltShellHook(const StormboltShellHook&) = delete;
    StormboltShellHook& operator=(const StormboltShellHook&) = delete;

    MH_STATUS add_to_transaction(HookTransaction& transaction,
                                const StormboltShellHookBindings& bindings) {
        if (active_ || stormbolt_shell_dispatch_target) return MH_ERROR_ALREADY_CREATED;
        if (!bindings.create_entry || !bindings.caller_return || !bindings.weapon_vtable ||
            !bindings.list_vtable || !bindings.record_at) return MH_ERROR_NOT_EXECUTABLE;
        bindings_ = bindings;
        active_ = this;
        stormbolt_shell_dispatch_target = &dispatch;
        // Retain the dispatcher after rollback; bootstrap owns quiescence and
        // process lifetime just as for the constructor/destructor hooks.
        return transaction.add(bindings.create_entry,
            reinterpret_cast<void*>(&stormbolt_shell_hook_entry), reinterpret_cast<void**>(&original_));
    }
};
} // namespace stormslinger
