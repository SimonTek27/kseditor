#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class ThemeSystem {
public:
    static ThemeSystem& instance() { static ThemeSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
