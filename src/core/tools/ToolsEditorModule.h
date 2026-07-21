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
#include <QScrollArea>
#include <QProgressBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

namespace ks {
namespace tools {

class ToolsEditorModule : public ModuleGuiBase {
    Q_OBJECT
public:
    explicit ToolsEditorModule(QWidget* parent = nullptr);
    ~ToolsEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;

    QString moduleName() const override { return "Tools & Utilities"; }
    QString moduleId() const override { return "tools"; }
    int getModulePriority() const override { return 25; }

protected:
    void buildUI() override;
    void onActivation() override;
    void onDeactivation() override;

private slots:
    void onBatchProcess();
    void onToolSelected(QTreeWidgetItem* item, int column);
    void onGenerateLOD();
    void onGenerateCollision();
    void onValidateAssets();
    void onBackupNow();
    void onRestoreBackup();
    void onMacroRecord();
    void onMacroPlay();
    void onOpenPythonConsole();

private:
    void setupToolsListTab();
    void setupBatchTab();
    void setupLODTab();
    void setupCollisionTab();
    void setupBackupTab();
    void setupMacrosTab();
    void populateToolsList();

    QTabWidget* m_tabWidget = nullptr;

    QWidget* m_toolsListTab = nullptr;
    QTreeWidget* m_toolTree = nullptr;
    QLabel* m_toolDescriptionLabel = nullptr;
    QPushButton* m_launchToolBtn = nullptr;

    QWidget* m_batchTab = nullptr;
    QTableWidget* m_batchQueueTable = nullptr;
    QPushButton* m_batchProcessBtn = nullptr;
    QPushButton* m_addBatchItemBtn = nullptr;
    QPushButton* m_removeBatchItemBtn = nullptr;
    QProgressBar* m_batchProgress = nullptr;
    QLabel* m_batchStatusLabel = nullptr;

    QWidget* m_lodTab = nullptr;
    QDoubleSpinBox* m_lodErrorSpin = nullptr;
    QSpinBox* m_lodTargetCountSpin = nullptr;
    QPushButton* m_generateLODBtn = nullptr;
    QProgressBar* m_lodProgress = nullptr;

    QWidget* m_collisionTab = nullptr;
    QComboBox* m_collisionMethodCombo = nullptr;
    QSpinBox* m_collisionMaxHullsSpin = nullptr;
    QDoubleSpinBox* m_collisionPrecisionSpin = nullptr;
    QPushButton* m_generateCollisionBtn = nullptr;
    QProgressBar* m_collisionProgress = nullptr;

    QWidget* m_backupTab = nullptr;
    QPushButton* m_backupBtn = nullptr;
    QPushButton* m_restoreBtn = nullptr;
    QTreeWidget* m_backupTree = nullptr;

    QWidget* m_macrosTab = nullptr;
    QPushButton* m_recordBtn = nullptr;
    QPushButton* m_playBtn = nullptr;
    QPushButton* m_openPythonBtn = nullptr;
    QListWidget* m_macroList = nullptr;
};

} // namespace tools
} // namespace ks
