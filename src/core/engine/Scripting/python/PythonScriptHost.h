#pragma once

#include "../ScriptHost.h"

// Forward declare Python types
struct _object;
typedef struct _object PyObject;

namespace ks {

class PythonScriptHost : public ScriptHost {
    Q_OBJECT

public:
    explicit PythonScriptHost(QObject* parent = nullptr);
    ~PythonScriptHost() override;

    bool initialize() override;
    void shutdown() override;
    bool execute(const QString& code, QString& output, QString& error) override;
    bool executeFile(const QString& filePath, QString& output, QString& error) override;
    QVariant getVariable(const QString& name) override;
    void setVariable(const QString& name, const QVariant& value) override;

    bool isAvailable() const;

private:
    PyObject* m_pyMain = nullptr;
    PyObject* m_pyBuiltins = nullptr;

    void registerBuiltinFunctions();
    bool pushQVariant(PyObject* module, const QString& name, const QVariant& value);
    QVariant callPythonFunction(const QString& funcName, const QVariantList& args);
};

} // namespace ks
