#pragma once
#include <string>
namespace ks { namespace scripting {
class ScriptDebugger {
public:
    static ScriptDebugger& instance() { static ScriptDebugger s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}} // namespace
