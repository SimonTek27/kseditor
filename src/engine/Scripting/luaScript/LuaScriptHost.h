#pragma once

#include <string>

// Opaque, declared here so lua.h never has to leak into the public engine
// interface — the state is created, used and destroyed entirely inside
// LuaScriptHost.cpp.
struct lua_State;

namespace ks { namespace scripting {

// Real Lua-backed script host. Owns a single lua_State for the process,
// with the standard libraries opened, and exposes three operations:
//
//   initialize()  create the state (idempotent; false when HAS_LUA=0)
//   eval(code)    run a chunk, or an expression — `1+2` and `x = 1` both
//                 work because the chunk is first tried as `return <code>`
//   runFile(path) load and run a file, like dofile()
//
// Failures never throw: they leave the state untouched, record the message
// in lastError() and return false, so a broken script cannot take the
// simulator down with it.
class LuaScriptHost {
public:
    static LuaScriptHost& instance() { static LuaScriptHost s; return s; }

    LuaScriptHost() = default;
    ~LuaScriptHost();

    LuaScriptHost(const LuaScriptHost&) = delete;
    LuaScriptHost& operator=(const LuaScriptHost&) = delete;

    bool initialize();
    void shutdown();
    bool isInitialized() const;

    bool eval(const std::string& code);
    bool runFile(const std::string& path);

    // Global number slot access — the minimal binding surface a sim needs to
    // hand telemetry in and read script output back out without pushing raw
    // Lua types through the engine.
    bool getNumber(const std::string& name, double& out) const;
    bool setNumber(const std::string& name, double value);

    // True when `name` is a global holding a function. Never touches
    // lastError(), so it is safe to poll every tick looking for a hook.
    bool hasFunction(const std::string& name) const;

    // Calls the global function `name` with a single numeric argument.
    bool callFunction(const std::string& name, double arg);

    const std::string& lastError() const { return m_lastError; }

    lua_State* state() { return m_state; }

private:
    bool report(int status);

    lua_State* m_state = nullptr;
    mutable std::string m_lastError;
};

}} // namespace ks::scripting
