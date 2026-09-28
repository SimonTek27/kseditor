#pragma once
#include <string>

namespace ks {
namespace engine {
namespace assets {

class CloudSyncManager {
public:
    static CloudSyncManager& instance() { static CloudSyncManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool sync(const std::string& /*localPath*/) { return false; }
};

} // namespace assets
} // namespace engine
} // namespace ks
