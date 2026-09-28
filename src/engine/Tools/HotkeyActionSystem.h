#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class HotkeyActionSystem {
public:
    static HotkeyActionSystem& instance() { static HotkeyActionSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
