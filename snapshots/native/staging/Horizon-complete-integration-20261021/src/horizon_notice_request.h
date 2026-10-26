#pragma once
#include "horizon_notice_callback.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace horizon_other {
// Layout consumed by original1AE2050/1ACF140. Text storage must outlive every
// copied native request. No title field is invented in this native descriptor.
struct NoticeRequest {
    std::int32_t kind=1;
    std::uint32_t padding04=0;
    const char* text=nullptr;
    std::int32_t x=0,y=0;
    std::array<std::uint8_t,8> padding18{};
    float parameter20=0,parameter24=0,parameter28=0,parameter2c=0;
    std::uint8_t flag30=0,flag31=0;
    std::uint16_t padding32=0;
    std::uint32_t flags34=0,sound38=0,parameter3c=0,parameter40=0,padding44=0;
    NoticeNativeFunction callback;
    std::uint8_t flag88=0;
    std::array<std::uint8_t,7> padding89{};
};
static_assert(sizeof(NoticeRequest)==0x90);
static_assert(offsetof(NoticeRequest,text)==8 && offsetof(NoticeRequest,x)==0x10);
static_assert(offsetof(NoticeRequest,parameter20)==0x20 && offsetof(NoticeRequest,flag30)==0x30);
static_assert(offsetof(NoticeRequest,sound38)==0x38 && offsetof(NoticeRequest,callback)==0x48);
static_assert(offsetof(NoticeRequest,flag88)==0x88);

// Snapshot the original runtime defaults at503C430/434/438 and logical
// dimensions3EDB520/524 on the native GUI thread. Do not hardcode1920x1080.
struct NoticeRequestDefaults {
    std::int32_t logical_width=0,logical_height=0;
    float parameter20=0,parameter24=0,parameter28=0;
};
inline NoticeRequest make_notice_request(const NoticeRequestDefaults& defaults,
    const char* process_lifetime_text,NoticeNativeFunction callback,
    std::uint32_t sound=0) {
    NoticeRequest request;
    request.text=process_lifetime_text;
    // Native selectWindow null_windowG: body580x380, stock large background
    // and button row. Original1D8E120 selects this through request+30.
    request.flag30=1;
    request.x=defaults.logical_width/2;request.y=defaults.logical_height/2;
    request.parameter20=defaults.parameter20;
    request.parameter24=defaults.parameter24;
    request.parameter28=defaults.parameter28;
    request.sound38=sound;request.callback=std::move(callback);
    return request;
}
} // namespace horizon_other
