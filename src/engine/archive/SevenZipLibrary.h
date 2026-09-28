#pragma once
/** Qt-free stub — 7z integration optional; excluded from minimal link if needed. */
#include <string>
#include <vector>

namespace ks {
namespace engine {
namespace archive {

class SevenZipLibrary {
public:
    static SevenZipLibrary& instance() { static SevenZipLibrary s; return s; }
    bool initialize() { return false; }
    void shutdown() {}
    bool extract(const std::string& /*archive*/, const std::string& /*dest*/) { return false; }
    std::vector<std::string> list(const std::string& /*archive*/) const { return {}; }
};

} // namespace archive
} // namespace engine
} // namespace ks
