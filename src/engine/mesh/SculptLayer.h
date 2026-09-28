#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class SculptLayer {
public:
    static SculptLayer& instance() { static SculptLayer s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
