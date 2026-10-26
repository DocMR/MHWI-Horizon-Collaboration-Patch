#include "text_resource_selector.h"
#include "generated_text_deltas.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
int main() {
    using namespace stormslinger;
    std::size_t accepted = 0;
    for (const auto family : {"item", "lbg", "wep_series"}) {
        for (const auto language : text_languages) {
            std::string path = std::string("common/text/steam/") + family + "_" + std::string(language);
            const auto result = select_text_resource(path.c_str(), path.size() + 1);
            assert(result);
            assert(text_languages[result.language] == language);
            const auto expected = std::string_view(family) == "item" ? TextFamily::item :
                std::string_view(family) == "lbg" ? TextFamily::lbg : TextFamily::series;
            assert(result.family == expected);
            const auto* group = find_text_deltas(result);
            assert(group && group->count == (expected == TextFamily::series ? 1u : 4u));
            assert(group->start + group->count <= text_deltas.size());
            assert(!select_text_resource(path.data(), path.size()));
            for (auto& c : path) if (c == '/') c = '\\';
            assert(select_text_resource(path.c_str(), path.size() + 1));
            ++accepted;
        }
    }
    for (const std::string path : {"common/text/steam/lbg_eng.gmd", "common/text/steam/lbg_ENG",
         "common/text/steam/lbg_oth", "common/text/steam/lbg_eng_extra", "common/text/steam/lbg",
         "nativePC/common/text/steam/lbg_eng", "common/text/steam/../steam/lbg_eng",
         "common/text/steam/notlbg_eng", "/common/text/steam/lbg_eng"})
        assert(!select_text_resource(path.c_str(), path.size() + 1));
    std::array<char, 79> unterminated{};
    unterminated.fill('x');
    assert(!select_text_resource(unterminated.data(), unterminated.size()));
    assert(!select_text_resource(nullptr, 79));
    const char partial[] = {'c', 'o', 'm'};
    assert(!select_text_resource(partial, sizeof(partial)));
    assert(!find_text_deltas({TextFamily::lbg, 99}));
    assert(!find_text_deltas({}));
    // Export compiled string bytes for independent comparison with source JSON.
    std::ofstream dump("../research/native-text-delta-compiled.bin", std::ios::binary);
    assert(dump);
    for (const auto& entry : text_deltas) {
        assert(entry.key_count > 0 && entry.key_start + entry.key_count <= text_delta_keys.size());
        for (const auto value : {text_delta_keys[entry.key_start], entry.before, entry.after}) {
            const auto length = static_cast<unsigned int>(value.size());
            dump.write(reinterpret_cast<const char*>(&length), sizeof(length));
            dump.write(value.data(), static_cast<std::streamsize>(value.size()));
        }
    }
    dump.close();
    assert(dump);
    std::cout << "42 target paths accepted; separators, bounds and non-target rejection passed\n";
    assert(accepted == 42);
}
