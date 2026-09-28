#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class ModuleManager {
public:
    static ModuleManager& instance() { static ModuleManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
