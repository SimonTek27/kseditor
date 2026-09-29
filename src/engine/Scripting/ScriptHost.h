#pragma once
#include <string>

namespace ks { namespace scripting {

// Facade over the concrete script host. Everything else in the engine talks
// to this type only, so swapping in another backend (or a HAS_LUA=0 build)
// never ripples past Scripting/.
class ScriptHost {
public:
    static ScriptHost& instance() { static ScriptHost s; return s; }

    bool initialize();
    void shutdown();
    bool isInitialized() const;

    // Runs a chunk or an expression; see LuaScriptHost::eval().
    bool eval(const std::string& code);
    bool runFile(const std::string& path);

    bool getNumber(const std::string& name, double& out) const;
    bool setNumber(const std::string& name, double value);

    // True when `name` is a global holding a function; never touches
    // lastError(), so it is safe to poll every tick looking for a hook.
    bool hasFunction(const std::string& name) const;

    // Calls the global function `name` with a single numeric argument.
    bool callFunction(const std::string& name, double arg);

    // Message from the last failed call, or empty after a successful one.
    const std::string& lastError() const;
};

}} // namespace ks::scripting
