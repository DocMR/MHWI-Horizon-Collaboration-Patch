#include <windows.h>
#include <cstdint>
#include <vector>
struct Match { unsigned count; unsigned long long first; };
// Masked binary signatures allow individual register bits to vary.
extern "C" __declspec(dllexport) int StormslingerScan(const unsigned char* data, size_t size, const char* bits, Match* out) {
 if (!data || !bits || !out) return -1;
 out->count=0; out->first=0;
 std::vector<unsigned char> value, mask;
 while (*bits) {
  while (*bits==' ' || *bits=='\r' || *bits=='\n' || *bits=='\t') ++bits;
  if (!*bits) break;
  unsigned char v=0,m=0;
  for (int j=0;j<8;++j) {
   char c=*bits; if(c!='0' && c!='1' && c!='.') return -2;
   ++bits; v=(v<<1)|(c=='1'); m=(m<<1)|(c!='.');
  }
  if (*bits && *bits!=' ' && *bits!='\r' && *bits!='\n' && *bits!='\t') return -2;
  value.push_back(v);mask.push_back(m);
 }
 if (value.empty()) return -2;
 if (size<value.size()) return 0;
 size_t anchor=0;for(size_t i=1;i<mask.size();++i) if(mask[i]>mask[anchor]) anchor=i;
 for(size_t i=0;i<=size-value.size();++i) {
  if((data[i+anchor]&mask[anchor])!=value[anchor]) continue;
  size_t j=0;while(j<value.size() && (data[i+j]&mask[j])==value[j]) ++j;
  if(j==value.size()) {if(out->count==0) out->first=i; if(++out->count==2) return 2;}
 }
 return out->count==1?1:0;
}
// No work is performed under the Windows loader lock. The future game bootstrap
// must explicitly invoke validation before enabling any controlled hooks.
#if defined(STORMSLINGER_STANDALONE_PROXY)
extern "C" void StormslingerRecordProxyAttach();
#endif
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
extern "C" void StormslingerBeginPluginBootstrap();
static DWORD WINAPI StartPluginOutsideLoaderLock(void*) {
 StormslingerBeginPluginBootstrap();
 return 0;
}
#endif
BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
#if defined(STORMSLINGER_STANDALONE_PROXY)
 if(reason==DLL_PROCESS_ATTACH)StormslingerRecordProxyAttach();
#endif
#if defined(STORMSLINGER_COMPATIBLE_PLUGIN)
 // Stracker's loader calls LoadLibrary only. The worker cannot run until the
 // loader has finished DLL initialization; native setup stays outside DllMain.
 if(reason==DLL_PROCESS_ATTACH) {
  HANDLE worker=CreateThread(nullptr,0,StartPluginOutsideLoaderLock,nullptr,0,nullptr);
  if(!worker)return FALSE;
  CloseHandle(worker);
 }
#endif
 return TRUE;
}
// Resolve an x64 E8 rel32 call only inside the caller-provided image span.
// A rejected instruction never leaves a stale destination in the output.
extern "C" __declspec(dllexport) int StormslingerResolveCall(
 const unsigned char* image, size_t size, size_t callOffset,
 unsigned long long* destination) {
 if (!destination) return -1;
 *destination=0;
 if (!image || size<5 || callOffset>size-5) return -1;
 if(image[callOffset]!=0xE8) return -2;
 int32_t displacement=0;
 memcpy(&displacement,image+callOffset+1,sizeof(displacement));
 const size_t next=callOffset+5;
 size_t target=0;
 if(displacement<0) {
  const uint64_t backward=static_cast<uint64_t>(-static_cast<int64_t>(displacement));
  if(backward>next) return -3;
  target=next-static_cast<size_t>(backward);
 } else {
  const size_t forward=static_cast<size_t>(displacement);
  if(next>=size || forward>=size-next) return -3;
  target=next+forward;
 }
 if(target>=size) return -3;
 *destination=target;
 return 0;
}

