#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class NormalMapBaker {
public:
    static NormalMapBaker& instance() { static NormalMapBaker s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
