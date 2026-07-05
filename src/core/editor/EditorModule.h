#pragma once
#include <QWidget>
#include <QString>
#include <QMainWindow>
#include <QDockWidget>
#include <QJsonObject>

namespace ks {

class EditorModule : public QWidget {
    Q_OBJECT
public:
    explicit EditorModule(QWidget* parent = nullptr) : QWidget(parent) {}
    virtual ~EditorModule() = default;

    // Core module identity / lifecycle
    virtual bool initialize() { return true; }
    virtual void shutdown() {}
    virtual QString moduleName() const { return QString(); }
    virtual QString moduleId() const { return QString(); }

    // Convenience wrappers
    virtual QString getModuleName() const { return moduleName(); }
    virtual QString getModuleIcon() const { return QString(); }
    virtual int getModulePriority() const { return 0; }
    virtual QDockWidget* getOrCreateDockWidget(QMainWindow* /*mainWindow*/) { return nullptr; }

    // File/project operations (override in modules that support them)
    virtual void exportFile(const QString& /*filePath*/) {}
    virtual void importFile(const QString& /*filePath*/) {}
    virtual void newProject(const QString& name, const QString& path);
    virtual void openProject(const QString& projectPath);
    virtual void saveProject(const QString& path = QString());
    virtual void saveProjectAs(const QString& path);

    QString projectPath() const { return m_projectPath; }
    void setProjectPath(const QString& path) { m_projectPath = path; }

    // Edit operations (can/call)
    virtual bool canCut() const { return false; }
    virtual bool canCopy() const { return false; }
    virtual bool canPaste() const { return false; }
    virtual bool canDelete() const { return false; }

    virtual void cut() {}
    virtual void copy() {}
    virtual void paste() {}
    virtual void deleteSelected() {}

    // Serialization helpers for subclasses
    virtual QJsonObject serializeProject() const { return QJsonObject(); }
    virtual void deserializeProject(const QJsonObject& /*data*/) {}

protected:
    QString m_projectPath;

public slots:
    virtual void onActivation() {}
    virtual void onDeactivation() {}
};

}