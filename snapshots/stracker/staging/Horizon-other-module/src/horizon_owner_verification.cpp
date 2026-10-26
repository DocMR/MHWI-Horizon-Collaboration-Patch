#include "horizon_owner_verification.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#pragma comment(lib, "bcrypt.lib")

namespace horizon_other {
bool verify_native_owner(const std::uint8_t* image, const TreeOwnerSpec& owner) noexcept {
    if (!image || !owner.bytes || owner.bytes > 16384) return false;
    std::uint8_t bytes[16384]{};
    SIZE_T read = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), image + owner.rva, bytes, owner.bytes, &read) ||
        read != owner.bytes) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return false;
    std::uint8_t digest[32]{};
    const auto status = BCryptHash(algorithm, nullptr, 0, bytes, static_cast<ULONG>(owner.bytes),
                                  digest, sizeof(digest));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return status >= 0 && std::memcmp(digest, owner.original_sha256, sizeof(digest)) == 0;
}
}
