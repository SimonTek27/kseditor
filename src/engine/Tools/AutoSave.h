#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class AutoSave {
public:
    static AutoSave& instance() { static AutoSave s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
