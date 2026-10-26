#pragma once
#include <windows.h>
#include <array>
#include <cstring>
#include <string>
#include <string_view>

namespace horizon_other {
struct NoticeCompletion {
    bool consumed=false;
    bool confirmed=false;
    bool persisted=false;
};

// A single notice/content-version on the serialized native GUI thread.
// The host supplies its owned local-appdata file, never a character-save path.
// This helper does not select a hub, publish a callback or display any text.
class NoticeAcknowledgement {
    std::wstring path_,temporary_;
    std::string record_;
    bool valid_=false,read_ok_=true,seen_=false,offered_=false,pending_=false;
    bool persist() const noexcept {
        HANDLE file=CreateFileW(temporary_.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return false;
        DWORD written=0;
        const bool ok=WriteFile(file,record_.data(),static_cast<DWORD>(record_.size()),&written,nullptr) && written==record_.size();
        CloseHandle(file);
        if(ok && MoveFileExW(temporary_.c_str(),path_.c_str(),MOVEFILE_REPLACE_EXISTING))return true;
        DeleteFileW(temporary_.c_str());
        return false;
    }
public:
    NoticeAcknowledgement(std::wstring path,std::string_view content_version)
        :path_(std::move(path)),temporary_(path_+L".tmp") {
        valid_=!path_.empty() && !content_version.empty() && content_version.size()<128 &&
            content_version.find_first_of("\r\n") == std::string_view::npos &&
            content_version.find('\0') == std::string_view::npos;
        if(!valid_){read_ok_=false;return;}
        record_=std::string(content_version)+"\n";
        HANDLE file=CreateFileW(path_.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE) {
            const auto error=GetLastError();
            read_ok_=error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND;
            return;
        }
        std::array<char,130> bytes{};DWORD count=0;
        read_ok_=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&count,nullptr)!=FALSE;
        CloseHandle(file);
        seen_=read_ok_ && count==record_.size() && !std::memcmp(bytes.data(),record_.data(),record_.size());
    }
    bool read_ok() const noexcept {return read_ok_;}
    bool seen() const noexcept {return seen_;}
    bool pending() const noexcept {return pending_;}
    bool should_offer() const noexcept {return valid_ && !seen_ && !offered_;}
    bool begin_request() noexcept {
        if(!should_offer())return false;
        offered_=pending_=true;return true;
    }
    // Only when the host knows no native request/callback was retained.
    bool submission_failed() noexcept {
        if(!pending_)return false;
        offered_=pending_=false;return true;
    }
    NoticeCompletion complete(int selected,int result) noexcept {
        if(!pending_)return {};
        pending_=false;
        // The original primary handler's normal result; cancellation/forced
        // close leave the disk record alone and do not re-open this session.
        const bool confirmed=selected==0 && result==0;
        if(!confirmed)return {true,false,false};
        seen_=true;
        return {true,true,persist()};
    }
};
} // namespace horizon_other
