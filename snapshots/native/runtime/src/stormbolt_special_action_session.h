#pragma once
#include "stormbolt_native_bindings.h"

namespace stormslinger {
// Stable ownership for the complete special action. Native API authentication,
// resource readiness and borrowed lifetimes remain the bootstrap's responsibility.
// Never move this object: the copied effect API points into its identity member.
template<std::size_t Capacity>
class StormboltSpecialActionSession {
    const StormboltEffectProviderIdentity identity_;
    const StormboltPlayerEffectApi effect_api_;
    StormboltTransitionAmmoAdapter<Capacity> ammo_;
    StormboltPlayerEffectAdapter effects_;
    StormboltSpecialToNormalAdapter<Capacity> core_;
    StormboltSpecialToNormalHook<Capacity> hook_;
    const StormboltSpecialToNormalBindings bindings_;

    static StormboltPlayerEffectApi bind_effects(StormboltPlayerEffectApi api,
        const StormboltEffectProviderIdentity& identity) noexcept {
        api.create = nullptr;
        api.provider_resolver_context = &identity;
        api.resolve_provider = &resolve_stormbolt_effect_provider;
        return api;
    }
public:
    static bool complete(const StormboltSpecialNativeEndpoints& endpoints) noexcept {
        const auto& id = endpoints.provider_identity;
        const auto& hook = endpoints.hook;
        return id.singleton_slot && id.vtable && id.dti && id.dti_vtable &&
            id.dti_getter && id.record_create && id.create &&
            hook.action_vtable && hook.actor_vtable && hook.weapon_vtable &&
            hook.original_enter && hook.original_update && hook.original_exit && hook.ready &&
            endpoints.action.complete() && endpoints.save.complete() &&
            bind_effects(endpoints.effects, id).complete();
    }
    StormboltSpecialActionSession(StormboltInstanceStore<Capacity>& store,
        const StormboltSpecialNativeEndpoints& endpoints)
        : identity_(endpoints.provider_identity),
          effect_api_(bind_effects(endpoints.effects, identity_)),
          ammo_(store, endpoints.save), effects_(effect_api_),
          core_(store, ammo_, effects_, endpoints.action), hook_(store, core_),
          bindings_(endpoints.hook) { }
    StormboltSpecialActionSession(const StormboltSpecialActionSession&) = delete;
    StormboltSpecialActionSession& operator=(const StormboltSpecialActionSession&) = delete;
    bool install() { return hook_.install(bindings_); }
    bool restore() { return hook_.restore(); }
    bool active() const noexcept { return hook_.active(); }
};
} // namespace stormslinger
