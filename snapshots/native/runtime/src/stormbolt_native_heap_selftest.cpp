#ifdef NDEBUG
#undef NDEBUG
#endif
#include <Windows.h>
#include "generated_source_command_fixture.h"
#include "generated_default_heap_fixture.h"
#include "stormbolt_default_heap_readiness.h"
#include "stormbolt_shoot_gatling_condition.h"
#include <cassert>
#include <cstdio>
using namespace stormslinger;
namespace {
struct Heap {void** table;unsigned index;};
alignas(16) std::array<unsigned char,0x48> storage{};
void* expected_heap=nullptr;
const void* expected_label=nullptr;
std::uint32_t expected_tag=0;
bool fail_allocate=false;
unsigned allocations=0,frees=0;
int (*retired_predicate)(void*)=nullptr;
void* allocate(void* heap,std::size_t bytes,std::uint32_t alignment,std::uint32_t tag,
    std::uint32_t a,std::uint32_t b,const void* label) {
    assert(heap==expected_heap && bytes==0x28 && alignment==0x10 && tag==expected_tag && !a && !b && label==expected_label);
    ++allocations;return fail_allocate?nullptr:storage.data()+16;
}
void* free_native(void* heap,void* object) {
    assert(heap==expected_heap && object==storage.data()+16);
    if(retired_predicate) assert(retired_predicate(object)==-1);
    ++frees;return reinterpret_cast<void*>(0x1234); // Native destructor must return object.
}
template<class T> void put(void* p,std::size_t offset,T value) {std::memcpy(static_cast<unsigned char*>(p)+offset,&value,sizeof(value));}
}
int main() {
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,command_fixture_image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(image);
    for(const auto& span:command_fixture_spans) std::memcpy(image+span.rva,span.data,span.size);
    for(const auto& span:default_heap_spans) std::memcpy(image+span.rva,span.data,span.size);
    for(const auto target:default_heap_error_targets) image[target]=0xCC;
    for(unsigned n=0;n<9;++n) put(image,0x31C0AA0+n*8,image+command_fixture_base_vtable[n]);
    for(const auto& span:command_fixture_spans) {DWORD old{};assert(VirtualProtect(image+span.rva,span.size,PAGE_EXECUTE_READ,&old));}
    for(const auto& span:default_heap_spans) {DWORD old{};assert(VirtualProtect(image+span.rva,span.size,PAGE_EXECUTE_READ,&old));}
    for(const auto target:default_heap_error_targets) {DWORD old{};assert(VirtualProtect(image+target,1,PAGE_EXECUTE_READ,&old));}
    assert(FlushInstructionCache(GetCurrentProcess(),image,command_fixture_image_size));
    auto create=reinterpret_cast<void*(*)(void*)>(image+0x11CB370);
    auto destroy=reinterpret_cast<void*(*)(void*,std::uint32_t)>(image+0xEF3CA0);
    std::array<void*,14> heap_table{};
    heap_table[7]=reinterpret_cast<void*>(&allocate);heap_table[13]=reinterpret_cast<void*>(&free_native);
    std::array<Heap,64> heaps{};
    auto* type=image+0x4F9F240;
    unsigned cases=0;
    for(unsigned index=0;index<64;++index) {
        heaps[index]={heap_table.data(),index};put(image,0x50EC920+index*8,&heaps[index]);
    }
    for(unsigned index=0;index<64;++index) {
        expected_heap=&heaps[index];expected_label=&heaps[63-index];expected_tag=0xA000+index;
        put(type,0x30,std::uint32_t{(index<<23)|0x12345});put(type,0x34,expected_tag);put(type,8,expected_label);
        for(std::uint32_t flags:{0u,1u,3u,4u,5u,7u,8u}) {
            storage.fill(0xA5);auto* object=create(type);assert(object==storage.data()+16);
            assert(stormbolt_source_field<void*>(object,0)==image+0x31C0AA0);
            assert(!stormbolt_source_field<void*>(object,0x10) && !stormbolt_source_field<void*>(object,0x18) && !stormbolt_source_field<void*>(object,0x20));
            const auto prior=frees;
            assert(destroy(object,flags)==object);
            assert(frees==prior+((flags&1) && !(flags&4)?1u:0u));
            assert(stormbolt_source_field<void*>(object,0)==image+0x2E4F640);
            for(unsigned n=0;n<16;++n) assert(storage[n]==0xA5 && storage[0x38+n]==0xA5);
            ++cases;
        }
        fail_allocate=true;storage.fill(0xA5);assert(!create(type));fail_allocate=false;
        for(auto b:storage) assert(b==0xA5);
        ++cases;
    }
    // Use the production object controller with actual native factory, context,
    // destructor and heap selector; only allocator virtual38/68 are fixture IO.
    StormboltInstanceStore<1> store;
    std::array<unsigned char,0x1500> actor{};
    std::array<unsigned char,0x80> controller{};
    put(actor.data(),0x14F8,controller.data());
    StormboltShootConditionBindings api{type,create,reinterpret_cast<void(*)(void*,void*)>(image+0x11CB480),destroy,
        reinterpret_cast<void**>(image+0x31C0AA0),1,2,+[](void*,void*,void*) {return false;},nullptr,
        {reinterpret_cast<bool(*)(void*,std::uint32_t)>(image+0x122FD90),reinterpret_cast<bool(*)(void*,std::uint32_t)>(image+0x1230070),3}};
    StormboltShootGatlingCondition<1> condition(store);
    assert(condition.create(actor.data(),api));
    auto** table=stormbolt_source_field<void**>(condition.object(),0);
    retired_predicate=reinterpret_cast<int(*)(void*)>(table[6]);
    auto* object=condition.object();const auto prior=frees;
    assert(reinterpret_cast<void*(*)(void*,std::uint32_t)>(table[0])(object,1)==object);
    assert(!condition.object() && frees==prior+1);
    retired_predicate=nullptr;
    // Execute original default allocator virtuals/backends against a real,
    // isolated Win32 heap. No allocator virtual method is replaced in this phase.
    auto private_heap=HeapCreate(0,0,0);assert(private_heap);
    put(image,0x5215038,private_heap);
    put(image,0x2E13000,&EnterCriticalSection);put(image,0x2E13008,&LeaveCriticalSection);
    put(image,0x2E13348,&HeapAlloc);put(image,0x2E13350,&HeapFree);put(image,0x2E13158,&GetLastError);
    put(image,0x34DF340+0x38,image+0x21F1CC0);put(image,0x34DF340+0x68,image+0x21F1DA0);
    alignas(16) std::array<unsigned char,0xA0> native_heap{};
    put(native_heap.data(),0,image+0x34DF340);put(image,0x50EC920,native_heap.data());
    put(type,0x30,std::uint32_t{0});put(image,0x4F7E7D0+0x30,std::uint32_t{0});
    auto* critical=reinterpret_cast<CRITICAL_SECTION*>(native_heap.data()+0x60);
    InitializeCriticalSection(critical);
    const auto base_address=reinterpret_cast<std::uintptr_t>(image);
    StormboltDefaultHeapBindings heap_api{base_address+0x50EC920,base_address+0x34DF340,
        base_address+0x21F1CC0,base_address+0x21F1DA0};
    StormboltDefaultHeapSelection selected{reinterpret_cast<void*>(1),999};
    for(unsigned index=0;index<64;++index) {
        put(image,0x50EC920+index*8,native_heap.data());
        put(type,0x30,std::uint32_t{(index<<23)|0x807FFFFF});
        assert(stormbolt_select_default_heap(heap_api,type,selected));
        assert(selected.heap==native_heap.data() && selected.index==index);
    }
    put(type,0x30,std::uint32_t{0});
    auto expect_rejected=[&] {
        selected={reinterpret_cast<void*>(1),999};
        assert(!stormbolt_select_default_heap(heap_api,type,selected));
        assert(selected.heap==reinterpret_cast<void*>(1) && selected.index==999);
    };
    put(image,0x50EC920,static_cast<void*>(nullptr));expect_rejected();
    put(image,0x50EC920,native_heap.data());
    put(native_heap.data(),0,heap_table.data());expect_rejected();
    put(native_heap.data(),0,image+0x34DF340);
    put(image,0x34DF340+0x38,image+0x21F1CC1);expect_rejected();
    put(image,0x34DF340+0x38,image+0x21F1CC0);
    put(image,0x34DF340+0x68,image+0x21F1DA1);expect_rejected();
    put(image,0x34DF340+0x68,image+0x21F1DA0);
    assert(!stormbolt_select_default_heap(heap_api,nullptr,selected));
    std::puts("PASS default-heap selection: all64 DTI indices, null/foreign heap and changed allocate/free rejection; output preserved on failure.");
    auto create_setbomb=reinterpret_cast<void*(*)(void*)>(image+0xF03CA0);
    auto destroy_setbomb=reinterpret_cast<void*(*)(void*,std::uint32_t)>(image+0xF0AE50);
    unsigned native_round_trips=0;
    for(unsigned flags:{0u,2u,4u,6u}) for(unsigned global:{0u,1u}) {
        put(native_heap.data(),0x5A,static_cast<std::uint16_t>(flags));image[0x5123358]=static_cast<unsigned char>(global);
        assert(!stormbolt_allocate_default_heap(heap_api,type,nullptr,0x38,0x10,0,0,0,nullptr));
        auto* resolved=stormbolt_allocate_default_heap(heap_api,type,native_heap.data(),0x38,0x10,0,0,0,nullptr);
        assert(resolved && (reinterpret_cast<std::uintptr_t>(resolved)&15)==0);
        assert(stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0x38);
        reinterpret_cast<void(*)(void*,void*)>(heap_api.release)(native_heap.data(),resolved);
        assert(stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0 && HeapValidate(private_heap,0,nullptr));
        ++native_round_trips;
        auto* base=create(type);assert(base && (reinterpret_cast<std::uintptr_t>(base)&15)==0);
        assert(stormbolt_source_field<std::uint64_t>(static_cast<unsigned char*>(base)-8,0)==0x28);
        assert(stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0x28);
        assert(destroy(base,1)==base && stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0);
        ++native_round_trips;
        auto* bomb=create_setbomb(image+0x4F7E7D0);assert(bomb && (reinterpret_cast<std::uintptr_t>(bomb)&15)==0);
        assert(stormbolt_source_field<std::uint32_t>(bomb,0x28)==0x20 && stormbolt_source_field<std::uint32_t>(bomb,0x2C)==0xFFFF);
        assert(stormbolt_source_field<std::uint64_t>(static_cast<unsigned char*>(bomb)-8,0)==0x38);
        assert(stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0x38);
        assert(destroy_setbomb(bomb,1)==bomb && stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0);
        ++native_round_trips;
        StormboltShootGatlingCondition<1> native_condition(store);
        assert(native_condition.create(actor.data(),api));native_condition.destroy_after_detach();
        assert(stormbolt_source_field<std::uint64_t>(native_heap.data(),8)==0 && stormbolt_source_field<std::uint64_t>(native_heap.data(),0x18)==0);
        assert(HeapValidate(private_heap,0,nullptr));++native_round_trips;
    }
    DeleteCriticalSection(critical);assert(HeapDestroy(private_heap));
    assert(VirtualFree(image,0,MEM_RELEASE));
    std::printf("PASS%u native heap-selector/factory/destructor cases, all64 heaps and flag routes; production owned object native create/context/destroy. Heap virtual methods are fixture IO.\n",cases);
    std::printf("PASS%u full native default-heap allocation/free round trips using private Win32 heap, actual locks/alignment/headers/accounting; base28/SetBomb38/owned condition.\n",native_round_trips);
}
