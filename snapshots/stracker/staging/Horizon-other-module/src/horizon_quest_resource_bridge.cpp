#include "horizon_quest_resource_bridge.h"
#include <array>
#include <cstring>
#include <limits>

namespace horizon_other {
namespace {
constexpr std::size_t maximum_quests = 6;

bool resource_name_equals(const void* resource, const char* expected) noexcept {
    const auto name = static_cast<const char*>(resource) + 0xC;
    const auto length = std::strlen(expected);
    return std::memcmp(name, expected, length) == 0 && name[length] == '\0';
}

void destroy_pending(std::array<QuestNoNode*, maximum_quests>& pending,
                     const QuestListApi& api) {
    for (auto& node : pending) {
        if (node) api.destroy(node);
        node = nullptr;
    }
}
}

void destroy_native_quest_node(QuestNoNode* node) {
    if (!node) return;
    using DeletingDestructor = void* (*)(QuestNoNode*, unsigned);
    auto table = static_cast<void**>(node->vtable);
    reinterpret_cast<DeletingDestructor>(table[0])(node, 1);
}

QuestListApi bind_quest_list_api(const unsigned char* certified_image) noexcept {
    if (!certified_image) return {};
    return {reinterpret_cast<decltype(QuestListApi::allocate)>(certified_image + 0x32A350),
            reinterpret_cast<decltype(QuestListApi::construct)>(certified_image + 0x32A330),
            &destroy_native_quest_node,
            reinterpret_cast<decltype(QuestListApi::grow)>(certified_image + 0x249B70)};
}

bool assign_loaded_quest_id(void* resource) noexcept {
    if (!resource) return false;
    std::size_t count = 0;
    const auto quests = quest_catalog(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (resource_name_equals(resource, quests[i].resource_name)) {
            auto id = reinterpret_cast<std::int32_t*>(static_cast<char*>(resource) + 0xE0);
            *id = quests[i].id;
            return true;
        }
    }
    return false;
}

AppendQuestResult append_quest_resource_nodes(void* resource, const QuestListApi& api) {
    if (!resource) return AppendQuestResult::invalid_resource;
    if (!resource_name_equals(resource, "quest\\questNoList")) {
        return AppendQuestResult::unrelated_resource;
    }
    if (!api.allocate || !api.construct || !api.destroy || !api.grow) {
        return AppendQuestResult::missing_api;
    }
    auto bytes = static_cast<char*>(resource);
    auto& count = *reinterpret_cast<int*>(bytes + 0xB8);
    auto& capacity = *reinterpret_cast<int*>(bytes + 0xBC);
    auto& entries = *reinterpret_cast<QuestNoNode***>(bytes + 0xC8);
    if (count < 0 || capacity < count || (count && !entries)) {
        return AppendQuestResult::invalid_resource;
    }
    std::size_t quest_count = 0;
    const auto quests = quest_catalog(quest_count);
    if (quest_count > maximum_quests) return AppendQuestResult::invalid_resource;
    std::array<bool, maximum_quests> missing{};
    int needed = 0;
    for (std::size_t i = 0; i < quest_count; ++i) {
        missing[i] = true;
        for (int j = 0; j < count; ++j) {
            if (entries[j] && entries[j]->id == quests[i].id) {
                missing[i] = false;
                break;
            }
        }
        if (missing[i]) ++needed;
    }
    if (!needed) return AppendQuestResult::unchanged;
    if (count > (std::numeric_limits<int>::max)() - needed) {
        return AppendQuestResult::invalid_resource;
    }
    std::array<QuestNoNode*, maximum_quests> pending{};
    for (std::size_t i = 0; i < quest_count; ++i) {
        if (!missing[i]) continue;
        auto memory = api.allocate(sizeof(QuestNoNode), 0x10);
        if (!memory) {
            destroy_pending(pending, api);
            return AppendQuestResult::allocation_failed;
        }
        // The certified native constructor32A330 always returns its nonnull input;
        // callbacks must bind that constructor, not a fallible replacement.
        pending[i] = api.construct(memory);
        pending[i]->id = quests[i].id;
    }
    const int original_count = count;
    if (capacity < count + needed) {
        api.grow(bytes + 0xB0, static_cast<std::uint32_t>(count + needed));
    }
    if (count != original_count || !entries || capacity < original_count + needed) {
        destroy_pending(pending, api);
        return AppendQuestResult::growth_failed;
    }
    int next = original_count;
    for (auto node : pending) {
        if (node) entries[next++] = node;
    }
    count = next;
    return AppendQuestResult::appended;
}

}
