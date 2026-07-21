#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QTextBrowser>
#include <QLineEdit>
#include <QComboBox>

namespace ks {
namespace help {

class HelpEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit HelpEditorModule(QWidget* parent = nullptr);
    ~HelpEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Help & Documentation"; }
    QString moduleId() const override { return "help"; }
    int getModulePriority() const override { return 99; }

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onTopicSelected(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onNavigateBack();
    void onNavigateForward();
    void onOpenExternalDocs();

private:
    void setupBrowserTab();
    void setupSearchTab();
    void populateTopicTree();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_browserTab = nullptr;
    QTreeWidget* m_topicTree = nullptr;
    QTextBrowser* m_contentBrowser = nullptr;
    QPushButton* m_backBtn = nullptr;
    QPushButton* m_forwardBtn = nullptr;
    QPushButton* m_externalBtn = nullptr;
    QLabel* m_topicTitleLabel = nullptr;

    QWidget* m_searchTab = nullptr;
    QLineEdit* m_searchInput = nullptr;
    QTreeWidget* m_searchResults = nullptr;
    QComboBox* m_sectionCombo = nullptr;

    QStringList m_history;
    int m_historyIndex = -1;
};

} // namespace help
} // namespace ks
