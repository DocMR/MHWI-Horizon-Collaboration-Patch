#include <windows.h>
#include <dinput.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cwchar>
#if defined(STORMSLINGER_STANDALONE_PROXY)
#include "runtime_log.h"
#endif
struct QuestPlan { unsigned version; unsigned count; unsigned long long rva[12]; };
extern "C" int StormslingerArmEquipmentBootstrap(void*,const QuestPlan*);
extern "C" int StormslingerEquipmentBootstrapResult();
extern "C" int StormslingerCancelEquipmentBootstrap();
extern "C" int StormslingerWeaponTreeDiagnostics(int*,int*);
extern "C" void StormslingerEquipmentBootstrapDiagnostics(int*,int*,DWORD*,DWORD*);
namespace {
INIT_ONCE systemOnce=INIT_ONCE_STATIC_INIT;
HMODULE systemInput=nullptr;
std::atomic<long> bootstrapStatus{0};
std::atomic<ULONGLONG> moduleAttachTick{0},triggerTick{0},workerTick{0},lastAttemptMs{0};
std::atomic<unsigned> bootstrapAttempts{0};
BOOL CALLBACK LoadSystemInput(PINIT_ONCE,void*,void**) {
 std::array<wchar_t,32768> path{};unsigned n=GetSystemDirectoryW(path.data(),static_cast<unsigned>(path.size()));
 if(!n || n+13>=path.size())return FALSE;
 wcscat_s(path.data(),path.size(),L"\\dinput8.dll");
 systemInput=LoadLibraryExW(path.data(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
 return systemInput!=nullptr;
}
FARPROC Forward(const char* name) {
 if(!InitOnceExecuteOnce(&systemOnce,LoadSystemInput,nullptr,nullptr))return nullptr;
 return GetProcAddress(systemInput,name);
}
void RecordStatus(long code) {
 bootstrapStatus.store(code);
 int phaseBefore=-1,phaseAfter=-1;[[maybe_unused]] int treeEnabled=StormslingerWeaponTreeDiagnostics(&phaseBefore,&phaseAfter);
 int installStage=0,installStatus=0;DWORD origin=0,winerror=0;StormslingerEquipmentBootstrapDiagnostics(&installStage,&installStatus,&origin,&winerror);
#if defined(STORMSLINGER_STANDALONE_PROXY)
 stormslinger::runtime_log::state(code,installStage,installStatus,origin,winerror,phaseBefore,phaseAfter,bootstrapAttempts.load());
#else
 HMODULE own=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&RecordStatus),&own))return;
 std::array<wchar_t,32768> path{};DWORD used=GetModuleFileNameW(own,path.data(),static_cast<DWORD>(path.size()));
 if(!used || used>=path.size())return;
 wchar_t* slash=wcsrchr(path.data(),L'\\');if(!slash)return;slash[1]=0;
 if(wcscat_s(path.data(),path.size(),L"Stormslinger-development-status.json"))return;
 HANDLE file=CreateFileW(path.data(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE)return;
#if defined(STORMSLINGER_DIAGNOSTIC_NO_SHOOTING)
 const char* shootingRequested="false";
#else
 const char* shootingRequested="true";
#endif
 char buffer[1024];int n=sprintf_s(buffer,"{\"component\":\"StormslingerStandaloneDevelopment\",\"status\":%ld,\"bootstrap_install_stage\":%d,\"bootstrap_mh_status\":%d,\"bootstrap_failure_origin\":%lu,\"bootstrap_win32_error\":%lu,\"weapon_tree_enabled\":%s,\"weapon_phase_before\":%d,\"weapon_phase_after\":%d,\"module_attach_tick\":%llu,\"trigger_tick\":%llu,\"worker_tick\":%llu,\"attempts\":%u,\"last_attempt_ms\":%llu,\"shooting_startup_requested\":%s,\"native_callback_lifetime_assumed\":true,\"completion_persistence\":false,\"game_acceptance_verified\":false}\n",code,installStage,installStatus,origin,winerror,treeEnabled?"true":"false",phaseBefore,phaseAfter,moduleAttachTick.load(),triggerTick.load(),workerTick.load(),bootstrapAttempts.load(),lastAttemptMs.load(),shootingRequested);
 DWORD written=0;if(n>0)WriteFile(file,buffer,static_cast<DWORD>(n),&written,nullptr);CloseHandle(file);
#endif
}
DWORD WINAPI Bootstrap(void*) {
 workerTick.store(GetTickCount64());
#if defined(STORMSLINGER_STANDALONE_PROXY)
 stormslinger::runtime_log::begin();
#endif
 RecordStatus(1);
 const QuestPlan plan{1,12,{0xf2c320,0x1976110,0x19751b0,0x249b70,0x1b659b0,0x19750c0,0x478a20,0x1a10170,0x134ef50,0x134efe0,0x1355c50,0x1355c40}};
 ULONGLONG deadline=GetTickCount64()+90000;
 do {
  auto attemptStart=GetTickCount64();bootstrapAttempts.fetch_add(1);
  int result=StormslingerArmEquipmentBootstrap(GetModuleHandleW(nullptr),&plan);
  if(result==0) result=StormslingerEquipmentBootstrapResult();
  lastAttemptMs.store(GetTickCount64()-attemptStart);
  if(result==0){RecordStatus(3);return 0;}
  if(result!=-13){RecordStatus(result);return 0;}
  RecordStatus(2);Sleep(50);
 }while(GetTickCount64()<deadline);
 const int finalResult=StormslingerCancelEquipmentBootstrap();
 RecordStatus(finalResult==0?3:finalResult);return 0;
}
void BeginBootstrap() {
#if defined(STORMSLINGER_DIAGNOSTIC_NO_BOOTSTRAP)
 return; // Controlled comparison only; retain the complete forwarding path.
#else
 long expected=0;if(!bootstrapStatus.compare_exchange_strong(expected,1))return;
 triggerTick.store(GetTickCount64());
 HMODULE own=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&BeginBootstrap),&own)){RecordStatus(-40);return;}
 HANDLE thread=CreateThread(nullptr,0,Bootstrap,nullptr,0,nullptr);
 if(!thread){RecordStatus(-41);return;}CloseHandle(thread);
