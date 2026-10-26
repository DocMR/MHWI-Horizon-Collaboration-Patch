#pragma once
#include <atomic>
#include <cstdint>

namespace stormslinger {
// Development observation only. Readers sample fields without synchronization;
// no native decision, state flag or fallback depends on these values.
struct StormboltLiveProbe {
    std::uint32_t version=4;
    std::atomic<std::uint32_t> status_calls{0},status_inputs{0},status_eligible{0},status_flag_calls{0};
    std::atomic<std::uint32_t> status_count{0},status_threshold{0},status_part8{0},status_skill_count{0};
    std::atomic<std::uint32_t> source_calls{0},source_stage{0},source_part8{0},source_result{0};
    std::atomic<std::uint32_t> graph_attempts{0},graph_rows{0},graph_registry_count{0},graph_stage{0};
    std::atomic<std::uint32_t> local_construct_returns{0},local_reset_returns{0};
    std::atomic<std::uint32_t> construct_native_count{0},construct_native_timer_bits{0};
    std::atomic<std::uint32_t> reset_native_count{0},reset_native_timer_bits{0};
    std::atomic<std::uint32_t> source_direct_calls{0},source_command_calls{0},source_command_accepted{0};
    std::atomic<std::uint32_t> source_result2_calls{0},source_result3_calls{0};
};
static_assert(sizeof(StormboltLiveProbe)==28*sizeof(std::uint32_t),"Live probe wire layout");
}
extern "C" {
inline __declspec(dllexport) stormslinger::StormboltLiveProbe stormbolt_live_probe;
}
