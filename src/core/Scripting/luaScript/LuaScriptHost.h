#pragma once

#include "../ScriptHost.h"

// Forward declare Lua types to avoid header dependency when Lua is not available
struct lua_State;

namespace ks {

class SceneGraph;

class LuaScriptHost : public ScriptHost {
    Q_OBJECT

public:
    explicit LuaScriptHost(QObject* parent = nullptr);
    ~LuaScriptHost() override;

    bool initialize() override;
    void shutdown() override;
    bool execute(const QString& code, QString& output, QString& error) override;
    bool executeFile(const QString& filePath, QString& output, QString& error) override;
    QVariant getVariable(const QString& name) override;
    void setVariable(const QString& name, const QVariant& value) override;

    bool isAvailable() const;

    void setSceneGraph(SceneGraph* sg) { m_sceneGraph = sg; }
    SceneGraph* sceneGraph() const { return m_sceneGraph; }

private:
    lua_State* m_lua = nullptr;
    SceneGraph* m_sceneGraph = nullptr;

    // C callback trampolines
    static int luaPrint(lua_State* L);
    static int luaSceneObjectCount(lua_State* L);
    static int luaSceneFindByName(lua_State* L);
    static int luaSceneFindByType(lua_State* L);
    static int luaSceneCreateObject(lua_State* L);
    static int luaSceneDeleteObject(lua_State* L);
    static int luaSceneSetTranslation(lua_State* L);
    static int luaSceneGetTranslation(lua_State* L);
    static int luaSceneSetMaterial(lua_State* L);
    static int luaSceneUpdateTransforms(lua_State* L);
    static int luaSceneSave(lua_State* L);
    static int luaSceneLoad(lua_State* L);

    void registerBuiltinFunctions();
    bool pushQVariant(lua_State* L, const QVariant& value);
    QVariant popQVariant(lua_State* L);
};

} // namespace ks
