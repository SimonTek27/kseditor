#pragma once
#include <string>
namespace ks { namespace scripting {
class PythonScriptEngine {
public:
    static PythonScriptEngine& instance() { static PythonScriptEngine s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}} // namespace
