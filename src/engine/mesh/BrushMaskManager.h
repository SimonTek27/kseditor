#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class BrushMaskManager {
public:
    static BrushMaskManager& instance() { static BrushMaskManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
