#pragma once
#include <string>

namespace ks {
namespace engine {
namespace assets {

class RaceFlagAssetLoader {
public:
    static RaceFlagAssetLoader& instance() { static RaceFlagAssetLoader s; return s; }
    bool load(const std::string& /*path*/) { return false; }
};

} // namespace assets
} // namespace engine
} // namespace ks
