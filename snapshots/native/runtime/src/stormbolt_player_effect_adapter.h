#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace stormslinger {
// PC cMhEfcParam. Native placement initializes the base and derived fields.
struct alignas(16) StormboltNativeEffectParameter {
    std::array<std::byte, 0xF0> bytes{};
};
static_assert(sizeof(StormboltNativeEffectParameter) == 0xF0);
using StormboltEffectCreate = void* (*)(void* provider, void* context, void* resource,
    std::uint32_t group, std::uint32_t record, void* parameter, void* owner, void* extra);
struct StormboltEffectProviderSnapshot {
    void* provider = nullptr;
    StormboltEffectCreate create = nullptr;
};
struct StormboltEffectRequestResult {
    bool provider_resolved = false;
    void* value = nullptr;
};

struct StormboltPlayerEffectApi {
    void* parameter_dti = nullptr;
    void* (*construct_parameter)(void* dti, void* storage) = nullptr; // 1AC6800
    void (*actor_selector)(void* parameter, void* actor) = nullptr; // 19A7350
    void** provider_slot = nullptr; // 51C5D48
    void* provider_context = nullptr; // address502CE18
    StormboltEffectCreate create = nullptr; // Only for an already-borrowed direct provider.
    std::uint8_t (*post_needed)(void* owner) = nullptr; // F9AD80, raw AL
    void** post_manager_slot = nullptr; // 4F87C90
    void (*post)(void* manager, void* result, const void* position,
        std::uint32_t selector) = nullptr; // F9AA40
    bool (*bank_active)(const void* bank) = nullptr; // 2360FE0
    void* (*assign_bank)(void* bank, void* provider_result) = nullptr; // 2360F20
    void (*clear_bank70)(void* component) = nullptr; // AE10F0
    const void* provider_resolver_context = nullptr; // Must outlive this copied API.
    bool (*resolve_provider)(const void* context, StormboltEffectProviderSnapshot& output) = nullptr;

    bool complete() const noexcept {
        return parameter_dti && construct_parameter && actor_selector &&
            provider_context && (resolve_provider ? provider_resolver_context != nullptr : provider_slot && create) && post_needed &&
            post_manager_slot && post && bank_active && assign_bank && clear_bank70;
    }
};

// Internal adapter, not an installed hook. Caller must supply a complete API,
// authenticate code/types, establish native component/owner/resource/provider
// lifetimes and serialized effect access, and satisfy resource readiness.
// This cannot bypass the unfinished EFX migration. Native calls run without
// sidecar locks/catch; exceptions propagate.
class StormboltPlayerEffectAdapter {
    const StormboltPlayerEffectApi api_;
    template<class T> static T read(const void* object, std::size_t offset) {
        T value{};
        std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));
        return value;
    }
    template<class T> static void write(void* object, std::size_t offset, T value) {
        std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
    }
    static void* bank70(void* component) {
        return read<std::byte*>(component, 8) + 0x1A40;
    }
public:
    explicit StormboltPlayerEffectAdapter(const StormboltPlayerEffectApi& api) : api_(api) { }

    // PS19233F0 resource+30 / actor-attachment request using the PC constructor
    // and provider ABI. AE2910 attaches to the weapon and uses19A72D0 instead;
    // it must not replace this request. Neither wrapper proves rendering.
    StormboltEffectRequestResult request_actor_effect(void* component, std::uint32_t group) const {
        StormboltNativeEffectParameter parameter{};
        api_.construct_parameter(api_.parameter_dti, &parameter);
        auto* owner = read<void*>(component, 0x18);
        api_.actor_selector(&parameter, owner);
        write(&parameter, 0x70, owner);
        write(&parameter, 0x60, std::uint32_t{0xFFFFFFFF});
        write(&parameter, 0xE0, static_cast<std::byte*>(component) + 0x368);
        auto* resource = read<void*>(component, 0x30);
        StormboltEffectProviderSnapshot provider{};
        if (api_.resolve_provider) {
            if (!api_.resolve_provider(api_.provider_resolver_context, provider) || !provider.provider || !provider.create)
                return {}; // No create/post/assignment with an unresolved provider.
        } else {
            provider = {*api_.provider_slot, api_.create};
        }
        auto* result = provider.create(provider.provider, api_.provider_context,
            resource, group, 0, &parameter, owner, nullptr);
        // PS re-reads the owner after the provider call; preserve raw result.
        owner = read<void*>(component, 0x18);
        if (api_.post_needed(owner))
            api_.post(*api_.post_manager_slot, result,
                static_cast<const std::byte*>(owner) + 0x160, 1);
        return {true,result}; // A native null result is distinct from failed resolution.
    }

    // PS39F2D70..DE0: the event was consumed by the enclosing Last update.
    // PC2360FE0 tests four 24-byte references starting at component buffer+1B00.
    bool update_last_effect(void* component) const {
        auto* bank=read<std::byte*>(component,8)+0x1B00;
        if (api_.bank_active(bank)) return true;
        const auto result=request_actor_effect(component,0x7DD);
        if (!result.provider_resolved) return false;
        bank=read<std::byte*>(component,8)+0x1B00;
        api_.assign_bank(bank,result.value); // Preserve native null result.
        return true;
    }

    // Only the effect branch after native owner/event gates. Ammo40, weapon
    // flags, persistence, animation and query remain the action's responsibility.
    // PS event+178 bit28 selects enable/disable here.
    bool update_transition_effect(void* component, bool enabled) const {
        if (!enabled) {
            api_.clear_bank70(component);
            return true;
        }
        if (api_.bank_active(bank70(component))) return true;
        const auto result = request_actor_effect(component, 0x7EE);
        if (!result.provider_resolved) return false;
        // Native creation may change the buffer. PS loads it again.
        api_.assign_bank(bank70(component), result.value);
        return true;
    }
};
} // namespace stormslinger
