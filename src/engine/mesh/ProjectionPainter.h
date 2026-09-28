#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class ProjectionPainter {
public:
    static ProjectionPainter& instance() { static ProjectionPainter s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
