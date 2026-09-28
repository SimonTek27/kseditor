#pragma once
#include <string>
namespace ks { namespace scripting {
class ScriptHost {
public:
    static ScriptHost& instance() { static ScriptHost s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool eval(const std::string& /*code*/) { return false; }
};
}} // namespace
