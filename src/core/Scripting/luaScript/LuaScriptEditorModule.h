#pragma once

#include "../../editor/EditorModule.h"
#include "../../textEditor/CodeEditor.h"
#include "../../textEditor/SyntaxHighlighter.h"
#include "../../textEditor/FindReplaceDialog.h"
#include <QDockWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QProcess>
#include <QTabWidget>
#include <QCompleter>
#include <QStringListModel>

namespace ks {

class LuaScriptTab {
public:
    CodeEditor* editor = nullptr;
    SyntaxHighlighter* highlighter = nullptr;
    QString filePath;
    QString fileType;
    bool modified = false;
    int tabIndex = -1;
};

class LuaScriptEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit LuaScriptEditorModule(QWidget* parent = nullptr);
    ~LuaScriptEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Lua Script Editor"; }
    QString moduleId() const override { return "luaScriptEditor"; }
    int getModulePriority() const override { return 44; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onNewFile();
    void onLoadFile();
    void onSaveFile();
    void onSaveAsFile();
    void onExecuteScript();
    void onFindReplace();
    void onCloseTab(int index);
    void onTabChanged(int index);
    void onTextChanged();
    void onLuaProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void setupUi();
    LuaScriptTab* createTab(const QString& title, const QString& filePath = QString());
    LuaScriptTab* currentTab() const;
    void updateTabTitle(LuaScriptTab* tab);
    void updateLineCount();
    bool maybeSaveModified();

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QComboBox* m_fileTypeCombo = nullptr;
    QLineEdit* m_filePathEdit = nullptr;
    QPushButton* m_newBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_saveAsBtn = nullptr;
    QPushButton* m_findBtn = nullptr;
    QPushButton* m_executeBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_lineCountLabel = nullptr;
    QProcess* m_luaProcess = nullptr;
    FindReplaceDialog* m_findReplaceDialog = nullptr;
    QStringList m_luaCompletions;
    QVector<LuaScriptTab*> m_tabs;
};

} // namespace ks
