#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class InteractiveRetopoTool {
public:
    static InteractiveRetopoTool& instance() { static InteractiveRetopoTool s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
