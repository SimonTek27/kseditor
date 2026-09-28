#pragma once
#include <string>
#include <vector>
namespace ks { namespace engine { namespace graphics {
class SceneGraph {
public:
    static SceneGraph& instance() { static SceneGraph s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void clear() {}
};
}}} // namespace