#endif
}
}
// Loader-lock entry only records a Kernel32 timestamp. No hook, IO, worker or
// system DLL loading happens here; normal exported calls initiate bootstrap.
extern "C" void StormslingerRecordProxyAttach(){moduleAttachTick.store(GetTickCount64());}
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
extern "C" void StormslingerBeginPluginBootstrap(){
 StormslingerRecordProxyAttach();
 BeginBootstrap();
}
#endif
extern "C" __declspec(dllexport) long StormslingerBootstrapStatus(){return bootstrapStatus.load();}
extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* result,LPUNKNOWN outer){
 using Fn=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
 BeginBootstrap();
 auto original=reinterpret_cast<Fn>(Forward("DirectInput8Create"));if(!original)return E_FAIL;
 return original(instance,version,iid,result,outer);
}
extern "C" HRESULT WINAPI DllCanUnloadNow(){using Fn=HRESULT(WINAPI*)();auto f=reinterpret_cast<Fn>(Forward("DllCanUnloadNow"));return f?f():S_FALSE;}
extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid,REFIID iid,LPVOID* output){using Fn=HRESULT(WINAPI*)(REFCLSID,REFIID,LPVOID*);auto f=reinterpret_cast<Fn>(Forward("DllGetClassObject"));return f?f(clsid,iid,output):CLASS_E_CLASSNOTAVAILABLE;}
extern "C" HRESULT WINAPI DllRegisterServer(){using Fn=HRESULT(WINAPI*)();auto f=reinterpret_cast<Fn>(Forward("DllRegisterServer"));return f?f():E_NOTIMPL;}
extern "C" HRESULT WINAPI DllUnregisterServer(){using Fn=HRESULT(WINAPI*)();auto f=reinterpret_cast<Fn>(Forward("DllUnregisterServer"));return f?f():E_NOTIMPL;}
extern "C" LPCDIDATAFORMAT WINAPI GetdfDIJoystick(){using Fn=LPCDIDATAFORMAT(WINAPI*)();auto f=reinterpret_cast<Fn>(Forward("GetdfDIJoystick"));return f?f():nullptr;}
