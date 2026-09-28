#pragma once
#include <string>
namespace ks { namespace scripting {
class LuaScriptHost {
public:
    static LuaScriptHost& instance() { static LuaScriptHost s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool eval(const std::string&) { return false; }
};
}} // namespace
