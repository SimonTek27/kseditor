#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class AdvancedSculpt {
public:
    static AdvancedSculpt& instance() { static AdvancedSculpt s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
