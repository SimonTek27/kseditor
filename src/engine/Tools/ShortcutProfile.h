#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class ShortcutProfile {
public:
    static ShortcutProfile& instance() { static ShortcutProfile s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
