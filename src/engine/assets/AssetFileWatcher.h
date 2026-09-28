#pragma once
#include <string>
namespace ks { namespace engine { namespace assets {
class AssetFileWatcher {
public:
    static AssetFileWatcher& instance() { static AssetFileWatcher s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
