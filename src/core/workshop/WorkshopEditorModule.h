#pragma once

#include "core/editor/ModuleGuiBase.h"
#include "WorkshopManager.h"
#include "WorkshopItem.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSplitter>

namespace ks {

class WorkshopEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit WorkshopEditorModule(QWidget* parent = nullptr);
    ~WorkshopEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Workshop"; }
    QString moduleId() const override { return "workshop"; }
    int getModulePriority() const override { return 40; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onRefreshClicked();
    void onCategoryChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemSelectionChanged();
    void onInstallClicked();
    void onUninstallClicked();
    void onUpdateClicked();
    void onPublishClicked();
    void onCreateModClicked();
    void onImportModClicked();
    void onExportModClicked();
    void onRemoveModClicked();
    void onRateClicked();
    void onOpenInBrowserClicked();
    void onResolveDepsClicked();
    void onCheckUpdatesClicked();
    void onProfileChanged(const QString& profile);
    void onCreateProfileClicked();
    void onDeleteProfileClicked();
    void onActivateProfileClicked();
    void onSaveProfileClicked();
    void onShowContextMenu(const QPoint& pos);

private:
    void setupBrowseTab();
    void setupInstalledTab();
    void setupPublishTab();
    void setupProfilesTab();
    void loadItems();
    void populateTree(const QVector<WorkshopItem>& items, QTreeWidget* tree);
    void updateItemDetails(const WorkshopItem* item);
    void updateButtonStates();
    void refreshProfiles();
    QString formatSize(qint64 bytes) const;
    QString formatDateTime(const QDateTime& dt) const;

    WorkshopManager* m_manager = nullptr;

    QTabWidget* m_tabWidget = nullptr;

    // Browse tab
    QWidget* m_browseTab = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QTreeWidget* m_browseTree = nullptr;
    QTextEdit* m_itemDetails = nullptr;
    QPushButton* m_installBtn = nullptr;
    QPushButton* m_updateBtn = nullptr;
    QPushButton* m_openBrowserBtn = nullptr;
    QPushButton* m_resolveDepsBtn = nullptr;
    QPushButton* m_rateBtn = nullptr;

    // Installed tab
    QWidget* m_installedTab = nullptr;
    QTreeWidget* m_installedTree = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
    QPushButton* m_checkUpdatesBtn = nullptr;
    QPushButton* m_openFolderBtn = nullptr;

    // Publish tab
    QWidget* m_publishTab = nullptr;
    QLineEdit* m_pubNameEdit = nullptr;
    QLineEdit* m_pubVersionEdit = nullptr;
    QLineEdit* m_pubAuthorEdit = nullptr;
    QComboBox* m_pubCategoryCombo = nullptr;
    QTextEdit* m_pubDescEdit = nullptr;
    QLineEdit* m_pubTagsEdit = nullptr;
    QLineEdit* m_pubSourcePathEdit = nullptr;
    QPushButton* m_pubBrowseBtn = nullptr;
    QPushButton* m_createModBtn = nullptr;
    QPushButton* m_publishBtn = nullptr;
    QLineEdit* m_pubDepsEdit = nullptr;
    QLineEdit* m_pubConflictsEdit = nullptr;
    QLineEdit* m_pubLicenseEdit = nullptr;
    QLineEdit* m_pubWebsiteEdit = nullptr;

    // Profiles tab
    QWidget* m_profilesTab = nullptr;
    QComboBox* m_profileCombo = nullptr;
    QPushButton* m_createProfileBtn = nullptr;
    QPushButton* m_deleteProfileBtn = nullptr;
    QPushButton* m_activateProfileBtn = nullptr;
    QPushButton* m_saveProfileBtn = nullptr;
    QTreeWidget* m_profileItemsTree = nullptr;
    QTextEdit* m_profileDesc = nullptr;

    QTreeWidgetItem* m_lastSelectedItem = nullptr;
    WorkshopItem m_lastSelectedWorkshopItem;
    bool m_hasSelection = false;
};

}
