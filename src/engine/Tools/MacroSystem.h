#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class MacroSystem {
public:
    static MacroSystem& instance() { static MacroSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
