#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class SubdivisionSurface {
public:
    static SubdivisionSurface& instance() { static SubdivisionSurface s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
