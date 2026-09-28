#pragma once
#include <string>
namespace ks { namespace engine { namespace assets {
class AssetDependencyGraph {
public:
    static AssetDependencyGraph& instance() { static AssetDependencyGraph s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
