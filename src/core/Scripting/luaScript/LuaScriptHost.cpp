#include "LuaScriptHost.h"
#include "core/Graphics/SceneGraph.h"
#include "core/Graphics/SceneObject.h"
#include <QDebug>
#include <QFile>

#if HAS_LUA
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#endif

namespace ks {

LuaScriptHost::LuaScriptHost(QObject* parent)
    : ScriptHost(Language::Lua, parent)
{
}

LuaScriptHost::~LuaScriptHost() {
    shutdown();
}

bool LuaScriptHost::isAvailable() const {
#if HAS_LUA
    return true;
#else
    return false;
#endif
}

bool LuaScriptHost::initialize() {
#if HAS_LUA
    if (m_lua) return true;

    m_lua = luaL_newstate();
    if (!m_lua) {
        emit error("Failed to create Lua state");
        return false;
    }

    luaL_openlibs(m_lua);
    registerBuiltinFunctions();

    m_initialized = true;
    return true;
#else
    emit error("Lua not available (HAS_LUA not defined). Install Lua development libraries.");
    return false;
#endif
}

void LuaScriptHost::shutdown() {
#if HAS_LUA
    if (m_lua) {
        lua_close(m_lua);
        m_lua = nullptr;
    }
    m_initialized = false;
#endif
}

bool LuaScriptHost::execute(const QString& code, QString& output, QString& error) {
#if HAS_LUA
    if (!m_lua) {
        error = "Lua not initialized";
        return false;
    }

    int result = luaL_dostring(m_lua, code.toUtf8().constData());
    if (result != LUA_OK) {
        error = QString(lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);
        emit this->error(error);
        return false;
    }

    // Get return value if any
    if (lua_gettop(m_lua) > 0) {
        output = popQVariant(m_lua).toString();
    }
    return true;
#else
    Q_UNUSED(code);
    output.clear();
    error = "Lua not available";
    return false;
#endif
}

bool LuaScriptHost::executeFile(const QString& filePath, QString& output, QString& error) {
#if HAS_LUA
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray code = file.readAll();
    file.close();

    return execute(QString::fromUtf8(code), output, error);
#else
    Q_UNUSED(filePath);
    output.clear();
    error = "Lua not available";
    return false;
#endif
}

QVariant LuaScriptHost::getVariable(const QString& name) {
#if HAS_LUA
    if (!m_lua) return QVariant();

    lua_getglobal(m_lua, name.toUtf8().constData());
    QVariant result = popQVariant(m_lua);
    return result;
#else
    Q_UNUSED(name);
    return QVariant();
#endif
}

void LuaScriptHost::setVariable(const QString& name, const QVariant& value) {
#if HAS_LUA
    if (!m_lua) return;

    pushQVariant(m_lua, value);
    lua_setglobal(m_lua, name.toUtf8().constData());
#endif
}

void LuaScriptHost::registerBuiltinFunctions() {
#if HAS_LUA
    // Store host pointer so trampolines can retrieve it
    lua_pushlightuserdata(m_lua, this);
    lua_setglobal(m_lua, "__ks_host");

    // Register C functions callable from Lua
    lua_pushcfunction(m_lua, luaPrint);
    lua_setglobal(m_lua, "print");

    lua_pushcfunction(m_lua, luaSceneObjectCount);
    lua_setglobal(m_lua, "scene_object_count");

    lua_pushcfunction(m_lua, luaSceneFindByName);
    lua_setglobal(m_lua, "scene_find_by_name");

    lua_pushcfunction(m_lua, luaSceneFindByType);
    lua_setglobal(m_lua, "scene_find_by_type");

    lua_pushcfunction(m_lua, luaSceneCreateObject);
    lua_setglobal(m_lua, "scene_create_object");

    lua_pushcfunction(m_lua, luaSceneDeleteObject);
    lua_setglobal(m_lua, "scene_delete_object");

    lua_pushcfunction(m_lua, luaSceneSetTranslation);
    lua_setglobal(m_lua, "scene_set_translation");

    lua_pushcfunction(m_lua, luaSceneGetTranslation);
    lua_setglobal(m_lua, "scene_get_translation");

    lua_pushcfunction(m_lua, luaSceneSetMaterial);
    lua_setglobal(m_lua, "scene_set_material");

    lua_pushcfunction(m_lua, luaSceneUpdateTransforms);
    lua_setglobal(m_lua, "scene_update_transforms");

    lua_pushcfunction(m_lua, luaSceneSave);
    lua_setglobal(m_lua, "scene_save");

    lua_pushcfunction(m_lua, luaSceneLoad);
    lua_setglobal(m_lua, "scene_load");
#endif
}

// ── Lua C callback trampolines ──────────────────────────────────────────
#if HAS_LUA

static LuaScriptHost* getHost(lua_State* L) {
    lua_getglobal(L, "__ks_host");
    void* ud = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return static_cast<LuaScriptHost*>(ud);
}

int LuaScriptHost::luaPrint(lua_State* L) {
    int n = lua_gettop(L);
    QString msg;
    for (int i = 1; i <= n; ++i) {
        if (i > 1) msg += QStringLiteral("\t");
        msg += QString::fromUtf8(lua_tostring(L, i));
    }
    qDebug() << "[Lua]" << msg;
    return 0;
}

int LuaScriptHost::luaSceneObjectCount(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    if (!host || !host->m_sceneGraph) {
        lua_pushinteger(L, 0);
        return 1;
    }
    lua_pushinteger(L, host->m_sceneGraph->objectCount());
    return 1;
}

int LuaScriptHost::luaSceneFindByName(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    const char* name = lua_tostring(L, 1);
    if (!host || !host->m_sceneGraph || !name) {
        lua_pushinteger(L, -1);
        return 1;
    }
    SceneObject* obj = host->m_sceneGraph->findObjectByName(QString::fromUtf8(name));
    lua_pushinteger(L, obj ? obj->id() : -1);
    return 1;
}

int LuaScriptHost::luaSceneFindByType(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    int typeInt = static_cast<int>(lua_tointeger(L, 1));
    if (!host || !host->m_sceneGraph) {
        lua_newtable(L);
        return 1;
    }
    auto type = static_cast<SceneObject::Type>(typeInt);
    auto objects = host->m_sceneGraph->findObjectsByType(type);
    lua_newtable(L);
    for (int i = 0; i < objects.size(); ++i) {
        lua_pushinteger(L, objects[i]->id());
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

int LuaScriptHost::luaSceneCreateObject(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    const char* name = lua_tostring(L, 1);
    int typeInt = static_cast<int>(lua_tointeger(L, 2));
    if (!host || !host->m_sceneGraph || !name) {
        lua_pushinteger(L, -1);
        return 1;
    }
    auto type = static_cast<SceneObject::Type>(typeInt);
    SceneObject* obj = host->m_sceneGraph->createObject(QString::fromUtf8(name), type);
    lua_pushinteger(L, obj->id());
    return 1;
}

int LuaScriptHost::luaSceneDeleteObject(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    int id = static_cast<int>(lua_tointeger(L, 1));
    if (!host || !host->m_sceneGraph) {
        lua_pushboolean(L, 0);
        return 1;
    }
    SceneObject* obj = host->m_sceneGraph->findObjectById(id);
    if (obj && obj != host->m_sceneGraph->root()) {
        host->m_sceneGraph->deleteObject(obj);
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaScriptHost::luaSceneSetTranslation(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    int id = static_cast<int>(lua_tointeger(L, 1));
    float x = static_cast<float>(lua_tonumber(L, 2));
    float y = static_cast<float>(lua_tonumber(L, 3));
    float z = static_cast<float>(lua_tonumber(L, 4));
    if (!host || !host->m_sceneGraph) {
        lua_pushboolean(L, 0);
        return 1;
    }
    SceneObject* obj = host->m_sceneGraph->findObjectById(id);
    if (obj) {
        obj->setTranslation(QVector3D(x, y, z));
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaScriptHost::luaSceneGetTranslation(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    int id = static_cast<int>(lua_tointeger(L, 1));
    lua_newtable(L);
    if (!host || !host->m_sceneGraph) return 1;
    SceneObject* obj = host->m_sceneGraph->findObjectById(id);
    if (obj) {
        auto t = obj->translation();
        lua_pushnumber(L, t.x());
        lua_rawseti(L, -2, 1);
        lua_pushnumber(L, t.y());
        lua_rawseti(L, -2, 2);
        lua_pushnumber(L, t.z());
        lua_rawseti(L, -2, 3);
    }
    return 1;
}

int LuaScriptHost::luaSceneSetMaterial(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    int id = static_cast<int>(lua_tointeger(L, 1));
    const char* matName = lua_tostring(L, 2);
    if (!host || !host->m_sceneGraph || !matName) {
        lua_pushboolean(L, 0);
        return 1;
    }
    SceneObject* obj = host->m_sceneGraph->findObjectById(id);
    if (obj && obj->mesh()) {
        obj->mesh()->materialName = QString::fromUtf8(matName);
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaScriptHost::luaSceneUpdateTransforms(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    if (!host || !host->m_sceneGraph) {
        lua_pushboolean(L, 0);
        return 1;
    }
    host->m_sceneGraph->updateAllTransforms();
    lua_pushboolean(L, 1);
    return 1;
}

int LuaScriptHost::luaSceneSave(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    const char* path = lua_tostring(L, 1);
    if (!host || !host->m_sceneGraph || !path) {
        lua_pushboolean(L, 0);
        return 1;
    }
    bool ok = host->m_sceneGraph->saveToFile(QString::fromUtf8(path));
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

int LuaScriptHost::luaSceneLoad(lua_State* L) {
    LuaScriptHost* host = getHost(L);
    const char* path = lua_tostring(L, 1);
    if (!host || !host->m_sceneGraph || !path) {
        lua_pushboolean(L, 0);
        return 1;
    }
    bool ok = host->m_sceneGraph->loadFromFile(QString::fromUtf8(path));
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

#else // !HAS_LUA - provide stub implementations

int LuaScriptHost::luaPrint(lua_State*) { return 0; }
int LuaScriptHost::luaSceneObjectCount(lua_State*) { return 0; }
int LuaScriptHost::luaSceneFindByName(lua_State*) { return 0; }
int LuaScriptHost::luaSceneFindByType(lua_State*) { return 0; }
int LuaScriptHost::luaSceneCreateObject(lua_State*) { return 0; }
int LuaScriptHost::luaSceneDeleteObject(lua_State*) { return 0; }
int LuaScriptHost::luaSceneSetTranslation(lua_State*) { return 0; }
int LuaScriptHost::luaSceneGetTranslation(lua_State*) { return 0; }
int LuaScriptHost::luaSceneSetMaterial(lua_State*) { return 0; }
int LuaScriptHost::luaSceneUpdateTransforms(lua_State*) { return 0; }
int LuaScriptHost::luaSceneSave(lua_State*) { return 0; }
int LuaScriptHost::luaSceneLoad(lua_State*) { return 0; }

#endif // HAS_LUA

bool LuaScriptHost::pushQVariant(lua_State* L, const QVariant& value) {
#if HAS_LUA
    switch (value.typeId()) {
        case QMetaType::Bool:
            lua_pushboolean(L, value.toBool());
            return true;
        case QMetaType::Int:
        case QMetaType::LongLong:
            lua_pushinteger(L, value.toInt());
            return true;
        case QMetaType::Double:
        case QMetaType::Float:
            lua_pushnumber(L, value.toDouble());
            return true;
        case QMetaType::QString:
            lua_pushstring(L, value.toString().toUtf8().constData());
            return true;
        default:
            lua_pushnil(L);
            return true;
    }
#else
    Q_UNUSED(L);
    return false;
#endif
}

QVariant LuaScriptHost::popQVariant(lua_State* L) {
#if HAS_LUA
    int type = lua_type(L, -1);
    QVariant result;
    switch (type) {
        case LUA_TBOOLEAN:
            result = QVariant((bool)lua_toboolean(L, -1));
            break;
        case LUA_TNUMBER:
            result = QVariant(lua_tonumber(L, -1));
            break;
        case LUA_TSTRING:
            result = QVariant(QString::fromUtf8(lua_tostring(L, -1)));
            break;
        default:
            break;
    }
    lua_pop(L, 1);
    return result;
#else
    Q_UNUSED(L);
    return QVariant();
#endif
}

} // namespace ks
