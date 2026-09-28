#pragma once
#include <string>
namespace ks { namespace engine { namespace config {
class ConfigIntegration {
public:
    static ConfigIntegration& instance() { static ConfigIntegration s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
