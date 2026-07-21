#pragma once

#include "core/editor/ModuleGuiBase.h"
#include "GitStatusWidget.h"
#include "GitManager.h"

#include <QTabWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QListWidget>
#include <QTableWidget>
#include <QDateTime>
#include <QProcess>

namespace ks {

class VcsEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit VcsEditorModule(QWidget* parent = nullptr);

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Version Control"; }
    QString moduleId() const override { return "vcs"; }
    int getModulePriority() const override { return 35; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onRepoPathChanged(const QString& path);
    void onRefreshClicked();
    void onInitRepoClicked();
    void onCloneRepoClicked();
    void onBranchChanged(const QString& branch);
    void onCreateBranchClicked();
    void onDeleteBranchClicked();
    void onCheckoutBranchClicked();
    void onMergeBranchClicked();
    void onCommitClicked();
    void onStageAllClicked();
    void onUnstageAllClicked();
    void onDiscardChangesClicked();
    void onPullClicked();
    void onPushClicked();
    void onFetchClicked();
    void onLogItemClicked(QTreeWidgetItem* item, int column);
    void onRemoteChanged(int index);
    void onAddRemoteClicked();
    void onRemoveRemoteClicked();
    void onStashClicked();
    void onStashPopClicked();
    void onTagClicked();
    void onCreateTagClicked();
    void onShowContextMenu(const QPoint& pos);
    void onFileItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupStatusTab();
    void setupBranchesTab();
    void setupHistoryTab();
    void setupRemotesTab();
    void setupStashesTab();
    void setupTagsTab();
    void setupSettingsTab();
    void refreshAll();
    void loadRepo(const QString& path);
    void updateStatusLabel(const QString& msg, bool success = true);
    QString runGitCommand(const QStringList& args, const QString& workDir = "");
    void populateBranches();
    void populateRemotes();
    void populateStashes();
    void populateTags();
    void populateHistory();
    void loadSettings();
    void saveSettings();

    QTabWidget* m_tabWidget = nullptr;
    
    // Status tab
    QWidget* m_statusTab = nullptr;
    QLineEdit* m_repoPathEdit = nullptr;
    QPushButton* m_browseRepoBtn = nullptr;
    QPushButton* m_initRepoBtn = nullptr;
    QPushButton* m_cloneRepoBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    GitStatusWidget* m_gitWidget = nullptr;
    QLabel* m_repoStatusLabel = nullptr;
    
    // Branches tab
    QWidget* m_branchesTab = nullptr;
    QTreeWidget* m_branchesTree = nullptr;
    QComboBox* m_branchCombo = nullptr;
    QPushButton* m_checkoutBranchBtn = nullptr;
    QPushButton* m_createBranchBtn = nullptr;
    QPushButton* m_deleteBranchBtn = nullptr;
    QPushButton* m_mergeBranchBtn = nullptr;
    QLineEdit* m_newBranchEdit = nullptr;
    
    // History tab
    QWidget* m_historyTab = nullptr;
    QTreeWidget* m_historyTree = nullptr;
    QTextEdit* m_commitDetails = nullptr;
    QSpinBox* m_historyLimitSpin = nullptr;
    
    // Remotes tab
    QWidget* m_remotesTab = nullptr;
    QTreeWidget* m_remotesTree = nullptr;
    QComboBox* m_remoteCombo = nullptr;
    QPushButton* m_pullBtn = nullptr;
    QPushButton* m_pushBtn = nullptr;
    QPushButton* m_fetchBtn = nullptr;
    QPushButton* m_addRemoteBtn = nullptr;
    QPushButton* m_removeRemoteBtn = nullptr;
    
    // Stashes tab
    QWidget* m_stashesTab = nullptr;
    QListWidget* m_stashesList = nullptr;
    QTextEdit* m_stashDetails = nullptr;
    QPushButton* m_stashBtn = nullptr;
    QPushButton* m_stashPopBtn = nullptr;
    QPushButton* m_stashDropBtn = nullptr;
    QPushButton* m_stashApplyBtn = nullptr;
    
    // Tags tab
    QWidget* m_tagsTab = nullptr;
    QTreeWidget* m_tagsTree = nullptr;
    QPushButton* m_createTagBtn = nullptr;
    QPushButton* m_deleteTagBtn = nullptr;
    QPushButton* m_pushTagBtn = nullptr;
    QLineEdit* m_newTagEdit = nullptr;
    
    // Settings tab
    QWidget* m_settingsTab = nullptr;
    QLineEdit* m_userNameEdit = nullptr;
    QLineEdit* m_userEmailEdit = nullptr;
    QCheckBox* m_autoFetchCheck = nullptr;
    QSpinBox* m_fetchIntervalSpin = nullptr;
    QComboBox* m_diffToolCombo = nullptr;
    QComboBox* m_mergeToolCombo = nullptr;

    QString m_currentRepoPath;
    GitManager* m_gitManager = nullptr;
};

} // namespace ks
