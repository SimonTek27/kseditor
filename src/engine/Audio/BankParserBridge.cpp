#include "BankParserBridge.h"

#include <cstdio>
#include <filesystem>

namespace ks {
namespace audio {

ParsedBank parseBankFile(const std::string& path)
{
    ParsedBank parsed;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        std::fprintf(stderr, "BankParserBridge: no such file: %s\n", path.c_str());
        return parsed;
    }

    // TODO(qt-free): implement a standalone FMOD bank reader (FEV event
    // table + FSB5 sample blocks) so event-driven engine audio works
    // without the Qt editor's bank tooling.
    std::fprintf(stderr,
                 "BankParserBridge: FMOD .bank parsing not implemented yet "
                 "(Qt-free build): %s\n",
                 path.c_str());
    return parsed;
}

} // namespace audio
} // namespace ks
