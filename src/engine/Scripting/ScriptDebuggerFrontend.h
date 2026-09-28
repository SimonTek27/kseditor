#pragma once
#include <string>

namespace ks {
namespace scripting {

class ScriptDebuggerFrontend {
public:
    static ScriptDebuggerFrontend& instance() { static ScriptDebuggerFrontend s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void setBreakpoint(const std::string& /*file*/, int /*line*/) {}
};

} // namespace scripting
} // namespace ks
