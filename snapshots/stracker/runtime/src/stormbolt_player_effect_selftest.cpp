#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_player_effect_adapter.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace stormslinger;
namespace {
template<std::size_t N> using Bytes = std::array<std::byte, N>;
Bytes<0x400> g_component{}, g_actor{}, g_other_actor{};
Bytes<0x2500> g_buffer{}, g_other_buffer{};
Bytes<0xC0> g_result{};
Bytes<16> g_resource{}, g_unrelated_resource{}, g_provider{}, g_post_manager{}, g_dti{}, g_context{};
void* g_provider_slot = g_provider.data();
void* g_post_slot = g_post_manager.data();
std::vector<int> g_calls;
bool g_has_active = false, g_change_owner = false, g_change_buffer = false;
bool g_null_result = false, g_throw_create = false, g_throw_post = false;
std::uint8_t g_post_byte = 0;
std::uint32_t g_group = 0x7EE;
std::size_t g_bank_offset=0x1A40;
void* g_assigned_bank = nullptr;
void* g_assigned_result = nullptr;
unsigned g_resolver_mode = 0;

template<class T> void put(void* p, std::size_t offset, T value) {
    std::memcpy(static_cast<std::byte*>(p) + offset, &value, sizeof(value));
}
void* construct(void* dti, void* storage) {
    assert(dti == g_dti.data());
    assert(reinterpret_cast<std::uintptr_t>(storage) % 16 == 0);
    g_calls.push_back(1);
    // Sentinels detect skipping native initialization and unintended writes.
    std::memset(storage, 0xA5, 0xF0);
    put(storage, 0xE8, 1.0f);
    put(storage, 0xEC, std::uint16_t{0});
    return storage;
}
void select_actor(void* parameter, void* actor) {
    assert(actor == g_actor.data());
    g_calls.push_back(2);
    put(parameter, 0xBC, std::uint32_t{4});
}
void* create(void* provider, void* context, void* resource, std::uint32_t group,
    std::uint32_t record, void* parameter, void* owner, void* extra) {
    assert(provider == g_provider.data() && context == g_context.data());
    assert(resource == g_resource.data()); // component+30, not+20/+28
    assert(group == g_group && record == 0 && owner == g_actor.data() && !extra);
    Bytes<0xF0> expected{};
    expected.fill(std::byte{0xA5});
    put(expected.data(), 0xE8, 1.0f);
    put(expected.data(), 0xEC, std::uint16_t{0});
    put(expected.data(), 0xBC, std::uint32_t{4});
    put(expected.data(), 0x60, std::uint32_t{0xFFFFFFFF});
    put(expected.data(), 0x70, owner);
    put(expected.data(), 0xE0, g_component.data() + 0x368);
    assert(std::memcmp(parameter, expected.data(), expected.size()) == 0);
    g_calls.push_back(3);
    if (g_throw_create) throw std::runtime_error("native create");
    if (g_change_owner) put(g_component.data(), 0x18, g_other_actor.data());
    if (g_change_buffer) put(g_component.data(), 8, g_other_buffer.data());
    return g_null_result ? nullptr : g_result.data();
}
std::uint8_t post_needed(void* owner) {
    assert(owner == (g_change_owner ? g_other_actor.data() : g_actor.data()));
    g_calls.push_back(4);
    return g_post_byte;
}
void post(void* manager, void* result, const void* position, std::uint32_t selector) {
    assert(manager == g_post_manager.data());
    assert(result == (g_null_result ? nullptr : g_result.data()));
    assert(position == (g_change_owner ? g_other_actor.data() : g_actor.data()) + 0x160);
    assert(selector == 1);
    g_calls.push_back(5);
    if (g_throw_post) throw std::runtime_error("native post");
}
bool active(const void* bank) {
    assert(bank == g_buffer.data() + g_bank_offset);
    g_calls.push_back(6);
    return g_has_active;
}
void* assign(void* bank, void* result) {
    g_calls.push_back(7);
    g_assigned_bank = bank;
    g_assigned_result = result;
    return bank;
}
void clear(void* component) {
    assert(component == g_component.data());
    g_calls.push_back(8);
}
StormboltPlayerEffectApi make_api() {
    return {g_dti.data(), construct, select_actor, &g_provider_slot, g_context.data(),
        create, post_needed, &g_post_slot, post, active, assign, clear};
}
bool resolve_provider(const void* context, StormboltEffectProviderSnapshot& output) {
    assert(context == &g_resolver_mode);
    g_calls.push_back(9);
    // Prove the consumer uses the pair returned here, not a later singleton read.
    g_provider_slot = g_unrelated_resource.data();
    if (g_resolver_mode == 1) return false;
    output = {g_resolver_mode == 2 ? nullptr : g_provider.data(), g_resolver_mode == 3 ? nullptr : create};
    return true;
}
void reset() {
    g_calls.clear();
    g_component.fill(std::byte{});
    put(g_component.data(), 8, g_buffer.data());
    put(g_component.data(), 0x18, g_actor.data());
    put(g_component.data(), 0x20, g_unrelated_resource.data());
    put(g_component.data(), 0x28, g_unrelated_resource.data());
    put(g_component.data(), 0x30, g_resource.data());
    g_has_active = g_change_owner = g_change_buffer = false;
    g_null_result = g_throw_create = g_throw_post = false;
    g_post_byte = 0;
    g_group = 0x7EE; g_bank_offset=0x1A40;
    g_assigned_bank = g_assigned_result = nullptr;
    g_provider_slot = g_provider.data();
}
} // namespace

