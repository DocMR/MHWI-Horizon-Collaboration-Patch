#include "horizon_executable_identity.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#pragma comment(lib, "bcrypt.lib")
namespace horizon_other {
namespace {
#include "generated_executable_identity.inc"
struct Handles {
    HANDLE file = INVALID_HANDLE_VALUE;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    ~Handles() {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    }
};
}
ExecutableIdentityResult certify_running_executable(const std::uint8_t* image) noexcept {
    const auto module = GetModuleHandleW(nullptr);
    if (!image || image != reinterpret_cast<const std::uint8_t*>(module))
        return ExecutableIdentityResult::wrong_module;
    wchar_t path[32768]{};
    const auto length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768) return ExecutableIdentityResult::file_unavailable;
    Handles handles;
    handles.file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (handles.file == INVALID_HANDLE_VALUE) return ExecutableIdentityResult::file_unavailable;
    if (BCryptOpenAlgorithmProvider(&handles.algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptCreateHash(handles.algorithm, &handles.hash, nullptr, 0, nullptr, 0, 0) < 0)
        return ExecutableIdentityResult::hash_failed;
    std::uint8_t bytes[65536]{};
    for (;;) {
        DWORD read = 0;
        if (!ReadFile(handles.file, bytes, sizeof(bytes), &read, nullptr))
            return ExecutableIdentityResult::file_unavailable;
        if (!read) break;
        if (BCryptHashData(handles.hash, bytes, read, 0) < 0) return ExecutableIdentityResult::hash_failed;
    }
    std::uint8_t digest[32]{};
    if (BCryptFinishHash(handles.hash, digest, sizeof(digest), 0) < 0) return ExecutableIdentityResult::hash_failed;
    return std::memcmp(digest, kExecutableDigest, sizeof(digest)) == 0
        ? ExecutableIdentityResult::matched : ExecutableIdentityResult::mismatch;
}
}
