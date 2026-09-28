#pragma once
#include <string>
namespace ks { namespace scripting {
class PythonScriptHost {
public:
    static PythonScriptHost& instance() { static PythonScriptHost s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}} // namespace
