#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class CacheManager {
public:
    static CacheManager& instance() { static CacheManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void clear() {}
};
}}} // namespace
