#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class SceneData {
public:
    static SceneData& instance() { static SceneData s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
