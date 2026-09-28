#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class PythonBridge {
public:
    static PythonBridge& instance() { static PythonBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
