#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QProgressBar>
#include <QLineEdit>

namespace ks {
namespace modmanager {

class ModManagerEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit ModManagerEditorModule(QWidget* parent = nullptr);
    ~ModManagerEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Mod Manager"; }
    QString moduleId() const override { return "modManager"; }
    int getModulePriority() const override { return 15; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onModSelected(QTreeWidgetItem* item, int column);
    void onInstallMod();
    void onUninstallMod();
    void onEnableMod();
    void onDisableMod();
    void onCheckConflicts();
    void onCreateProfile();
    void onDeleteProfile();
    void onProfileSelected(int index);
    void onVerifyContent();
    void onRepairContent();
    void onRefreshMods();

private:
    void setupModListTab();
    void setupProfilesTab();
    void setupContentRepairTab();
    void populateModTree();
    void populateProfiles();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_modListTab = nullptr;
    QTreeWidget* m_modTree = nullptr;
    QLabel* m_modInfoLabel = nullptr;
    QLabel* m_modVersionLabel = nullptr;
    QLabel* m_modStatusLabel = nullptr;
    QPushButton* m_installBtn = nullptr;
    QPushButton* m_uninstallBtn = nullptr;
    QPushButton* m_enableBtn = nullptr;
    QPushButton* m_disableBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_conflictBtn = nullptr;
    QLineEdit* m_modSearchInput = nullptr;

    QWidget* m_profilesTab = nullptr;
    QComboBox* m_profileCombo = nullptr;
    QPushButton* m_createProfileBtn = nullptr;
    QPushButton* m_deleteProfileBtn = nullptr;
    QTableWidget* m_profileModsTable = nullptr;

    QWidget* m_contentRepairTab = nullptr;
    QPushButton* m_verifyBtn = nullptr;
    QPushButton* m_repairBtn = nullptr;
    QProgressBar* m_repairProgress = nullptr;
    QLabel* m_repairStatusLabel = nullptr;
    QTreeWidget* m_issueTree = nullptr;
};

} // namespace modmanager
} // namespace ks
