#pragma once
#include "horizon_notice_title_flow.h"
#include "horizon_owner_verification.h"
#include "../../../runtime/src/hooks.h"
#include "../../../runtime/src/runtime_log.h"
#include <optional>
#include <cstring>

namespace horizon_other {
namespace notice_detail {
#include "generated_notice_native_owners.inc"
template<class T> bool read(const void* at,T& value) noexcept {
    SIZE_T count=0;
    return ReadProcessMemory(GetCurrentProcess(),at,&value,sizeof(value),&count) && count==sizeof(value);
}
}
// Joins the complete-content transaction. Native GUI requests and static text
// keep DLL code alive through the normal process-lifetime runtime ownership.
class NoticeNativeSession {
    using Update=void(*)(void*);
    const std::uint8_t* image_=nullptr;
    Update original_=nullptr;
    stormslinger::HookTransaction* transaction_=nullptr;
    std::shared_ptr<NoticePages> pages_;
    std::optional<NoticeTitleFlow> flow_;
    bool active_=false,restored_=false,failed_=false,waiting_logged_=false;
    bool restore_pending_=false;
    unsigned language_=0;
    const char* labeled_body_=nullptr;
    void* button_font_node_=nullptr;
    std::uint32_t original_button_font_=0;
    struct PageLayout {
        void* body=nullptr;
        void* frame=nullptr;
        void* buttons=nullptr;
        std::array<float,4> frame_transform{}; // X,Y,scale X,scale Y
        std::array<float,2> body_position{},body_size{};
        float button_y=0;
        std::uint8_t layout=0;
        std::uint32_t control_point=0;
    } layout_;
    inline static NoticeNativeSession* owner_=nullptr;
    struct Submission {const std::uint8_t* image;void* gui;NoticeNativeSession* owner;};
    static void submit(void* context,const NoticeRequest& request) {
        const auto& binding=*static_cast<Submission*>(context);
        using Native=void(*)(void*,const NoticeRequest*);
        auto owned=request;
        const auto completion=owned.callback;
        auto* session=binding.owner;
        owned.callback=[session,completion](const int& selected,const int& result) {
            // The callback precedes the stock closing animation. Keep the
            // owned layout until the native queue is idle, or the fading
            // window visibly snaps back to the stock smaller layout.
            session->restore_pending_=true;
            completion(selected,result);
        };
        reinterpret_cast<Native>(const_cast<std::uint8_t*>(binding.image)+0x1ae2050)(binding.gui,&owned);
    }
    void restore_button_font() noexcept {
        if(button_font_node_) {
            using SetFont=void(*)(void*,std::uint32_t);
            reinterpret_cast<SetFont>(const_cast<std::uint8_t*>(image_)+0x2343240)(
                button_font_node_,original_button_font_);
            button_font_node_=nullptr;
        }
    }
    void scalar(void* node,std::uintptr_t setter,float value) noexcept {
        using Set=void(*)(void*,float);
        reinterpret_cast<Set>(const_cast<std::uint8_t*>(image_)+setter)(node,value);
    }
    void text_size(void* node,const std::array<float,2>& value) noexcept {
        using Set=void(*)(void*,const float*);
        reinterpret_cast<Set>(const_cast<std::uint8_t*>(image_)+0x2345170)(node,value.data());
    }
    void text_layout(void* node,std::uint8_t value) noexcept {
        using Set=void(*)(void*,std::uint8_t);
        reinterpret_cast<Set>(const_cast<std::uint8_t*>(image_)+0x2343bc0)(node,value);
    }
    void text_anchor(void* node,std::uint32_t value) noexcept {
        using Set=void(*)(void*,std::uint32_t);
        reinterpret_cast<Set>(const_cast<std::uint8_t*>(image_)+0x2342cd0)(node,value);
    }
    void restore_page_layout() noexcept {
        if(!layout_.body)return;
        scalar(layout_.frame,0x234f3c0,layout_.frame_transform[0]);
        scalar(layout_.frame,0x234f3e0,layout_.frame_transform[1]);
        scalar(layout_.frame,0x234f7d0,layout_.frame_transform[2]);
        scalar(layout_.frame,0x234f7f0,layout_.frame_transform[3]);
        scalar(layout_.body,0x23445f0,layout_.body_position[0]);
        scalar(layout_.body,0x2344610,layout_.body_position[1]);
        text_size(layout_.body,layout_.body_size);
        text_layout(layout_.body,layout_.layout);
        text_anchor(layout_.body,layout_.control_point);
        scalar(layout_.buttons,0x234f3e0,layout_.button_y);
        layout_={};
    }
    bool apply_page_layout(const std::uint8_t* message,void* body) noexcept {
        using notice_detail::read;
        if(!layout_.body) {
            using Find=void*(*)(const void*,const char*);
            // The native binding routine uses this same instance lookup. The
            // large frame is a_window_subG, NOT the small frame cached at2910.
            auto* frame=reinterpret_cast<Find>(const_cast<std::uint8_t*>(image_)+0x23537a0)(message,"a_window_subG");
            PageLayout saved;
            saved.body=body;saved.frame=frame;
            if(!frame || !read(message+0x2940,saved.buttons) || !saved.buttons)return false;
            const auto* f=static_cast<const std::uint8_t*>(frame);
            const auto* b=static_cast<const std::uint8_t*>(body);
            if(!read(f+0xb0,saved.frame_transform[0]) || !read(f+0xb4,saved.frame_transform[1]) ||
               !read(f+0xc0,saved.frame_transform[2]) || !read(f+0xc4,saved.frame_transform[3]) ||
               !read(b+0xa0,saved.body_position[0]) || !read(b+0xa4,saved.body_position[1]) ||
               !read(b+0xf8,saved.body_size) || !read(b+0x150,saved.layout) ||
               !read(b+0xc0,saved.control_point) ||
               !read(static_cast<const std::uint8_t*>(saved.buttons)+0xb4,saved.button_y))return false;
            saved.control_point&=15;
            layout_=saved;
        }
        // Author in the existing GUI's local logical units. Native viewport,
        // safe-area and parent animations still map this to every resolution.
        // Compact760x620 frame, original font size, separate button row.
        scalar(layout_.frame,0x234f3c0,-380.0f);
        scalar(layout_.frame,0x234f3e0,-310.0f);
        scalar(layout_.frame,0x234f7d0,layout_.frame_transform[2]*(760.0f/638.0f));
        scalar(layout_.frame,0x234f7f0,layout_.frame_transform[3]*(620.0f/500.0f));
        // cGUIObjMessage inherits the drawing node, NOT the instance transform:
        // its native PosX/Y are A0/A4 and ControlPoint is C0's low nibble.
        scalar(body,0x23445f0,-330.0f);
        scalar(body,0x2344610,-260.0f);
        text_size(body,{660.0f,460.0f});
        text_layout(body,0); // Native Layout: top-left, rather than17(center).
        text_anchor(body,0);
        scalar(layout_.buttons,0x234f3e0,250.0f);
        return true;
    }
    static void invoke(void* logo) {
        auto* self=owner_;
        try {if(self->active_ && !self->failed_ && self->hold(logo))return;}
        catch(const std::bad_alloc&) {
            self->failed_=true;
            stormslinger::runtime_log::notice(6,0,self->language_);
        }
        self->original_(logo);
    }
    static void completed(const NoticeCompletion& result) noexcept {
        stormslinger::runtime_log::notice(4,static_cast<unsigned>(owner_->pages_->page()+1),owner_->language_,result.confirmed,result.persisted);
    }
    void localize_owned_button(const std::uint8_t* message) {
        using notice_detail::read;
        const char* body=nullptr;
        std::uintptr_t callback=0;
        if(!message || !read(message+0x2968,body) || !body ||
           !read(message+0x29e0,callback) || (!callback && !restore_pending_))return;
        const auto language=notice_language_from_native_index(language_);
        bool owned=false;
        for(std::size_t page=0;page<6;++page)
            owned=owned || body==notice_page_text(language,page);
        if(!owned)return;
        std::uint8_t alternate=0;
        void* button_text=nullptr;
        const auto* label=notice_close_text(language);
        // Original1D8E120 selects R14 from these same two text slots and
        // assigns common-text CLOSE(53) through18C48A0. Only replace our own
        // static-body request; leave every ordinary dialog's labels untouched.
        if(!label || !read(message+0x2990,alternate) ||
           !read(message+(alternate ? 0x2920 : 0x28f8),button_text) || !button_text)return;
        // Both nodes are native cGUIObjMessage. The stock button uses Font0,
        // while this dialog's localized body uses Font4. Borrow the actual
        // working body font through the original Font setter, then restore
        // before another native dialog can reuse this button.
        void* body_text=nullptr;
        std::uint32_t body_font=0,button_font=0;
        if(!read(message+(alternate ? 0x2938 : 0x28f0),body_text) || !body_text ||
           !read(static_cast<const std::uint8_t*>(body_text)+0x14c,body_font) ||
           !read(static_cast<const std::uint8_t*>(button_text)+0x14c,button_font))return;
        if(!alternate || !apply_page_layout(message,body_text))return;
        if(body==labeled_body_)return;
        if(!button_font_node_) {
            button_font_node_=button_text;original_button_font_=button_font;
            using SetFont=void(*)(void*,std::uint32_t);
            reinterpret_cast<SetFont>(const_cast<std::uint8_t*>(image_)+0x2343240)(button_text,body_font);
        }
        using SetText=void(*)(void*,const char*);
        // Layout and Size setters only change properties. The original first
        // SetText already built centered glyph positions via2334E90 ->24DC290
        // ->24E0B30 ->24DF0C0. Rebuild once AFTER the owned properties are set;
        // moving the node alone leaves the old centered glyph cache intact.
        reinterpret_cast<SetText>(const_cast<std::uint8_t*>(image_)+0x18c48a0)(body_text,body);
        reinterpret_cast<SetText>(const_cast<std::uint8_t*>(image_)+0x18c48a0)(button_text,label);
        labeled_body_=body;
    }
    bool hold(void* logo) {
        using notice_detail::read;
        auto* object=static_cast<const std::uint8_t*>(logo);
        std::uintptr_t table=0;
        std::uint32_t stage=0,inner=0;
        if(!object || !read(object,table) || table!=reinterpret_cast<std::uintptr_t>(image_+0x2f0f3b0) ||
            !read(object+0x50,stage) || !read(object+0xa0,inner) || stage!=22 || inner!=2)return false;
        if(!pages_->needed() && !restore_pending_)return false;
        std::uint8_t* gui=nullptr;
        if(!read(image_+0x51c4640,gui) || !gui)return true;
        NoticeTitleSnapshot snapshot;
        snapshot.before_title_request=true;
        if(!read(gui+0x4c,snapshot.native_language))return true;
        language_=snapshot.native_language;
        if(notice_language_from_native_index(language_).empty()) {
            failed_=true;stormslinger::runtime_log::notice(5,0,language_);return false;
        }
        if(!read(image_+0x3edb520,snapshot.defaults.logical_width) ||
           !read(image_+0x3edb524,snapshot.defaults.logical_height) ||
           !read(image_+0x503c430,snapshot.defaults.parameter20) ||
           !read(image_+0x503c434,snapshot.defaults.parameter24) ||
           !read(image_+0x503c438,snapshot.defaults.parameter28))return true;
        std::uint8_t* message=nullptr;
        std::uint64_t queued=0,ring=0;
        std::uintptr_t callback=0;
        std::uint32_t flags=0;std::uint8_t loaded=0;
        if(read(gui+0x13d60,message) && message)localize_owned_button(message);
        snapshot.message_queue_ready=read(gui+0x13d60,message) && message &&
            read(message+0xc,loaded) && (loaded&0xe) &&
            read(gui+0x8f60,queued) && queued==0 &&
            read(message+0x2c38,ring) && ring==0 &&
            read(message+0x29e0,callback) && callback==0 &&
            read(message+0x2298,flags) && !(flags&0x80000011u);
        if(restore_pending_) {
            // Finish even the final page's closing animation before releasing
            // the held title flow. Restore before submitting any next request.
            if(!snapshot.message_queue_ready)return true;
            restore_button_font();restore_page_layout();restore_pending_=false;
        }
        Submission binding{image_,gui,this};
        if(!snapshot.message_queue_ready && !waiting_logged_) {
            waiting_logged_=true;stormslinger::runtime_log::notice(2,static_cast<unsigned>(pages_->page()+1),language_);
        }
        const bool submitting=snapshot.message_queue_ready && pages_->ready();
        const bool held=flow_->hold_title(snapshot,&submit,&binding);
        if(submitting)stormslinger::runtime_log::notice(3,static_cast<unsigned>(pages_->page()+1),language_);
        if(held) {
            // Preserve the original aLogo update's unconditional GUI flag.
            const std::uint8_t one=1;SIZE_T count=0;
            WriteProcessMemory(GetCurrentProcess(),gui+0x14788,&one,1,&count);
        }
        return held;
    }
public:
    bool prepare_while_quiescent(const std::uint8_t* image,stormslinger::HookTransaction& transaction,
                                const std::wstring& supplied_directory={}) {
        if(image_ || owner_ || !image || !transaction.can_add_hooks())return false;
        for(const auto& owner:notice_detail::kNoticeNativeOwners)
            if(!verify_native_owner(image,owner))return false;
        // The production caller uses LocalAppData; an explicit host-owned
        // directory also permits isolated fixtures without touching profiles.
        std::wstring directory=supplied_directory;
        if(directory.empty()) {
            std::array<wchar_t,32768> app{};
            const auto length=GetEnvironmentVariableW(L"LOCALAPPDATA",app.data(),static_cast<DWORD>(app.size()));
            if(!length || length>=app.size())return false;
            directory=app.data();directory+=L"\\Stormslinger";
        }
        if(!CreateDirectoryW(directory.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)return false;
        auto acknowledgement=std::make_shared<NoticeAcknowledgement>(directory+L"\\horizon-content.version","horizon-complete-20261022-c10");
        pages_=std::make_shared<NoticePages>(std::move(acknowledgement),&completed);
        flow_.emplace(pages_);image_=image;transaction_=&transaction;owner_=this;
        return transaction.add(const_cast<std::uint8_t*>(image)+0x24c740,
            reinterpret_cast<void*>(&invoke),reinterpret_cast<void**>(&original_))==MH_OK;
    }
    bool activate_after_shared_commit() noexcept {
        if(owner_!=this || !transaction_ || !transaction_->last.active)return false;
        active_=true;stormslinger::runtime_log::notice(1,0,0);return true;
    }
    bool restore_while_quiescent() noexcept {
        if(!image_)return true;
        // One reference here and one in flow; additional references belong to
        // deferred native callbacks that must be disposed before unloading.
        if(restore_pending_ || (pages_ && pages_.use_count()>2))return false;
        restore_button_font();
        restore_page_layout();
        active_=false;restored_=true;return true;
    }
    bool release_after_shared_stop() noexcept {
        if(!image_)return true;
        if(!restored_ || transaction_->has_library_state() || owner_!=this)return false;
        owner_=nullptr;flow_.reset();pages_.reset();return true;
    }
};
} // namespace horizon_other
