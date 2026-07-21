#include "ToolsEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMenu>
#include <QAction>
#include <QProgressBar>

namespace ks {
namespace tools {

ToolsEditorModule::ToolsEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_toolsListTab(nullptr)
    , m_toolTree(nullptr)
    , m_toolDescriptionLabel(nullptr)
    , m_launchToolBtn(nullptr)
    , m_batchTab(nullptr)
    , m_batchQueueTable(nullptr)
    , m_batchProcessBtn(nullptr)
    , m_addBatchItemBtn(nullptr)
    , m_removeBatchItemBtn(nullptr)
    , m_batchProgress(nullptr)
    , m_batchStatusLabel(nullptr)
    , m_lodTab(nullptr)
    , m_lodErrorSpin(nullptr)
    , m_lodTargetCountSpin(nullptr)
    , m_generateLODBtn(nullptr)
    , m_lodProgress(nullptr)
    , m_collisionTab(nullptr)
    , m_collisionMethodCombo(nullptr)
    , m_collisionMaxHullsSpin(nullptr)
    , m_collisionPrecisionSpin(nullptr)
    , m_generateCollisionBtn(nullptr)
    , m_collisionProgress(nullptr)
    , m_backupTab(nullptr)
    , m_backupBtn(nullptr)
    , m_restoreBtn(nullptr)
    , m_backupTree(nullptr)
    , m_macrosTab(nullptr)
    , m_recordBtn(nullptr)
    , m_playBtn(nullptr)
    , m_openPythonBtn(nullptr)
    , m_macroList(nullptr)
{
    setObjectName("ToolsEditorModule");
}

bool ToolsEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void ToolsEditorModule::shutdown() {
    m_uiBuilt = false;
}

void ToolsEditorModule::onActivation() {}
void ToolsEditorModule::onDeactivation() {}

void ToolsEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupToolsListTab();
    setupBatchTab();
    setupLODTab();
    setupCollisionTab();
    setupBackupTab();
    setupMacrosTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void ToolsEditorModule::setupToolsListTab() {
    m_toolsListTab = new QWidget();
    auto* layout = new QVBoxLayout(m_toolsListTab);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_toolTree = createTreeWidget({"Tool", "Category", "Status"});
    splitter->addWidget(m_toolTree);

    auto* infoPanel = new QWidget();
    auto* infoLayout = new QVBoxLayout(infoPanel);
    m_toolDescriptionLabel = createLabel("Select a tool to view description");
    m_launchToolBtn = createButton("Launch Tool");
    infoLayout->addWidget(m_toolDescriptionLabel);
    infoLayout->addWidget(m_launchToolBtn);
    infoLayout->addStretch();
    splitter->addWidget(infoPanel);

    layout->addWidget(splitter);

    connect(m_toolTree, &QTreeWidget::itemClicked, this, &ToolsEditorModule::onToolSelected);
    connect(m_launchToolBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_toolTree->currentItem();
        if (item) log(QString("Launching tool: %1").arg(item->text(0)));
    });

    populateToolsList();
    m_tabWidget->addTab(m_toolsListTab, "Tool Library");
}

void ToolsEditorModule::setupBatchTab() {
    m_batchTab = new QWidget();
    auto* layout = new QVBoxLayout(m_batchTab);

    auto* btnLayout = new QHBoxLayout();
    m_batchProcessBtn = createButton("Process All");
    m_addBatchItemBtn = createButton("Add Items");
    m_removeBatchItemBtn = createButton("Remove");
    btnLayout->addWidget(m_batchProcessBtn);
    btnLayout->addWidget(m_addBatchItemBtn);
    btnLayout->addWidget(m_removeBatchItemBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_batchQueueTable = new QTableWidget(0, 4);
    m_batchQueueTable->setHorizontalHeaderLabels({"Item", "Tool", "Status", "Progress"});
    m_batchQueueTable->horizontalHeader()->setStretchLastSection(true);
    m_batchQueueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_batchQueueTable->setAlternatingRowColors(true);
    layout->addWidget(m_batchQueueTable);

    m_batchProgress = new QProgressBar();
    m_batchProgress->setVisible(false);
    layout->addWidget(m_batchProgress);

    m_batchStatusLabel = createLabel("");
    layout->addWidget(m_batchStatusLabel);

    connect(m_batchProcessBtn, &QPushButton::clicked, this, &ToolsEditorModule::onBatchProcess);
    connect(m_addBatchItemBtn, &QPushButton::clicked, this, [this]() {
        QStringList files = selectFiles("Add Items to Batch", "All Supported (*.kn5 *.fbx *.gltf *.glb *.obj *.dae);;All Files (*)");
        for (const auto& f : files) {
            int row = m_batchQueueTable->rowCount();
            m_batchQueueTable->insertRow(row);
            m_batchQueueTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(f).fileName()));
            m_batchQueueTable->setItem(row, 1, new QTableWidgetItem("Auto"));
            m_batchQueueTable->setItem(row, 2, new QTableWidgetItem("Pending"));
            m_batchQueueTable->setItem(row, 3, new QTableWidgetItem("0%"));
        }
    });
    connect(m_removeBatchItemBtn, &QPushButton::clicked, this, [this]() {
        int row = m_batchQueueTable->currentRow();
        if (row >= 0) m_batchQueueTable->removeRow(row);
    });

