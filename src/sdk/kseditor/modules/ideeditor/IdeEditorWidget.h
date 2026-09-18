#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QBoxLayout>

namespace ks {

class CodeEditor;
class FindReplaceDialog;
class IdeEditorFileBrowser;
class IdeEditorSearchPanel;

class IdeEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit IdeEditorWidget(QWidget* parent = nullptr);
    ~IdeEditorWidget() override;

    void openFile(const QString& filePath);
    CodeEditor* currentEditor() const;
    int tabCount() const { return m_tabWidget ? m_tabWidget->count() : 0; }
    IdeEditorFileBrowser* fileBrowser() const { return m_fileBrowser; }
    IdeEditorSearchPanel* searchPanel() const { return m_searchPanel; }

    // File operations
    void newFile();
    void openFiles();
    void saveCurrent();
    void saveCurrentAs();
    void closeCurrentTab();
    void closeTab(int index);

    // Edit operations
    bool canCut() const;
    bool canCopy() const;
    bool canPaste() const;
    bool canDelete() const;
    void cut();
    void copy();
    void paste();
    void deleteSelected();

    // Navigation
    void gotoLine();
    void find();
    void replace();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    void setStatusText(const QString& text);

signals:
    void fileOpened(const QString& path);
    void fileClosed(const QString& path);
    void fileSaved(const QString& path);
    void statusChanged(const QString& text);

private slots:
    void onTabChanged(int index);
    void onTabCloseRequested(int index);
    void onDocumentModified();
    void onFileBrowserDoubleClick(const QString& filePath);
    void onSearchNavigate(const QString& filePath, int line);

private:
    void setupUI();
    void setupToolbar(QBoxLayout* mainLayout);
    int findTabByPath(const QString& path) const;
    CodeEditor* editorAt(int index) const;
    void updateTitle(int index);

    QSplitter* m_splitter = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QToolBar* m_toolbar = nullptr;
    QLabel* m_statusLabel = nullptr;

    IdeEditorFileBrowser* m_fileBrowser = nullptr;
    IdeEditorSearchPanel* m_searchPanel = nullptr;
    FindReplaceDialog* m_findDialog = nullptr;

    QMap<int, QString> m_tabPaths;
    QMap<int, bool> m_tabModified;

    int m_currentFontSize = 13;
    static constexpr int kMinZoom = 8;
    static constexpr int kMaxZoom = 40;
    static constexpr int kBaseFontSize = 13;
};

} // namespace ks
