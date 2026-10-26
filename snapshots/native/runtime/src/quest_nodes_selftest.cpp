#include "quest_bridge.cpp"
#include <cstdio>
#include <cstdlib>
extern "C" int StormslingerCheckExecutable(const wchar_t*,char*,size_t){return -1;}
extern "C" int StormslingerBuildQuestPlan(const unsigned char*,size_t,unsigned long long,QuestPlan*){return -1;}
namespace {
int allocations=0,destroys=0,failAt=0;
void* mockDestroy(void* node,unsigned flags){if(flags!=1)std::abort();++destroys;std::free(node);return nullptr;}
void* table[]={reinterpret_cast<void*>(mockDestroy)};
void* mockAllocate(size_t n,unsigned alignment){if(n!=16 || alignment!=16)std::abort();++allocations;return allocations==failAt?nullptr:std::malloc(n);}
QuestNo* mockConstruct(void* memory){auto node=static_cast<QuestNo*>(memory);node->vtable=table;node->id=0;node->padding=0;return node;}
bool mockLoad(void*,void*){return true;}
void mockGrow(void*,uint32_t){}
}
int main(){
 alignas(16) unsigned char resource[0xd0]{};void* entries[4]{};
 std::strcpy(reinterpret_cast<char*>(resource)+0xc,"quest\\questNoList");
 *reinterpret_cast<int*>(resource+0xbc)=4;
 *reinterpret_cast<void***>(resource+0xc8)=entries;
 listOriginal=mockLoad;grow=mockGrow;allocateNode=mockAllocate;constructNode=mockConstruct;
 failAt=2;
 if(!listHook(resource,nullptr) || allocations!=2 || destroys!=1 || *reinterpret_cast<int*>(resource+0xb8)!=0)return 1;
 failAt=0;allocations=0;destroys=0;
 if(!listHook(resource,nullptr) || allocations!=2 || destroys!=0 || *reinterpret_cast<int*>(resource+0xb8)!=2)return 2;
 if(static_cast<QuestNo*>(entries[0])->id!=ids[0] || static_cast<QuestNo*>(entries[1])->id!=ids[1])return 3;
 listHook(resource,nullptr);if(allocations!=2 || *reinterpret_cast<int*>(resource+0xb8)!=2)return 4;
 for(auto node:entries)if(node)destroyNode(static_cast<QuestNo*>(node));
 if(destroys!=2)return 5;
 std::puts("PASS: second allocation failure rollback, native node layout, duplicate reload, destructor slot 0 dispatch (mock allocator; game lifetime unverified)");
}
