#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QMainWindow>
#include <QDockWidget>
#include <QList>
#include <QString>
#include "../editor/EditorModule.h"

namespace ks { class EditorModule; }

/**
 * @brief Module Manager - Manages all editor modules and their switching
 */
class ModuleManager : public QWidget
{
    Q_OBJECT

public:
    explicit ModuleManager(QWidget* parent = nullptr);
    ~ModuleManager() override;

    // Module management
    int moduleCount() const { return m_modules.size(); }
    int currentModule() const { return m_stackedWidget->currentIndex(); }
    QString moduleName(int index) const;
    int moduleIndex(const QString& name) const;

    ks::EditorModule* currentEditorModule() const;
    ks::EditorModule* editorModule(int index) const;
    QList<ks::EditorModule*> modules() const { return m_modules; }

    // Module switching
    void setCurrentModule(int index);
    void setCurrentModule(const QString& name);

    // Edit operations delegation
    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;
    bool canDelete() const;

    // File operations delegation
    void importFile(const QString& filePath);
    void exportFile(const QString& filePath);

    // Project operations
    void newProject(const QString& name, const QString& path);
    void openProject(const QString& projectPath);
    void saveProject();
    void buildCurrentProject();

signals:
    void moduleChanged(int index);
    void moduleAboutToChange(int from, int to);
    void fileExported(const QString& filePath);
    void fileImported(const QString& filePath);
    void projectCreated(const QString& name, const QString& path);
    void projectOpened(const QString& projectPath);

public slots:
    void registerModule(ks::EditorModule* module);
    void unregisterModule(ks::EditorModule* module);

private:
    void setupUI();
    void loadModules();

    QStackedWidget* m_stackedWidget;
    QList<ks::EditorModule*> m_modules;
};
