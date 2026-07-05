#pragma once

#include <QString>
#include <QVariantMap>
#include <QObject>
#include <functional>

namespace ks {

class SceneGraph;

class ScriptHost : public QObject {
    Q_OBJECT

public:
    enum class Language { Lua, Python, Unknown };

    explicit ScriptHost(Language lang, QObject* parent = nullptr);
    virtual ~ScriptHost();

    Language language() const { return m_language; }
    bool isInitialized() const { return m_initialized; }

    // Script execution
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool execute(const QString& code, QString& output, QString& error) = 0;
    virtual bool executeFile(const QString& filePath, QString& output, QString& error) = 0;

    // Variable access
    virtual QVariant getVariable(const QString& name) = 0;
    virtual void setVariable(const QString& name, const QVariant& value) = 0;

    // Editor API bindings
    void setSceneGraph(SceneGraph* graph) { m_sceneGraph = graph; }
    SceneGraph* sceneGraph() const { return m_sceneGraph; }

    // Register a C++ function callable from scripts
    using ScriptFunction = std::function<QVariant(const QVariantList&)>;
    void registerFunction(const QString& name, ScriptFunction func);

    // Built-in functions available to all scripts
    void registerBuiltins();

signals:
    void output(const QString& text);
    void error(const QString& text);

protected:
    Language m_language;
    bool m_initialized = false;
    SceneGraph* m_sceneGraph = nullptr;
    QMap<QString, ScriptFunction> m_functions;
};

} // namespace ks
