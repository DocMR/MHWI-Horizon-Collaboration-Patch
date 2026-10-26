#pragma once
#include "horizon_quest_catalog.h"
#include <cstddef>
#include <cstdint>

namespace horizon_other {
struct QuestNoNode {
    void* vtable;
    std::int32_t id;
    std::uint32_t reserved;
};
static_assert(sizeof(QuestNoNode) == 16, "Native QuestNo node layout");

struct QuestListApi {
    void* (*allocate)(std::size_t bytes, unsigned alignment);
    QuestNoNode* (*construct)(void* memory);
    void (*destroy)(QuestNoNode* node);
    void (*grow)(void* native_array, std::uint32_t capacity);
};
void destroy_native_quest_node(QuestNoNode* node);
// The shared bootstrap must certify the executable/code version before binding.
QuestListApi bind_quest_list_api(const unsigned char* certified_image) noexcept;
enum class AppendQuestResult {
    unrelated_resource, unchanged, appended, invalid_resource, missing_api,
    allocation_failed, growth_failed
};

// Call only after successful original native deserialization, under its resource
// publication lifetime. The native resource owns all successfully appended nodes.
AppendQuestResult append_quest_resource_nodes(void* resource, const QuestListApi& api);
bool assign_loaded_quest_id(void* resource) noexcept;

// Native optional count1355C50 and lookup1355C40 consume the shared redirected
// mapping, including all six IDs. Do not add a second count/lookup extension:
// that would append the same quests twice after mapping activation.
// Resource registration does not grant prerequisites or completion state.
}