    m_tabWidget->addTab(m_batchTab, "Batch Processing");
}

void ToolsEditorModule::setupLODTab() {
    m_lodTab = new QWidget();
    auto* layout = new QVBoxLayout(m_lodTab);

    auto* params = createGroupBox("LOD Generation");
    auto* form = new QFormLayout(params);
    m_lodErrorSpin = createDoubleSpinBox(0.001, 1.0, 0.05, 3, "");
    m_lodTargetCountSpin = createSpinBox(1, 10, 3, " LODs");
    form->addRow("Max Error:", m_lodErrorSpin);
    form->addRow("Target LOD Count:", m_lodTargetCountSpin);

    auto* btnLayout = new QHBoxLayout();
    m_generateLODBtn = createButton("Generate LODs");
    btnLayout->addWidget(m_generateLODBtn);
    btnLayout->addStretch();
    form->addRow("", btnLayout);

    layout->addWidget(params);

    m_lodProgress = new QProgressBar();
    m_lodProgress->setVisible(false);
    layout->addWidget(m_lodProgress);

    layout->addStretch();

    connect(m_generateLODBtn, &QPushButton::clicked, this, &ToolsEditorModule::onGenerateLOD);

    m_tabWidget->addTab(m_lodTab, "LOD Generator");
}

void ToolsEditorModule::setupCollisionTab() {
    m_collisionTab = new QWidget();
    auto* layout = new QVBoxLayout(m_collisionTab);

    auto* params = createGroupBox("Collision Mesh Generation");
    auto* form = new QFormLayout(params);
    m_collisionMethodCombo = createComboBox({"V-HACD (Convex Decomposition)", "Primitive Fitting (Box/Sphere/Capsule)", "Convex Hull"});
    m_collisionMaxHullsSpin = createSpinBox(1, 64, 16, " hulls");
    m_collisionPrecisionSpin = createDoubleSpinBox(0.001, 1.0, 0.01, 3, "");
    form->addRow("Method:", m_collisionMethodCombo);
    form->addRow("Max Hulls:", m_collisionMaxHullsSpin);
    form->addRow("Precision:", m_collisionPrecisionSpin);

    auto* btnLayout = new QHBoxLayout();
    m_generateCollisionBtn = createButton("Generate Collision");
    btnLayout->addWidget(m_generateCollisionBtn);
    btnLayout->addStretch();
    form->addRow("", btnLayout);

    layout->addWidget(params);

    m_collisionProgress = new QProgressBar();
    m_collisionProgress->setVisible(false);
    layout->addWidget(m_collisionProgress);

    layout->addStretch();

    connect(m_generateCollisionBtn, &QPushButton::clicked, this, &ToolsEditorModule::onGenerateCollision);

    m_tabWidget->addTab(m_collisionTab, "Collision Mesh");
}

