#pragma once

#include "core/editor/ModuleGuiBase.h"
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QGroupBox>
#include <QFormLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QProgressBar>

namespace ks {
namespace assets {

class AssetEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit AssetEditorModule(QWidget* parent = nullptr);
    ~AssetEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Asset Manager"; }
    QString moduleId() const override { return "assets"; }
    int getModulePriority() const override { return 10; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onAssetSelected(QTreeWidgetItem* item, int column);
    void onSearchTextChanged(const QString& text);
    void onFilterChanged(int index);
    void onImportAsset();
    void onExportAsset();
    void onDeleteAsset();
    void onRefreshAssets();
    void onPreviewAsset();
    void onShowDependencies();
    void onTagFilterChanged(int index);

private:
    void setupBrowserTab();
    void setupSearchTab();
    void setupDependenciesTab();
    void setupCloudSyncTab();
    void populateAssetTree();
    void populateDependencies();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_browserTab = nullptr;
    QTreeWidget* m_assetTree = nullptr;
    QLabel* m_assetInfoLabel = nullptr;
    QLabel* m_assetPreviewLabel = nullptr;
    QLineEdit* m_searchInput = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QComboBox* m_tagFilterCombo = nullptr;
    QPushButton* m_importBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_previewBtn = nullptr;

    QWidget* m_searchTab = nullptr;
    QTableWidget* m_searchResults = nullptr;
    QProgressBar* m_searchProgress = nullptr;

    QWidget* m_dependenciesTab = nullptr;
    QTreeWidget* m_depTree = nullptr;
    QPushButton* m_showDepsBtn = nullptr;

    QWidget* m_cloudSyncTab = nullptr;
    QPushButton* m_syncNowBtn = nullptr;
    QPushButton* m_configureSyncBtn = nullptr;
    QLabel* m_syncStatusLabel = nullptr;
    QProgressBar* m_syncProgress = nullptr;
};

} // namespace assets
} // namespace ks
