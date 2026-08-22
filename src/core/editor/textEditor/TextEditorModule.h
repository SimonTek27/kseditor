#pragma once

#include "../editor/EditorModule.h"
#include <QDockWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>
#include <QMap>
#include <QFileSystemWatcher>
#include <QVBoxLayout>

namespace ks {

class CodeEditor;
class FindReplaceDialog;

class TextEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit TextEditorModule(QWidget* parent = nullptr);
    ~TextEditorModule() override;

    QString moduleName() const override { return "Text Editor"; }
    QString moduleId() const override { return "textEditor"; }
    int getModulePriority() const override { return 60; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    bool initialize() override;
    void shutdown() override;

    void importFile(const QString& filePath) override;
    void openFileAtLine(const QString& filePath, int line);
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

    bool canCut() const override;
    bool canCopy() const override;
    bool canPaste() const override;
    bool canDelete() const override;
    void cut() override;
    void copy() override;
    void paste() override;
    void deleteSelected() override;

protected:
    void onActivation() override;
    void onDeactivation() override;

signals:
    void fileOpened(const QString& path);
    void fileClosed(const QString& path);
    void fileSaved(const QString& path);

private slots:
    void onNewFile();
    void onOpenFile();
    void onSaveFile();
    void onSaveAsFile();
    void onCloseTab(int index);
    void onCloseCurrentTab();
    void onTabChanged(int index);
    void onDocumentModified();
    void onUndo();
    void onRedo();
    void onFind();
    void onReplace();
    void onGotoLine();
    void onZoomIn();
    void onZoomOut();
    void onResetZoom();
    void onFoldAll();
    void onUnfoldAll();

private:
    void setupUI();
    void setupToolbar(QVBoxLayout* mainLayout);
    void updateTitle(int index);
    QString getTabText(int index) const;
    QString getTabPath(int index) const;
    CodeEditor* currentEditor() const;
    int findTabByPath(const QString& path) const;
    void setEditorModified(int index, bool modified);
    void applyZoom(int delta);

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QToolBar* m_toolbar = nullptr;

    FindReplaceDialog* m_findDialog = nullptr;

    QMap<int, QString> m_tabPaths;
    QMap<int, bool> m_tabModified;

    QPushButton* m_newBtn = nullptr;
    QPushButton* m_openBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_saveAsBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
    QPushButton* m_undoBtn = nullptr;
    QPushButton* m_redoBtn = nullptr;
    QPushButton* m_findBtn = nullptr;
    QPushButton* m_replaceBtn = nullptr;
    QPushButton* m_gotoBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    static constexpr int kZoomStep = 1;
    static constexpr int kMinZoom = 6;
    static constexpr int kMaxZoom = 40;
    static constexpr int kBaseFontSize = 13;
    int m_currentFontSize = kBaseFontSize;
};

} // namespace ks
