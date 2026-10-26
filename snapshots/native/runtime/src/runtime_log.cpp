#include "runtime_log.h" // Build identity is persisted in every diagnostic record.
#include <array>
#include <atomic>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <climits>
namespace stormslinger::runtime_log {
namespace {
SRWLOCK lock=SRWLOCK_INIT;
std::array<wchar_t,32768> directory{};
std::size_t limit=2*1024*1024;
bool initialized=false;
long lastCode=LONG_MIN;
unsigned resourceRecords=0;
char session[64]{};
using Exit=void(WINAPI*)(UINT);
Exit originalExit=nullptr;
std::atomic<bool> exitObserverDecided{false};
using Terminate=BOOL(WINAPI*)(HANDLE,UINT);
Terminate originalTerminate=nullptr;
std::atomic<bool> terminateObserverDecided{false};
bool path(std::array<wchar_t,32768>& output,const wchar_t* name) {
 return swprintf_s(output.data(),output.size(),L"%s\\%s",directory.data(),name)>0;
}
void marker(bool closing,UINT code=0) {
 std::array<wchar_t,32768> file{};if(!path(file,closing?L"session.json":L"running.marker"))return;
 HANDLE handle=CreateFileW(file.data(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(handle==INVALID_HANDLE_VALUE)return;
 char data[384]{};int n=sprintf_s(data,"{\"session\":\"%s\",\"version\":\"%s\",\"build\":\"%s\",\"exit_request_observed\":%s,\"exit_code\":%u}\n",session,version,build,closing?"true":"false",code);
 DWORD written=0;if(n>0)WriteFile(handle,data,static_cast<DWORD>(n),&written,nullptr);CloseHandle(handle);
 if(closing && path(file,L"running.marker"))DeleteFileW(file.data());
}
void write(const char* event,const char* fields) {
 if(!initialized)return;
 SYSTEMTIME now{};GetSystemTime(&now);
 char data[2048]{};
 int n=sprintf_s(data,"{\"utc\":\"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ\",\"version\":\"%s\",\"build\":\"%s\",\"session\":\"%s\",\"pid\":%lu,\"event\":\"%s\"%s}\n",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds,version,build,session,GetCurrentProcessId(),event,fields);
 if(n<=0 || static_cast<std::size_t>(n)>limit)return;
 std::array<wchar_t,32768> current{},first{},second{};
 if(!path(current,L"runtime.jsonl") || !path(first,L"runtime.1.jsonl") || !path(second,L"runtime.2.jsonl"))return;
 WIN32_FILE_ATTRIBUTE_DATA info{};
 if(GetFileAttributesExW(current.data(),GetFileExInfoStandard,&info)) {
  const auto bytes=(static_cast<ULONGLONG>(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
  if(bytes+static_cast<unsigned>(n)>limit) {
   DeleteFileW(second.data());MoveFileExW(first.data(),second.data(),MOVEFILE_REPLACE_EXISTING);
   if(!MoveFileExW(current.data(),first.data(),MOVEFILE_REPLACE_EXISTING))return;
  }
 }
 HANDLE file=CreateFileW(current.data(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return;
 DWORD written=0;WriteFile(file,data,static_cast<DWORD>(n),&written,nullptr);CloseHandle(file);
}
void WINAPI exit_hook(UINT code) {
 AcquireSRWLockExclusive(&lock);
 char fields[64]{};sprintf_s(fields,",\"code\":%u",code);write("exit_requested",fields);marker(true,code);
 ReleaseSRWLockExclusive(&lock);
 originalExit(code);
}
BOOL WINAPI terminate_hook(HANDLE process,UINT code) {
 const DWORD previous=GetLastError();
 if(GetProcessId(process)==GetCurrentProcessId()) {
  AcquireSRWLockExclusive(&lock);
  char fields[64]{};sprintf_s(fields,",\"code\":%u",code);
  write("self_termination_requested",fields);marker(true,code);
  ReleaseSRWLockExclusive(&lock);
 }
 SetLastError(previous);
 return originalTerminate(process,code);
}
}
bool initialize_directory(const wchar_t* root,std::size_t max_bytes) noexcept {
 AcquireSRWLockExclusive(&lock);
 if(initialized) {ReleaseSRWLockExclusive(&lock);return true;}
 if(!root || max_bytes<512 || wcscpy_s(directory.data(),directory.size(),root)) {ReleaseSRWLockExclusive(&lock);return false;}
 if(!CreateDirectoryW(root,nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) {ReleaseSRWLockExclusive(&lock);return false;}
 limit=max_bytes;FILETIME time{};GetSystemTimeAsFileTime(&time);
 sprintf_s(session,"%08lx%08lx-%lu",time.dwHighDateTime,time.dwLowDateTime,GetCurrentProcessId());
 initialized=true;
 std::array<wchar_t,32768> running{};const bool prior=path(running,L"running.marker") && GetFileAttributesW(running.data())!=INVALID_FILE_ATTRIBUTES;
 write("runtime_begin",prior?",\"previous_marker_present\":true":",\"previous_marker_present\":false");marker(false);
 // session.json always describes this run, not the previous run's exit.
 std::array<wchar_t,32768> current{};
 if(path(current,L"session.json")) {
  HANDLE file=CreateFileW(current.data(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file!=INVALID_HANDLE_VALUE) {char data[320]{};int n=sprintf_s(data,"{\"session\":\"%s\",\"version\":\"%s\",\"build\":\"%s\",\"exit_request_observed\":false}\n",session,version,build);DWORD written=0;if(n>0)WriteFile(file,data,static_cast<DWORD>(n),&written,nullptr);CloseHandle(file);}
 }
 ReleaseSRWLockExclusive(&lock);return true;
}
void begin() noexcept {
 std::array<wchar_t,32768> root{},app{};
 const auto used=GetEnvironmentVariableW(L"LOCALAPPDATA",app.data(),static_cast<DWORD>(app.size()));
 if(!used || used>=app.size() || swprintf_s(root.data(),root.size(),L"%s\\Stormslinger",app.data())<=0)return;
 if(!CreateDirectoryW(root.data(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)return;
 if(wcscat_s(root.data(),root.size(),L"\\logs"))return;
 initialize_directory(root.data());
}
void state(long code,int stage,int mh,DWORD origin,DWORD error,int before,int after,unsigned attempts) noexcept {
 AcquireSRWLockExclusive(&lock);
 if(code!=lastCode) {
  lastCode=code;char fields[512]{};
  sprintf_s(fields,",\"code\":%ld,\"stage\":%d,\"mh_status\":%d,\"failure_origin\":%lu,\"win32_error\":%lu,\"phase_before\":%d,\"phase_after\":%d,\"attempts\":%u",code,stage,mh,origin,error,before,after,attempts);
  write("bootstrap_state",fields);
 }
 ReleaseSRWLockExclusive(&lock);
}
void notice(unsigned stage,unsigned page,unsigned language,bool confirmed,bool persisted) noexcept {
 const DWORD previousError=GetLastError();
 AcquireSRWLockExclusive(&lock);
 char fields[192]{};
 sprintf_s(fields,",\"stage\":%u,\"page\":%u,\"language_index\":%u,\"confirmed\":%s,\"persisted\":%s",stage,page,language,confirmed?"true":"false",persisted?"true":"false");
 write("content_notice",fields);
 ReleaseSRWLockExclusive(&lock);SetLastError(previousError);
}
void resource_wait(unsigned group,unsigned record) noexcept {
 const DWORD previousError=GetLastError();
 AcquireSRWLockExclusive(&lock);
 if(resourceRecords<3) {++resourceRecords;char fields[128]{};sprintf_s(fields,",\"group\":%u,\"record\":%u,\"occurrence\":%u",group,record,resourceRecords);write("resource_not_ready",fields);}
 ReleaseSRWLockExclusive(&lock);
 SetLastError(previousError);
}
bool observe_exit_slot(void** slot) noexcept {
 if(exitObserverDecided.exchange(true))return originalExit!=nullptr;
 const auto expected=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"ExitProcess"));
 SIZE_T read=0;void* current=nullptr;
 bool installed=slot && expected && ReadProcessMemory(GetCurrentProcess(),slot,&current,sizeof(current),&read) && read==sizeof(current) && current==expected;
 DWORD protection=0;
 if(installed)installed=VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection)!=0;
 if(installed) {
  originalExit=reinterpret_cast<Exit>(expected);
  installed=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&exit_hook),expected)==expected;
  DWORD ignored=0;VirtualProtect(slot,sizeof(void*),protection,&ignored);
  if(!installed)originalExit=nullptr;
 }
 AcquireSRWLockExclusive(&lock);write("exit_observer",installed?",\"installed\":true":",\"installed\":false");ReleaseSRWLockExclusive(&lock);
 return installed;
}
bool observe_terminate_slot(void** slot) noexcept {
 if(terminateObserverDecided.exchange(true))return originalTerminate!=nullptr;
 const auto expected=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"TerminateProcess"));
 SIZE_T read=0;void* current=nullptr;
 bool installed=slot && expected && ReadProcessMemory(GetCurrentProcess(),slot,&current,sizeof(current),&read) && read==sizeof(current) && current==expected;
 DWORD protection=0;
 if(installed)installed=VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection)!=0;
 if(installed) {
  originalTerminate=reinterpret_cast<Terminate>(expected);
  installed=InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(&terminate_hook),expected)==expected;
  DWORD ignored=0;VirtualProtect(slot,sizeof(void*),protection,&ignored);
  if(!installed)originalTerminate=nullptr;
 }
 AcquireSRWLockExclusive(&lock);write("self_termination_observer",installed?",\"installed\":true":",\"installed\":false");ReleaseSRWLockExclusive(&lock);
 return installed;
}
}
