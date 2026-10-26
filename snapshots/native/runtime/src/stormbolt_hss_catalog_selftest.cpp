#ifdef NDEBUG
#undef NDEBUG
#endif
#include "stormbolt_hss_catalog.h"
#include <array>
#include <cassert>
#include <cstdio>

namespace {
struct Row {void* type;const char* path;std::uint64_t flags;};
std::array<Row,2> original{{{reinterpret_cast<void*>(1),"first",5},
    {reinterpret_cast<void*>(2),"second",8}}};
Row foreign{reinterpret_cast<void*>(4),"foreign",0};
void* hss=reinterpret_cast<void*>(3);
bool hooked=true,nest=false,change_count=false,change_pointer=false;
unsigned calls{};
using Create=void(*)(void*,unsigned,void*);
Create create_entry{};
void inspect(void* manager,unsigned category) {
    ++calls;Row* rows{};unsigned count{};
    auto* bytes=static_cast<unsigned char*>(manager);
    std::memcpy(&rows,bytes+0x14FE0,8);std::memcpy(&count,bytes+0x14FE8,4);
    const bool extended=hooked && category==3;
    assert(count==(extended?3u:2u));
    assert(std::memcmp(rows,original.data(),sizeof(original))==0);
    if(extended){assert(rows[2].type==hss && rows[2].flags==0);
        assert(std::strcmp(rows[2].path,"ui\\hud\\hud_hss")==0);}
    if(change_count){const unsigned changed=19;std::memcpy(bytes+0x14FE8,&changed,4);}
    if(change_pointer){auto* replacement=&foreign;const unsigned changed=1;
        std::memcpy(bytes+0x14FE0,&replacement,8);std::memcpy(bytes+0x14FE8,&changed,4);}
}
void preload(void* manager,unsigned resource_group,unsigned category) {
    assert(resource_group==41);inspect(manager,category);
    if(nest){nest=false;create_entry(manager,category,reinterpret_cast<void*>(0x1234));}
}
void create(void* manager,unsigned category,void* output) {
    assert(output==reinterpret_cast<void*>(0x1234));inspect(manager,category);
}
void remove_fixture(void* manager,unsigned category){inspect(manager,category);}
void stub(unsigned char* address,void* target){
    std::memset(address,0x90,8);address[8]=0x48;address[9]=0xB8;
    std::memcpy(address+10,&target,8);address[18]=0xFF;address[19]=0xE0;
}
}
void hss_catalog_selftest(){
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1AF0000,
        MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(image);
    stub(image+0x1AD8D20,reinterpret_cast<void*>(&preload));
    stub(image+0x1AD6230,reinterpret_cast<void*>(&create));
    stub(image+0x1AE00F0,reinterpret_cast<void*>(&remove_fixture));
    DWORD old{};assert(VirtualProtect(image,0x1AF0000,PAGE_EXECUTE_READ,&old));
    assert(FlushInstructionCache(GetCurrentProcess(),image,0x1AF0000));
    auto load_entry=reinterpret_cast<void(*)(void*,unsigned,unsigned)>(image+0x1AD8D20);
    create_entry=reinterpret_cast<Create>(image+0x1AD6230);
    auto remove_entry=reinterpret_cast<void(*)(void*,unsigned)>(image+0x1AE00F0);
    std::vector<unsigned char> manager(0x15000);
    auto reset=[&]{auto* p=original.data();const unsigned count=2;
        std::memcpy(manager.data()+0x14FE0,&p,8);std::memcpy(manager.data()+0x14FE8,&count,4);};
    auto restored=[&]{Row* p{};unsigned count{};std::memcpy(&p,manager.data()+0x14FE0,8);
        std::memcpy(&count,manager.data()+0x14FE8,4);assert(p==original.data() && count==2);};
    reset();stormslinger::HookTransaction transaction;stormslinger::StormboltHssCatalog catalog;
    assert(transaction.begin()==MH_OK);
    assert(catalog.prepare(transaction,reinterpret_cast<std::uintptr_t>(image),hss)==MH_OK);
    assert(transaction.commit()==MH_OK);
    for(unsigned category:{2u,3u}){
        load_entry(manager.data(),41,category);restored();
        create_entry(manager.data(),category,reinterpret_cast<void*>(0x1234));restored();
        remove_entry(manager.data(),category);restored();
    }
    nest=true;load_entry(manager.data(),41,3);restored();
    change_count=true;remove_entry(manager.data(),3);change_count=false;restored();
    change_pointer=true;remove_entry(manager.data(),3);change_pointer=false;
    Row* p{};std::memcpy(&p,manager.data()+0x14FE0,8);assert(p==&foreign);reset();
    assert(catalog.failures()==2);
    catalog.withdraw();hooked=false;
    load_entry(manager.data(),41,3);restored();
    create_entry(manager.data(),3,reinterpret_cast<void*>(0x1234));restored();
    assert(transaction.stop()==MH_OK);hooked=false;
    load_entry(manager.data(),41,3);restored();
    create_entry(manager.data(),3,reinterpret_cast<void*>(0x1234));restored();
    remove_entry(manager.data(),3);restored();
    assert(VirtualFree(image,0,MEM_RELEASE));
    std::printf("PASS:HSS catalog real hooks/trampolines, forwarding, nesting, ownership conflicts and stop (%u calls); fixture manager only\n",calls);
}