int main() {
    auto api = make_api();
    assert(api.complete());
    assert(!StormboltPlayerEffectApi{}.complete());
    StormboltPlayerEffectAdapter adapter(api);
    for(unsigned mask=0;mask<16;++mask) {
        reset(); g_group=0x7DD; g_bank_offset=0x1B00;
        g_has_active=(mask&1)!=0; g_null_result=(mask&2)!=0;
        g_change_buffer=(mask&4)!=0; g_change_owner=(mask&8)!=0;
        g_post_byte=1;
        assert(adapter.update_last_effect(g_component.data()));
        if(g_has_active) { assert((g_calls==std::vector<int>{6})); assert(!g_assigned_bank); }
        else {
            assert((g_calls==std::vector<int>{6,1,2,3,4,5,7}));
            assert(g_assigned_bank==(g_change_buffer ? g_other_buffer.data() : g_buffer.data())+0x1B00);
            assert(g_assigned_result==(g_null_result ? nullptr : g_result.data()));
        }
    }
    unsigned cases = 0;
    for (const auto post_byte : {std::uint8_t{0}, std::uint8_t{1}, std::uint8_t{0x80}}) {
        for (unsigned mask = 0; mask != 8; ++mask) {
            reset();
            g_post_byte = post_byte;
            g_change_owner = (mask & 1) != 0;
            g_change_buffer = (mask & 2) != 0;
            g_null_result = (mask & 4) != 0;
            adapter.update_transition_effect(g_component.data(), true);
            assert(g_calls == (post_byte ? std::vector<int>{6, 1, 2, 3, 4, 5, 7}
                                        : std::vector<int>{6, 1, 2, 3, 4, 7}));
            assert(g_assigned_bank == (g_change_buffer ? g_other_buffer.data() : g_buffer.data()) + 0x1A40);
            assert(g_assigned_result == (g_null_result ? nullptr : g_result.data()));
            ++cases;
        }
    }
    reset();
    g_has_active = true;
    adapter.update_transition_effect(g_component.data(), true);
    assert(g_calls == std::vector<int>{6});
    for (bool currently_active : {false, true}) {
        reset();
        g_has_active = currently_active;
        adapter.update_transition_effect(g_component.data(), false);
        assert(g_calls == std::vector<int>{8}); // native helper owns all cleanup
    }
    reset();
    g_group = 0x7DD;
    const auto request = adapter.request_actor_effect(g_component.data(), g_group);
    assert(request.provider_resolved && request.value == g_result.data());
    assert(g_calls == (std::vector<int>{1, 2, 3, 4}));
    for (bool throw_in_post : {false, true}) {
        reset();
        g_post_byte = 1;
        g_throw_post = throw_in_post;
        g_throw_create = !throw_in_post;
        bool observed = false;
        try { adapter.update_transition_effect(g_component.data(), true); }
        catch (const std::runtime_error&) { observed = true; }
        assert(observed && !g_assigned_bank);
        assert(g_calls == (throw_in_post ? std::vector<int>{6, 1, 2, 3, 4, 5}
                                        : std::vector<int>{6, 1, 2, 3}));
    }
    auto resolved_api = api;
    resolved_api.provider_slot = nullptr; resolved_api.create = nullptr;
    resolved_api.provider_resolver_context = &g_resolver_mode;
    resolved_api.resolve_provider = resolve_provider;
    assert(resolved_api.complete());
    StormboltPlayerEffectAdapter resolved(resolved_api);
    for (g_resolver_mode = 0; g_resolver_mode <= 3; ++g_resolver_mode) {
        reset();
        assert(resolved.update_transition_effect(g_component.data(), true) == (g_resolver_mode == 0));
        assert(g_calls == (g_resolver_mode == 0 ? std::vector<int>{6,1,2,9,3,4,7} : std::vector<int>{6,1,2,9}));
        assert((g_assigned_bank != nullptr) == (g_resolver_mode == 0));
    }
    reset(); g_resolver_mode = 0; g_null_result = true;
    assert(resolved.update_transition_effect(g_component.data(), true));
    assert(g_assigned_bank && !g_assigned_result); // Native null is still assigned.
    reset(); g_has_active = true;
    assert(resolved.update_transition_effect(g_component.data(), true) && g_calls == std::vector<int>{6});
    reset();
    assert(resolved.update_transition_effect(g_component.data(), false) && g_calls == std::vector<int>{8});
    std::printf("Player effect adapter passed: %u request combinations, provider snapshot/absence, reuse/clear, raw result, rereads and native exceptions.\n", cases);
}
