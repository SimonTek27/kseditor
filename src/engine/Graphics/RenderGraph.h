#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class RenderGraph {
public:
    static RenderGraph& instance() { static RenderGraph s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
