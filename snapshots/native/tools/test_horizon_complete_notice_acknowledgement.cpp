#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../staging/Horizon-complete-integration-20261021/src/horizon_notice_acknowledgement.h"
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<"\n";return 1;}} while(0)
namespace fs=std::filesystem;
std::string read(const fs::path& path) {
    std::ifstream file(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
int main() {
    using horizon_other::NoticeAcknowledgement;
    const fs::path root=L"research/horizon-complete-notice-ack-fixture-20261021";
    CHECK(!fs::exists(root));CHECK(fs::create_directory(root));
    const auto record=root/L"content-notice.version";
    {
        NoticeAcknowledgement state(record.wstring(),"content-v1");
        CHECK(state.read_ok() && state.should_offer() && !state.seen());
        CHECK(!state.complete(0,0).consumed && !fs::exists(record));
        CHECK(state.begin_request() && state.pending() && !state.begin_request());
        CHECK(!fs::exists(record));
        const auto closed=state.complete(0,2);
        CHECK(closed.consumed && !closed.confirmed && !state.should_offer() && !fs::exists(record));
    }
    {
        NoticeAcknowledgement state(record.wstring(),"content-v1");CHECK(state.should_offer());
        CHECK(state.begin_request());CHECK(!state.complete(1,0).confirmed);CHECK(!fs::exists(record));
    }
    {
        NoticeAcknowledgement state(record.wstring(),"content-v1");CHECK(state.begin_request());
        CHECK(state.submission_failed() && state.should_offer() && !fs::exists(record));
        CHECK(state.begin_request());const auto confirmed=state.complete(0,0);
        CHECK(confirmed.consumed && confirmed.confirmed && confirmed.persisted);
        CHECK(state.seen() && !state.pending() && !state.should_offer());
        CHECK(read(record)=="content-v1\n");CHECK(!fs::exists(record.wstring()+L".tmp"));
    }
    {
        NoticeAcknowledgement same(record.wstring(),"content-v1");CHECK(same.seen() && !same.should_offer());
        NoticeAcknowledgement next(record.wstring(),"content-v2");CHECK(!next.seen() && next.begin_request());
        CHECK(!next.complete(0,1).confirmed && read(record)=="content-v1\n");
    }
    {
        const auto absent=root/L"absent-parent/record";
        NoticeAcknowledgement state(absent.wstring(),"content-v1");CHECK(state.begin_request());
        const auto confirmed=state.complete(0,0);CHECK(confirmed.confirmed && !confirmed.persisted);
        CHECK(state.seen() && !state.should_offer() && !fs::exists(absent.parent_path()));
    }
    {
        HANDLE held=CreateFileW(record.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        CHECK(held!=INVALID_HANDLE_VALUE);
        NoticeAcknowledgement next(record.wstring(),"content-v2");CHECK(next.begin_request());
        const auto confirmed=next.complete(0,0);CHECK(confirmed.confirmed && !confirmed.persisted);
        CHECK(read(record)=="content-v1\n" && !fs::exists(record.wstring()+L".tmp"));
        CloseHandle(held);
    }
    {
        NoticeAcknowledgement next(record.wstring(),"content-v2");CHECK(next.begin_request());
        CHECK(next.complete(0,0).persisted && read(record)=="content-v2\n");
        NoticeAcknowledgement same(record.wstring(),"content-v2");CHECK(!same.should_offer());
    }
    std::cout<<"PASS notice acknowledgement: no write before confirmation, cancel/close, one offer per session, submission rollback, version reload, failed persistence and old-record preservation. Isolated path only; no native UI or real user profile access.\n";
}
