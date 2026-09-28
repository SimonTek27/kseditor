#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class PaintLayersManager {
public:
    static PaintLayersManager& instance() { static PaintLayersManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
