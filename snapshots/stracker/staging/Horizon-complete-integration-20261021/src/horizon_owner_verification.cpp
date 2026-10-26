#include "horizon_owner_verification.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#if defined(STORMSLINGER_HORIZON_STRACKER)
#include "../../../runtime/src/quest_loader_owner_copy.h"
#include <array>
#include "../../../runtime/src/runtime_log.h"
extern "C" int StormslingerCheckExecutable(const wchar_t*,char*,size_t);
#endif
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
    bool matched=status >= 0 && std::memcmp(digest, owner.original_sha256, sizeof(digest)) == 0;
#if defined(STORMSLINGER_HORIZON_STRACKER)
    if(!matched) {
        const auto loader=GetModuleHandleW(L"QuestLoader.dll");
        std::array<wchar_t,32768> path{};char hash[65]{};
        const auto used=loader?GetModuleFileNameW(loader,path.data(),static_cast<DWORD>(path.size())):0;
        if(used && used<path.size() && StormslingerCheckExecutable(path.data(),hash,sizeof(hash))>=0 &&
            !std::strcmp(hash,"97380a19c12822c318ebc7ef09df601823cbf33ec674e1aee9f8a690d5422c08") &&
            stormslinger::reconstruct_quest_loader_owner(reinterpret_cast<std::uintptr_t>(image),
                reinterpret_cast<std::uintptr_t>(loader),owner.rva,bytes,owner.bytes,
                [](std::uintptr_t address,void* data,std::size_t count) {
                    SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),data,count,&got) && got==count;
                })) {
            matched=BCryptHash(algorithm,nullptr,0,bytes,static_cast<ULONG>(owner.bytes),digest,sizeof(digest))>=0 &&
                !std::memcmp(digest,owner.original_sha256,sizeof(digest));
        }
    }
#endif
    BCryptCloseAlgorithmProvider(algorithm, 0);
#if defined(STORMSLINGER_HORIZON_STRACKER)
    if(!matched)stormslinger::runtime_log::startup_component(100,owner.rva,static_cast<int>(owner.bytes));
#endif
    return matched;
}
}
