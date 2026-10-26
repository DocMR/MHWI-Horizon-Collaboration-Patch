#pragma once
#include <cstdint>
namespace horizon_other {
inline bool focus_pendant_exclusion_result(std::int32_t row,bool original_excluded) noexcept {
    return (row==87 || (row>=98 && row<=100)) ? false : original_excluded;
}
}
