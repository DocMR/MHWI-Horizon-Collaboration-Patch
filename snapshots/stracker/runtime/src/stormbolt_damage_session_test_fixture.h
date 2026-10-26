#pragma once
// Native-shaped endpoints for shared transaction selftests only.
namespace damage_session_fixture {
inline std::array<std::byte,0x290> component{};
inline std::array<std::byte,0xDB8> state_object{};
__declspec(noinline) inline void* construct(void* object) {
    const std::uintptr_t vt=55;std::memcpy(object,&vt,sizeof(vt));return object;
}
__declspec(noinline) inline void* destroy(void* object,std::uint32_t) {
    const std::uintptr_t vt=0;std::memcpy(object,&vt,sizeof(vt));return object;
}
__declspec(noinline) inline void update(void* object) {
    static_cast<std::byte*>(object)[0x70]=std::byte{0};
}
__declspec(noinline) inline float absorb(void*,float input) {return input*0.25f;}
inline void configure(stormslinger::StormboltNativeBindings& native) {
    native.damage_construct=reinterpret_cast<std::uintptr_t>(&construct);
    native.damage_destroy=reinterpret_cast<std::uintptr_t>(&destroy);
    native.damage_update=reinterpret_cast<std::uintptr_t>(&update);
    native.damage_absorb=reinterpret_cast<std::uintptr_t>(&absorb);
    native.damage_vtable=55;
}
} // namespace damage_session_fixture
