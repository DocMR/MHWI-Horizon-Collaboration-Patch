#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstring>
#include <vector>
#include "quest_signatures.h"
#pragma comment(lib,"bcrypt.lib")
struct Match { unsigned count; unsigned long long first; };
extern "C" int StormslingerScan(const unsigned char*,size_t,const char*,Match*);
extern "C" int StormslingerResolveCall(const unsigned char*,size_t,size_t,unsigned long long*);
struct QuestPlan { unsigned version; unsigned count; unsigned long long rva[12]; };
extern "C" __declspec(dllexport) int StormslingerBuildQuestPlan(
 const unsigned char* text,size_t length,unsigned long long textRva,QuestPlan* output) {
 if(!output) return -1;
 memset(output,0,sizeof(*output));
 if(!text || !length || textRva>0xffffffffULL || length>0xffffffffULL-textRva) return -1;
 QuestPlan candidate{};candidate.version=1;candidate.count=12;
 for(size_t i=0;i<8;++i) {
  Match hit{};const auto& sig=stormslinger::questSignatures[i];
  int status=StormslingerScan(text,length,sig.bits,&hit);
  if(status!=1) return 100+static_cast<int>(i);
  const auto rva=textRva+hit.first;
  if(rva!=sig.rva) return 200+static_cast<int>(i);
  candidate.rva[i]=rva;
 }
 const unsigned owner[]={4,4,5,5};
 const unsigned offset[]={0x4e1,0x6d2,0x5f,0x81};
 const unsigned expected[]={0x134ef50,0x134efe0,0x1355c50,0x1355c40};
 for(size_t i=0;i<4;++i) {
  auto address=candidate.rva[owner[i]]+offset[i];
  if(address<textRva || address-textRva>=length) return 300+static_cast<int>(i);
  unsigned long long destination=0;
  if(StormslingerResolveCall(text,length,static_cast<size_t>(address-textRva),&destination)!=0) return 300+static_cast<int>(i);
  if(textRva+destination!=expected[i]) return 400+static_cast<int>(i);
  candidate.rva[8+i]=textRva+destination;
 }
 *output=candidate;return 0;
}
// Exact executable hashing, deliberately separate from live-code validation.
// Both gates are required by the future bootstrap; neither enables hooks.
extern "C" __declspec(dllexport) int StormslingerCheckExecutable(const wchar_t* path,char* hexHash,size_t capacity) {
 if(!path || !hexHash || capacity<65) return -1;
 hexHash[0]=0;
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(file==INVALID_HANDLE_VALUE) return -2;
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 int result=-3;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0) {
  if(BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0) {
   std::array<unsigned char,65536> buffer;DWORD read=0;bool ok=true;
   for(;;) {
    if(!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&read,nullptr)){ok=false;break;}
    if(!read) break;
    if(BCryptHashData(hash,buffer.data(),read,0)<0){ok=false;break;}
   }
   std::array<unsigned char,32> digest{};
   if(ok && BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0) {
    const char* digits="0123456789abcdef";
    for(size_t i=0;i<digest.size();++i){hexHash[i*2]=digits[digest[i]>>4];hexHash[i*2+1]=digits[digest[i]&15];}
    hexHash[64]=0;
    result=strcmp(hexHash,"c2ebbbd2c49f216d484e31a5219bed419eb1e5e7d206d02cba040a3ab79d90ea")==0?0:1;
   }
   BCryptDestroyHash(hash);
  }
  BCryptCloseAlgorithmProvider(algorithm,0);
 }
 CloseHandle(file);return result;
}
