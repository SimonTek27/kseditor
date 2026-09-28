#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class StreamlineIntegration {
public:
    static StreamlineIntegration& instance() { static StreamlineIntegration s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
