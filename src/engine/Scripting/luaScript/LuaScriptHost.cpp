#include "LuaScriptHost.h"

// Defensive: ksengine always publishes HAS_LUA through its PUBLIC compile
// definitions, but a TU that reaches this file without them should read as
// "no Lua" rather than trip a warning about an undefined macro.
#ifndef HAS_LUA
#define HAS_LUA 0
#endif

#if HAS_LUA
// Lua's headers are written for C and carry no `extern "C"` guard of their
// own, so without this wrapper every api call would be name-mangled here and
// never match the symbols in ksengine_lua.
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

#include <cstdio>

namespace ks { namespace scripting {

#if HAS_LUA
namespace {
// Wrap a chunk as `return <code>` so eval() accepts plain expressions
// (`1+2`, `speed * 2`) as well as statements. Anything that still fails to
// parse is retried verbatim, which is how `x = 1` gets through.
int loadAsExpression(lua_State* L, const std::string& code) {
    const std::string wrapped = "return " + code;
    if (luaL_loadbuffer(L, wrapped.c_str(), wrapped.size(), "eval") == LUA_OK) return LUA_OK;
    lua_pop(L, 1);
    return luaL_loadbuffer(L, code.c_str(), code.size(), "eval");
}
} // namespace
#endif

LuaScriptHost::~LuaScriptHost() { shutdown(); }

bool LuaScriptHost::initialize() {
#if HAS_LUA
    if (m_state) return true;
    m_state = luaL_newstate();
    if (!m_state) {
        m_lastError = "luaL_newstate() returned null";
        return false;
    }
    luaL_openlibs(m_state);
    m_lastError.clear();
    return true;
#else
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

void LuaScriptHost::shutdown() {
#if HAS_LUA
    if (m_state) {
        lua_close(m_state);
        m_state = nullptr;
    }
#endif
}

bool LuaScriptHost::isInitialized() const {
#if HAS_LUA
    return m_state != nullptr;
#else
    return false;
#endif
}

bool LuaScriptHost::report(int status) {
#if HAS_LUA
    if (status == LUA_OK) {
        m_lastError.clear();
        return true;
    }
    const char* msg = lua_tostring(m_state, -1);
    m_lastError = msg ? msg : "unknown Lua error";
    lua_pop(m_state, 1);
    return false;
#else
    (void)status;
    return false;
#endif
}

bool LuaScriptHost::eval(const std::string& code) {
#if HAS_LUA
    if (!m_state) {
        m_lastError = "script host not initialized";
        return false;
    }
    if (!report(loadAsExpression(m_state, code))) return false;
    return report(lua_pcall(m_state, 0, 0, 0));
#else
    (void)code;
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

bool LuaScriptHost::runFile(const std::string& path) {
#if HAS_LUA
    if (!m_state) {
        m_lastError = "script host not initialized";
        return false;
    }
    if (!report(luaL_loadfile(m_state, path.c_str()))) return false;
    return report(lua_pcall(m_state, 0, 0, 0));
#else
    (void)path;
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

bool LuaScriptHost::getNumber(const std::string& name, double& out) const {
#if HAS_LUA
    if (!m_state) {
        m_lastError = "script host not initialized";
        return false;
    }
    lua_getglobal(m_state, name.c_str());
    const bool ok = lua_isnumber(m_state, -1) != 0;
    if (ok) out = lua_tonumber(m_state, -1);
    else m_lastError = "global '" + name + "' is not a number";
    lua_pop(m_state, 1);
    return ok;
#else
    (void)name;
    (void)out;
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

bool LuaScriptHost::setNumber(const std::string& name, double value) {
#if HAS_LUA
    if (!m_state) {
        m_lastError = "script host not initialized";
        return false;
    }
    lua_pushnumber(m_state, value);
    lua_setglobal(m_state, name.c_str());
    m_lastError.clear();
    return true;
#else
    (void)name;
    (void)value;
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

bool LuaScriptHost::hasFunction(const std::string& name) const {
#if HAS_LUA
    if (!m_state) return false;
    lua_getglobal(m_state, name.c_str());
    const bool ok = lua_isfunction(m_state, -1) != 0;
    lua_pop(m_state, 1);
    return ok;
#else
    (void)name;
    return false;
#endif
}

bool LuaScriptHost::callFunction(const std::string& name, double arg) {
#if HAS_LUA
    if (!m_state) {
        m_lastError = "script host not initialized";
        return false;
    }
    lua_getglobal(m_state, name.c_str());
    if (!lua_isfunction(m_state, -1)) {
        m_lastError = "global '" + name + "' is not a function";
        lua_pop(m_state, 1);
        return false;
    }
    lua_pushnumber(m_state, arg);
    return report(lua_pcall(m_state, 1, 0, 0));
#else
    (void)name;
    (void)arg;
    m_lastError = "Lua support was not compiled in (HAS_LUA=0)";
    return false;
#endif
}

}} // namespace ks::scripting
