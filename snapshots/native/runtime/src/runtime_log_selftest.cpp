#ifdef NDEBUG
#undef NDEBUG
#endif
#include "runtime_log.h"
#include <cassert>
#include <cwchar>
using Exit=void(WINAPI*)(UINT);
Exit fixtureExit=&ExitProcess;
using Terminate=BOOL(WINAPI*)(HANDLE,UINT);
Terminate fixtureTerminate=&TerminateProcess;
int wmain(int argc,wchar_t** argv) {
 assert(argc==3);
 const bool rotateMode=!wcscmp(argv[1],L"rotate");
 assert(stormslinger::runtime_log::initialize_directory(argv[2],rotateMode?512:2*1024*1024));
 for(int i=0;i<(rotateMode?100:2);++i)stormslinger::runtime_log::state(i,6,0,0,0,0,0,static_cast<unsigned>(i));
 SetLastError(1234);
 stormslinger::runtime_log::notice(3,1,8);
 assert(GetLastError()==1234);
 for(unsigned i=0;i<100;++i){stormslinger::runtime_log::resource_wait(2000,i);assert(GetLastError()==1234);}
 if(!wcscmp(argv[1],L"abrupt")) {TerminateProcess(GetCurrentProcess(),99);return 99;}
 if(!wcscmp(argv[1],L"self")) {
  assert(stormslinger::runtime_log::observe_terminate_slot(reinterpret_cast<void**>(&fixtureTerminate)));
  fixtureTerminate(GetCurrentProcess(),0);return 1;
 }
 assert(stormslinger::runtime_log::observe_exit_slot(reinterpret_cast<void**>(&fixtureExit)));
 fixtureExit(0);return 1;
}
