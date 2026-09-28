#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class SymmetryManager {
public:
    static SymmetryManager& instance() { static SymmetryManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
