#pragma once
#include <cstdint>
namespace horizon_other {
enum class ExecutableIdentityResult { matched, wrong_module, file_unavailable, hash_failed, mismatch };
// Future in-process bootstrap: main module base plus read-only file SHA256.
// Native original code-owner certification is still required independently.
ExecutableIdentityResult certify_running_executable(const std::uint8_t* image) noexcept;
}
