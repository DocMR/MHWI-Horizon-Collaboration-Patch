#include <windows.h>
#include <dinput.h>
#include <array>
#include <cstdio>
#include <cwchar>
using Create=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
int wmain(int argc,wchar_t** argv){
 if(argc!=2 && argc!=3)return 1;
 const bool runtimeOff=argc==3 && !wcscmp(argv[2],L"--runtime-off");
 if(argc==3 && !runtimeOff)return 1;
 std::array<wchar_t,32768> path{};if(!GetSystemDirectoryW(path.data(),static_cast<UINT>(path.size())))return 2;
 wcscat_s(path.data(),path.size(),L"\\dinput8.dll");
 HMODULE system=LoadLibraryExW(path.data(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!system)return 3;
 HMODULE proxy=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!proxy || proxy==system)return 4;
 const char* required[]={"DirectInput8Create","DllCanUnloadNow","DllGetClassObject","DllRegisterServer","DllUnregisterServer","GetdfDIJoystick"};
 for(auto name:required)if(!GetProcAddress(proxy,name))return 5;
 auto original=reinterpret_cast<Create>(GetProcAddress(system,"DirectInput8Create"));
 auto forwarded=reinterpret_cast<Create>(GetProcAddress(proxy,"DirectInput8Create"));
 IDirectInput8W* a=nullptr;IDirectInput8W* b=nullptr;
 HRESULT systemResult=original(GetModuleHandleW(nullptr),0x0800,IID_IDirectInput8W,reinterpret_cast<void**>(&a),nullptr);
 HRESULT proxyResult=forwarded(GetModuleHandleW(nullptr),0x0800,IID_IDirectInput8W,reinterpret_cast<void**>(&b),nullptr);
 if(systemResult!=proxyResult || FAILED(proxyResult) || !a || !b)return 6;
 a->Release();b->Release();
 using Status=long(*)();auto status=reinterpret_cast<Status>(GetProcAddress(proxy,"StormslingerBootstrapStatus"));if(!status)return 7;
 if(runtimeOff) {
  if(status()!=0)return 8;
  puts("PASS: six system exports, real DirectInput8 COM forwarding; project bootstrap remains inactive");
  FreeLibrary(system);return 0;
 }
 auto deadline=GetTickCount64()+5000;
 while(status()>=0 && GetTickCount64()<deadline)Sleep(10);
 if(status()!=-10)return 8;
 puts("PASS: six system exports, real DirectInput8 COM creation, separate system module, wrong-host bootstrap rejection");
 // Proxy is pinned by design until process exit; never force-unload it.
 FreeLibrary(system);return 0;
}
