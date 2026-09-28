#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class WeightPainting {
public:
    static WeightPainting& instance() { static WeightPainting s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
