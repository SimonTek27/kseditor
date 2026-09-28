#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class OpenGLRenderer {
public:
    static OpenGLRenderer& instance() { static OpenGLRenderer s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