void ToolsEditorModule::setupBackupTab() {
    m_backupTab = new QWidget();
    auto* layout = new QVBoxLayout(m_backupTab);

    auto* btnLayout = new QHBoxLayout();
    m_backupBtn = createButton("Backup Now");
    m_restoreBtn = createButton("Restore from Backup");
    btnLayout->addWidget(m_backupBtn);
    btnLayout->addWidget(m_restoreBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_backupTree = createTreeWidget({"Backup", "Date", "Size", "Type"});
    m_backupTree->addTopLevelItem(new QTreeWidgetItem({"Auto-backup 001", "2024-01-15 10:00", "1.2 GB", "Auto"}));
    m_backupTree->addTopLevelItem(new QTreeWidgetItem({"Before LOD Gen", "2024-01-14 15:30", "980 MB", "Manual"}));
    m_backupTree->addTopLevelItem(new QTreeWidgetItem({"End of Day", "2024-01-13 18:00", "1.1 GB", "Scheduled"}));
    layout->addWidget(m_backupTree);

    connect(m_backupBtn, &QPushButton::clicked, this, &ToolsEditorModule::onBackupNow);
    connect(m_restoreBtn, &QPushButton::clicked, this, &ToolsEditorModule::onRestoreBackup);

    m_tabWidget->addTab(m_backupTab, "Backup");
}

void ToolsEditorModule::setupMacrosTab() {
    m_macrosTab = new QWidget();
    auto* layout = new QVBoxLayout(m_macrosTab);

    auto* btnLayout = new QHBoxLayout();
    m_recordBtn = createButton("Record Macro");
    m_playBtn = createButton("Play Macro");
    m_openPythonBtn = createButton("Python Console");
    btnLayout->addWidget(m_recordBtn);
    btnLayout->addWidget(m_playBtn);
    btnLayout->addWidget(m_openPythonBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_macroList = new QListWidget();
    m_macroList->addItem("Batch Export All");
    m_macroList->addItem("Generate LODs + Collision");
    m_macroList->addItem("Validate Project");
    m_macroList->addItem("Publish to Workshop");
    layout->addWidget(m_macroList);

    connect(m_recordBtn, &QPushButton::clicked, this, &ToolsEditorModule::onMacroRecord);
    connect(m_playBtn, &QPushButton::clicked, this, &ToolsEditorModule::onMacroPlay);
    connect(m_openPythonBtn, &QPushButton::clicked, this, &ToolsEditorModule::onOpenPythonConsole);

    m_tabWidget->addTab(m_macrosTab, "Macros & Scripting");
}

void ToolsEditorModule::populateToolsList() {
    m_toolTree->clear();

    auto* generation = new QTreeWidgetItem(m_toolTree, {"Mesh Generation", "", ""});
    generation->addChild(new QTreeWidgetItem({"LOD Generator", "Mesh", "Ready"}));
    generation->addChild(new QTreeWidgetItem({"Collision Mesh Generator", "Mesh", "Ready"}));
    generation->addChild(new QTreeWidgetItem({"UV Unwrapper", "Mesh", "Ready"}));

    auto* validation = new QTreeWidgetItem(m_toolTree, {"Validation", "", ""});
    validation->addChild(new QTreeWidgetItem({"Asset Validator", "QA", "Ready"}));
    validation->addChild(new QTreeWidgetItem({"Format Validator", "QA", "Ready"}));
    validation->addChild(new QTreeWidgetItem({"Dependency Scanner", "QA", "Ready"}));

    auto* conversion = new QTreeWidgetItem(m_toolTree, {"Conversion", "", ""});
    conversion->addChild(new QTreeWidgetItem({"Batch Converter", "Format", "Ready"}));
    conversion->addChild(new QTreeWidgetItem({"Export Presets", "Format", "Ready"}));

    auto* utilities = new QTreeWidgetItem(m_toolTree, {"Utilities", "", ""});
    utilities->addChild(new QTreeWidgetItem({"Backup Manager", "System", "Ready"}));
    utilities->addChild(new QTreeWidgetItem({"Macro Recorder", "Automation", "Ready"}));
    utilities->addChild(new QTreeWidgetItem({"Performance Optimizer", "System", "Ready"}));
    utilities->addChild(new QTreeWidgetItem({"Theme Editor", "UI", "Ready"}));

    m_toolTree->expandAll();
}

void ToolsEditorModule::onToolSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item && item->childCount() == 0) {
        m_toolDescriptionLabel->setText(QString("Tool: %1\nCategory: %2\nStatus: %3").arg(item->text(0), item->text(1), item->text(2)));
    }
}

void ToolsEditorModule::onBatchProcess() {
    if (m_batchQueueTable->rowCount() == 0) {
        logError("No items in batch queue");
        return;
    }
    log(QString("Starting batch processing of %1 items").arg(m_batchQueueTable->rowCount()));
    m_batchProgress->setVisible(true);
    m_batchProgress->setRange(0, m_batchQueueTable->rowCount());
    m_batchStatusLabel->setText("Processing...");
}

void ToolsEditorModule::onGenerateLOD() {
    log(QString("Generating LODs with max error %1, %2 levels").arg(m_lodErrorSpin->value(), 0, 'f', 3).arg(m_lodTargetCountSpin->value()));
    m_lodProgress->setVisible(true);
    m_lodProgress->setRange(0, 0);
}

void ToolsEditorModule::onGenerateCollision() {
    log(QString("Generating collision mesh using %1").arg(m_collisionMethodCombo->currentText()));
    m_collisionProgress->setVisible(true);
    m_collisionProgress->setRange(0, 0);
}

void ToolsEditorModule::onValidateAssets() {
    log("Starting asset validation...");
}

void ToolsEditorModule::onBackupNow() {
    log("Creating backup...");
}

void ToolsEditorModule::onRestoreBackup() {
    auto* item = m_backupTree->currentItem();
    if (item) {
        if (confirmAction("Restore Backup", QString("Restore '%1'? Current data will be overwritten.").arg(item->text(0)))) {
            log(QString("Restoring from backup: %1").arg(item->text(0)));
        }
    }
}

void ToolsEditorModule::onMacroRecord() {
    log("Starting macro recording...");
}

void ToolsEditorModule::onMacroPlay() {
    auto* item = m_macroList->currentItem();
    if (item) {
        log(QString("Playing macro: %1").arg(item->text()));
    }
}

void ToolsEditorModule::onOpenPythonConsole() {
    log("Opening Python console...");
}

} // namespace tools
} // namespace ks

#include "ToolsEditorModule.moc"
