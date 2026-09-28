#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class ScientificCalculator {
public:
    static ScientificCalculator& instance() { static ScientificCalculator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
