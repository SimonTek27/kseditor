#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class Viewport3DSystem {
public:
    static Viewport3DSystem& instance() { static Viewport3DSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
