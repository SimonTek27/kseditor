#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class LODSystem {
public:
    static LODSystem& instance() { static LODSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
