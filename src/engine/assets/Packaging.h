#pragma once
#include <string>

namespace ks {
namespace engine {
namespace assets {

class Packaging {
public:
    static bool pack(const std::string& /*srcDir*/, const std::string& /*outFile*/) { return false; }
    static bool unpack(const std::string& /*inFile*/, const std::string& /*dstDir*/) { return false; }
};

} // namespace assets
} // namespace engine
} // namespace ks
