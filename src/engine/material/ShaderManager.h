#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class ShaderManager {
public:
    static ShaderManager& instance() { static ShaderManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
