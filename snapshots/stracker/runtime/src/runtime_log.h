#pragma once
#include <windows.h>
#include <cstddef>
namespace stormslinger::runtime_log {
#if defined(STORMSLINGER_HORIZON_STRACKER)
inline constexpr char version[]="1.1.0-stracker-candidate";
inline constexpr char build[]="horizon-complete-stracker-20261023-mapping-link-fix";
#elif defined(STORMSLINGER_HORIZON_COMPLETE)
inline constexpr char version[]="1.1.0-candidate";
inline constexpr char build[]="horizon-complete-20261022-c10-notice-close";
#else
inline constexpr char version[]="1.0.0";
inline constexpr char build[]="native-20261021-r2";
#endif
// These functions never throw or change gameplay success/failure decisions.
void begin() noexcept;
bool initialize_directory(const wchar_t* directory,std::size_t max_bytes=2*1024*1024) noexcept;
void state(long code,int stage,int mh,DWORD origin,DWORD error,int before,int after,unsigned attempts) noexcept;
void resource_wait(unsigned group,unsigned record) noexcept;
void notice(unsigned stage,unsigned page,unsigned language,bool confirmed=false,bool persisted=false) noexcept;
void startup_component(unsigned component,unsigned detail,int native_status=0) noexcept;
// Host authenticates the executable before supplying its ExitProcess IAT slot.
bool observe_exit_slot(void** slot) noexcept;
bool observe_terminate_slot(void** slot) noexcept;
}
