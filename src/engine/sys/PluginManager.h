#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class PluginManager {
public:
    static PluginManager& instance() { static PluginManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
