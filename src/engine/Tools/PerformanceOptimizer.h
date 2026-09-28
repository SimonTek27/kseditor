#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class PerformanceOptimizer {
public:
    static PerformanceOptimizer& instance() { static PerformanceOptimizer s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
