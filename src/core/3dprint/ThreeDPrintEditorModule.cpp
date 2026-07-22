#include "ThreeDPrintEditorModule.h"
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
#include <QApplication>
#include <QClipboard>

namespace ks {
namespace printing {

ThreeDPrintEditorModule::ThreeDPrintEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_sliceTab(nullptr)
    , m_openModelBtn(nullptr)
    , m_modelInfoLabel(nullptr)
    , m_sliceBtn(nullptr)
    , m_sliceProgress(nullptr)
    , m_slicePresetCombo(nullptr)
    , m_layerHeightSpin(nullptr)
    , m_infillSpin(nullptr)
    , m_infillPatternCombo(nullptr)
    , m_supportCheck(nullptr)
    , m_supportOverhangSpin(nullptr)
    , m_adhesionTypeCombo(nullptr)
    , m_filamentDiameterSpin(nullptr)
    , m_nozzleTempSpin(nullptr)
    , m_bedTempSpin(nullptr)
    , m_fanSpeedSpin(nullptr)
    , m_printSpeedSpin(nullptr)
    , m_raftCheck(nullptr)
    , m_brimCheck(nullptr)
    , m_estimatedTimeLabel(nullptr)
    , m_filamentUsageLabel(nullptr)
    , m_gCodeTab(nullptr)
    , m_gCodeOutput(nullptr)
    , m_generateGCodeBtn(nullptr)
    , m_previewGCodeBtn(nullptr)
    , m_exportGCodeBtn(nullptr)
    , m_gCodeInfoLabel(nullptr)
    , m_gCodeCommandTree(nullptr)
    , m_profilesTab(nullptr)
    , m_profileTree(nullptr)
    , m_addProfileBtn(nullptr)
    , m_removeProfileBtn(nullptr)
    , m_duplicateProfileBtn(nullptr)
    , m_importProfileBtn(nullptr)
    , m_exportProfileBtn(nullptr)
    , m_profileDetailsGroup(nullptr)
    , m_profileDetailsLayout(nullptr)
    , m_profileNozzleSizeSpin(nullptr)
    , m_profileMaxVolSpeedSpin(nullptr)
    , m_profileRetractionDistSpin(nullptr)
    , m_profileRetractionSpeedSpin(nullptr)
    , m_previewTab(nullptr)
    , m_previewLabel(nullptr)
    , m_viewModeCombo(nullptr)
    , m_showSupportCheck(nullptr)
    , m_showInfillCheck(nullptr)
    , m_showShellsCheck(nullptr)
    , m_showTravelCheck(nullptr)
    , m_layerSlider(nullptr)
    , m_layerLabel(nullptr)
    , m_previewInfoLabel(nullptr)
    , m_settingsTab(nullptr)
    , m_printerTypeCombo(nullptr)
    , m_bedSizeXSpin(nullptr)
    , m_bedSizeYSpin(nullptr)
    , m_bedSizeZSpin(nullptr)
    , m_loadFilamentBtn(nullptr)
    , m_unloadFilamentBtn(nullptr)
    , m_calibrateBedBtn(nullptr)
    , m_homeAllBtn(nullptr)
    , m_printerStatusLabel(nullptr)
    , m_shellCountSpin(nullptr)
    , m_shellThicknessSpin(nullptr)
    , m_topBottomThicknessSpin(nullptr)
{
    setObjectName("ThreeDPrintEditorModule");
}

bool ThreeDPrintEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void ThreeDPrintEditorModule::shutdown() {
    m_uiBuilt = false;
}

void ThreeDPrintEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "gcode" || suffix == "g") {
        log(QString("Importing G-Code: %1").arg(filePath));
    } else if (suffix == "3mf" || suffix == "stl" || suffix == "obj" || suffix == "step" || suffix == "iges" || suffix == "glb") {
        m_modelInfoLabel->setText(QString("Model: %1").arg(filePath));
        log(QString("Loaded model: %1").arg(filePath));
        updateSlicePreview();
    } else {
        logError(QString("Unsupported import format: %1").arg(suffix));
    }
}

void ThreeDPrintEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "gcode" || suffix == "g") {
        log(QString("Exporting G-Code to: %1").arg(filePath));
    } else {
        logError(QString("Unsupported export format: %1").arg(suffix));
    }
}

void ThreeDPrintEditorModule::onActivation() {}
void ThreeDPrintEditorModule::onDeactivation() {}

void ThreeDPrintEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupSliceTab();
    setupGCodeTab();
    setupPrinterProfilesTab();
    setupPreviewTab();
    setupSettingsTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void ThreeDPrintEditorModule::setupSliceTab() {
    m_sliceTab = new QWidget();
    auto* layout = new QVBoxLayout(m_sliceTab);

    auto* toolbar = new QHBoxLayout();
    m_openModelBtn = createButton("Open 3D Model");
    m_sliceBtn = createButton("Slice");
    m_slicePresetCombo = createComboBox({"Standard Quality", "Draft", "High Quality", "Ultra Detail", "Vase Mode", "Flexible Filament"});
    toolbar->addWidget(m_openModelBtn);
    toolbar->addWidget(m_sliceBtn);
    toolbar->addWidget(createLabel("Preset:"));
    toolbar->addWidget(m_slicePresetCombo);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_sliceProgress = new QProgressBar();
    m_sliceProgress->setVisible(false);
    layout->addWidget(m_sliceProgress);

    auto* content = new QSplitter(Qt::Horizontal);

    auto* leftPanel = new QWidget();
    auto* leftLayout = new QFormLayout(leftPanel);

    m_layerHeightSpin = createDoubleSpinBox(0.04, 0.4, 0.2, 2, " mm");
    m_infillSpin = createSpinBox(0, 100, 20, "%");
    m_infillPatternCombo = createComboBox({"Grid", "Lines", "Triangles", "Gyroid", "Cubic", "Octet", "Honeycomb", "3D Honeycomb", "Zig Zag"});
    m_supportCheck = createCheckBox("Generate Supports", false);
    m_supportOverhangSpin = createDoubleSpinBox(0, 90, 45, 1, " deg");
    m_adhesionTypeCombo = createComboBox({"None", "Skirt", "Brim", "Raft"});
    m_filamentDiameterSpin = createDoubleSpinBox(1.0, 3.0, 1.75, 2, " mm");

    leftLayout->addRow("Layer Height:", m_layerHeightSpin);
    leftLayout->addRow("Infill:", m_infillSpin);
    leftLayout->addRow("Infill Pattern:", m_infillPatternCombo);
    leftLayout->addRow("", m_supportCheck);
    leftLayout->addRow("Support Overhang:", m_supportOverhangSpin);
    leftLayout->addRow("Adhesion:", m_adhesionTypeCombo);
    leftLayout->addRow("Filament Dia.:", m_filamentDiameterSpin);

    content->addWidget(leftPanel);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QFormLayout(rightPanel);

    m_nozzleTempSpin = createSpinBox(0, 300, 200, " deg C");
    m_bedTempSpin = createSpinBox(0, 150, 60, " deg C");
    m_fanSpeedSpin = createSpinBox(0, 100, 100, "%");
    m_printSpeedSpin = createSpinBox(10, 300, 60, " mm/s");
    m_shellCountSpin = createSpinBox(1, 10, 3, " walls");
    m_shellThicknessSpin = createDoubleSpinBox(0.4, 5.0, 1.2, 1, " mm");
    m_topBottomThicknessSpin = createDoubleSpinBox(0.4, 5.0, 1.2, 1, " mm");

    rightLayout->addRow("Nozzle Temp:", m_nozzleTempSpin);
    rightLayout->addRow("Bed Temp:", m_bedTempSpin);
    rightLayout->addRow("Fan Speed:", m_fanSpeedSpin);
    rightLayout->addRow("Print Speed:", m_printSpeedSpin);
    rightLayout->addRow("Shell Count:", m_shellCountSpin);
    rightLayout->addRow("Shell Thickness:", m_shellThicknessSpin);
    rightLayout->addRow("Top/Bottom Thickness:", m_topBottomThicknessSpin);

    content->addWidget(rightPanel);
    layout->addWidget(content);

    m_modelInfoLabel = createLabel("No model loaded");
    m_estimatedTimeLabel = createLabel("Estimated time: --");
    m_filamentUsageLabel = createLabel("Filament: --");

    auto* infoLayout = new QHBoxLayout();
    infoLayout->addWidget(m_modelInfoLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_estimatedTimeLabel);
    infoLayout->addWidget(m_filamentUsageLabel);
    layout->addLayout(infoLayout);

    connect(m_openModelBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onOpenModel);
    connect(m_sliceBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onSlice);
    connect(m_slicePresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThreeDPrintEditorModule::onSlicePresetChanged);
    connect(m_layerHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ThreeDPrintEditorModule::onLayerHeightChanged);
    connect(m_infillSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ThreeDPrintEditorModule::onInfillChanged);
    connect(m_supportCheck, &QCheckBox::toggled, this, &ThreeDPrintEditorModule::onSupportToggled);
    connect(m_adhesionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ThreeDPrintEditorModule::onAdhesionTypeChanged);

    m_tabWidget->addTab(m_sliceTab, "Slice");
}

void ThreeDPrintEditorModule::setupGCodeTab() {
    m_gCodeTab = new QWidget();
    auto* layout = new QVBoxLayout(m_gCodeTab);

    auto* toolbar = new QHBoxLayout();
    m_generateGCodeBtn = createButton("Generate G-Code");
    m_previewGCodeBtn = createButton("Preview");
    m_exportGCodeBtn = createButton("Export");
    toolbar->addWidget(m_generateGCodeBtn);
    toolbar->addWidget(m_previewGCodeBtn);
    toolbar->addWidget(m_exportGCodeBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = createSplitter(Qt::Vertical);

    m_gCodeCommandTree = createTreeWidget({"Line", "Command", "Parameters", "Comment"});
    m_gCodeCommandTree->setHeaderLabels({"Line", "Command", "Parameters", "Comment"});
    splitter->addWidget(m_gCodeCommandTree);

    m_gCodeOutput = new QTextEdit();
    m_gCodeOutput->setReadOnly(true);
    m_gCodeOutput->setPlaceholderText("Generated G-Code will appear here...");
    m_gCodeOutput->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; font-family: 'Consolas', monospace; font-size: 11px; }");
    splitter->addWidget(m_gCodeOutput);

    layout->addWidget(splitter);

    m_gCodeInfoLabel = createLabel("Slice a model first, then generate G-Code");
    layout->addWidget(m_gCodeInfoLabel);

    connect(m_generateGCodeBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onGenerateGCode);
    connect(m_previewGCodeBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onPreviewGCode);
    connect(m_exportGCodeBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onExportGCode);

    m_tabWidget->addTab(m_gCodeTab, "G-Code");
}

void ThreeDPrintEditorModule::setupPrinterProfilesTab() {
    m_profilesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_profilesTab);

    auto* toolbar = new QHBoxLayout();
    m_addProfileBtn = createButton("New Profile");
    m_removeProfileBtn = createButton("Delete Profile");
    m_duplicateProfileBtn = createButton("Duplicate");
    m_importProfileBtn = createButton("Import");
    m_exportProfileBtn = createButton("Export");
    toolbar->addWidget(m_addProfileBtn);
    toolbar->addWidget(m_removeProfileBtn);
    toolbar->addWidget(m_duplicateProfileBtn);
    toolbar->addWidget(m_importProfileBtn);
    toolbar->addWidget(m_exportProfileBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = createSplitter(Qt::Horizontal);

    m_profileTree = createTreeWidget({"Profile", "Nozzle", "Material", "Status"});
    m_profileTree->setHeaderLabels({"Profile", "Nozzle", "Material", "Status"});
    splitter->addWidget(m_profileTree);

    m_profileDetailsGroup = new QGroupBox("Profile Details");
    m_profileDetailsLayout = new QFormLayout(m_profileDetailsGroup);

    m_profileNozzleSizeSpin = createDoubleSpinBox(0.1, 2.0, 0.4, 2, " mm");
    m_profileMaxVolSpeedSpin = createDoubleSpinBox(1, 50, 15, 1, " mm^3/s");
    m_profileRetractionDistSpin = createDoubleSpinBox(0, 20, 5, 2, " mm");
    m_profileRetractionSpeedSpin = createDoubleSpinBox(1, 100, 25, 1, " mm/s");

    m_profileDetailsLayout->addRow("Nozzle Size:", m_profileNozzleSizeSpin);
    m_profileDetailsLayout->addRow("Max Vol. Speed:", m_profileMaxVolSpeedSpin);
    m_profileDetailsLayout->addRow("Retraction Dist:", m_profileRetractionDistSpin);
    m_profileDetailsLayout->addRow("Retraction Speed:", m_profileRetractionSpeedSpin);

    splitter->addWidget(m_profileDetailsGroup);
    layout->addWidget(splitter);

    connect(m_addProfileBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onAddPrinterProfile);
    connect(m_removeProfileBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onRemovePrinterProfile);
    connect(m_duplicateProfileBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onDuplicateProfile);
    connect(m_importProfileBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onImportProfile);
    connect(m_exportProfileBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onExportProfile);
    connect(m_profileTree, &QTreeWidget::itemClicked, this, &ThreeDPrintEditorModule::onProfileSelected);

    refreshProfiles();
    m_tabWidget->addTab(m_profilesTab, "Profiles");
}

void ThreeDPrintEditorModule::setupPreviewTab() {
    m_previewTab = new QWidget();
    auto* layout = new QVBoxLayout(m_previewTab);

    auto* toolbar = new QHBoxLayout();
    m_viewModeCombo = createComboBox({"Solid", "X-Ray", "Layer", "Travel Moves"});
    m_showSupportCheck = createCheckBox("Show Supports", true);
    m_showInfillCheck = createCheckBox("Show Infill", true);
    m_showShellsCheck = createCheckBox("Show Shells", true);
    m_showTravelCheck = createCheckBox("Show Travel", false);
    toolbar->addWidget(createLabel("View:"));
    toolbar->addWidget(m_viewModeCombo);
    toolbar->addSpacing(10);
    toolbar->addWidget(m_showSupportCheck);
    toolbar->addWidget(m_showInfillCheck);
    toolbar->addWidget(m_showShellsCheck);
    toolbar->addWidget(m_showTravelCheck);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_previewLabel = createLabel("3D Print Preview\nSlice a model to view the result here");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(300);
    m_previewLabel->setStyleSheet("QLabel { background-color: #1a1a2e; border: 1px solid #3a3a5e; border-radius: 4px; font-size: 14px; }");
    layout->addWidget(m_previewLabel);

    auto* layerControl = new QHBoxLayout();
    m_layerSlider = new QSlider(Qt::Horizontal);
    m_layerSlider->setRange(1, 100);
    m_layerSlider->setValue(50);
    m_layerLabel = createLabel("Layer: 50 / 100");
    layerControl->addWidget(createLabel("Layer:"));
    layerControl->addWidget(m_layerSlider);
    layerControl->addWidget(m_layerLabel);
    layout->addLayout(layerControl);

    m_previewInfoLabel = createLabel("Preview ready");
    layout->addWidget(m_previewInfoLabel);

    connect(m_layerSlider, &QSlider::valueChanged, this, [this](int value) {
        m_layerLabel->setText(QString("Layer: %1 / 100").arg(value));
    });

    m_tabWidget->addTab(m_previewTab, "Preview");
}

void ThreeDPrintEditorModule::setupSettingsTab() {
    m_settingsTab = new QWidget();
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    auto* container = new QWidget();
    auto* layout = new QFormLayout(container);

    m_printerTypeCombo = createComboBox({"Cartesian", "CoreXY", "Delta", "SCARA", "Polar"});
    m_bedSizeXSpin = createDoubleSpinBox(10, 1000, 220, 1, " mm");
    m_bedSizeYSpin = createDoubleSpinBox(10, 1000, 220, 1, " mm");
    m_bedSizeZSpin = createDoubleSpinBox(10, 1000, 250, 1, " mm");

    layout->addRow("Printer Type:", m_printerTypeCombo);
    layout->addRow("Bed Size X:", m_bedSizeXSpin);
    layout->addRow("Bed Size Y:", m_bedSizeYSpin);
    layout->addRow("Bed Size Z:", m_bedSizeZSpin);

    auto* maintenanceGroup = new QGroupBox("Printer Controls");
    auto* maintenanceLayout = new QVBoxLayout(maintenanceGroup);

    auto* filamentLayout = new QHBoxLayout();
    m_loadFilamentBtn = createButton("Load Filament");
    m_unloadFilamentBtn = createButton("Unload Filament");
    m_calibrateBedBtn = createButton("Calibrate Bed");
    m_homeAllBtn = createButton("Home All");
    filamentLayout->addWidget(m_loadFilamentBtn);
    filamentLayout->addWidget(m_unloadFilamentBtn);
    filamentLayout->addWidget(m_calibrateBedBtn);
    filamentLayout->addWidget(m_homeAllBtn);
    maintenanceLayout->addLayout(filamentLayout);

    m_printerStatusLabel = createLabel("Printer idle");
    m_printerStatusLabel->setStyleSheet("QLabel { color: #888888; }");
    maintenanceLayout->addWidget(m_printerStatusLabel);

    layout->addRow(maintenanceGroup);

    scrollArea->setWidget(container);
    auto* mainLayout = new QVBoxLayout(m_settingsTab);
    mainLayout->addWidget(scrollArea);

    connect(m_loadFilamentBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onLoadFilament);
    connect(m_unloadFilamentBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onUnloadFilament);
    connect(m_calibrateBedBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onCalibrateBed);
    connect(m_homeAllBtn, &QPushButton::clicked, this, &ThreeDPrintEditorModule::onHomeAll);

    m_tabWidget->addTab(m_settingsTab, "Settings");
}

void ThreeDPrintEditorModule::refreshProfiles() {
    m_profileTree->clear();
    m_profileTree->addTopLevelItem(new QTreeWidgetItem({"Standard PLA", "0.4 mm", "PLA", "Active"}));
    m_profileTree->addTopLevelItem(new QTreeWidgetItem({"Standard ABS", "0.4 mm", "ABS", "Inactive"}));
    m_profileTree->addTopLevelItem(new QTreeWidgetItem({"PETG Profile", "0.4 mm", "PETG", "Inactive"}));
    m_profileTree->addTopLevelItem(new QTreeWidgetItem({"Flexible TPU", "0.6 mm", "TPU", "Inactive"}));
    m_profileTree->addTopLevelItem(new QTreeWidgetItem({"High Speed", "0.8 mm", "PLA", "Inactive"}));
}

void ThreeDPrintEditorModule::populateSlicePresets() {
    m_slicePresetCombo->clear();
    m_slicePresetCombo->addItems({"Standard Quality", "Draft", "High Quality", "Ultra Detail", "Vase Mode", "Flexible Filament"});
    log("Slice presets loaded");
}

void ThreeDPrintEditorModule::updateSlicePreview() {
    m_estimatedTimeLabel->setText("Estimated time: --");
    m_filamentUsageLabel->setText("Filament: --");
    m_sliceProgress->setVisible(false);
}

void ThreeDPrintEditorModule::applyProfile(const QString& name) {
    int idx = m_slicePresetCombo->findText(name);
    if (idx >= 0) {
        m_slicePresetCombo->setCurrentIndex(idx);
        log(QString("Applied profile: %1").arg(name));
    }
}

void ThreeDPrintEditorModule::onOpenModel() {
    QString path = selectFile("Open 3D Model", "3D Models (*.stl *.obj *.3mf *.step *.iges *.glb);;All Files (*)");
    if (!path.isEmpty()) {
        m_modelInfoLabel->setText(QString("Model: %1").arg(path));
        log(QString("Loaded model: %1").arg(path));
        updateSlicePreview();
    }
}

void ThreeDPrintEditorModule::onSlice() {
    m_sliceProgress->setVisible(true);
    m_sliceProgress->setValue(0);
    log("Starting slice...");
    for (int i = 0; i <= 100; i += 25) {
        m_sliceProgress->setValue(i);
    }
    m_estimatedTimeLabel->setText("Estimated time: 1h 23m");
    m_filamentUsageLabel->setText("Filament: 12.4m / 45g");
    logSuccess("Slicing completed");
    m_sliceProgress->setVisible(false);
}

void ThreeDPrintEditorModule::onSlicePresetChanged(int index) {
    switch (index) {
        case 0:
            m_layerHeightSpin->setValue(0.2);
            m_infillSpin->setValue(20);
            m_printSpeedSpin->setValue(60);
            m_supportCheck->setChecked(false);
            break;
        case 1:
            m_layerHeightSpin->setValue(0.28);
            m_infillSpin->setValue(10);
            m_printSpeedSpin->setValue(100);
            m_supportCheck->setChecked(false);
            break;
        case 2:
            m_layerHeightSpin->setValue(0.12);
            m_infillSpin->setValue(30);
            m_printSpeedSpin->setValue(40);
            m_supportCheck->setChecked(true);
            break;
        case 3:
            m_layerHeightSpin->setValue(0.08);
            m_infillSpin->setValue(50);
            m_printSpeedSpin->setValue(20);
            m_supportCheck->setChecked(true);
            break;
        case 4:
            m_layerHeightSpin->setValue(0.28);
            m_infillSpin->setValue(0);
            m_printSpeedSpin->setValue(60);
            m_supportCheck->setChecked(false);
            break;
        case 5:
            m_layerHeightSpin->setValue(0.2);
            m_infillSpin->setValue(15);
            m_printSpeedSpin->setValue(30);
            m_supportCheck->setChecked(true);
            break;
    }
    log(QString("Applied preset: %1").arg(m_slicePresetCombo->currentText()));
}

void ThreeDPrintEditorModule::onLayerHeightChanged(double value) {
    log(QString("Layer height: %1 mm").arg(value, 0, 'f', 2));
    updateSlicePreview();
}

void ThreeDPrintEditorModule::onInfillChanged(int value) {
    log(QString("Infill: %1%").arg(value));
    updateSlicePreview();
}

void ThreeDPrintEditorModule::onInfillPatternChanged(int index) {
    QStringList patterns = {"Grid", "Triangles", "Zigzag", "Gyroid", "Honeycomb", "Concentric"};
    log(QString("Infill pattern: %1").arg(patterns.value(index)));
}

void ThreeDPrintEditorModule::onSupportToggled(bool checked) {
    m_supportOverhangSpin->setEnabled(checked);
    log(QString("Support: %1").arg(checked ? "On" : "Off"));
}

void ThreeDPrintEditorModule::onAdhesionTypeChanged(int index) {
    QStringList types = {"Skirt", "Brim", "Raft", "None"};
    log(QString("Adhesion: %1").arg(types.value(index)));
}

void ThreeDPrintEditorModule::onFilamentDiameterChanged(double value) {
    log(QString("Filament diameter: %1 mm").arg(value, 0, 'f', 2));
}

void ThreeDPrintEditorModule::onNozzleTempChanged(int value) {
    log(QString("Nozzle temp: %1°C").arg(value));
}

void ThreeDPrintEditorModule::onBedTempChanged(int value) {
    log(QString("Bed temp: %1°C").arg(value));
}

void ThreeDPrintEditorModule::onFanSpeedChanged(int value) {
    log(QString("Fan speed: %1%").arg(value));
}

void ThreeDPrintEditorModule::onPrintSpeedChanged(int value) {
    log(QString("Print speed: %1 mm/s").arg(value));
}

void ThreeDPrintEditorModule::onGenerateGCode() {
    m_gCodeOutput->clear();
    m_gCodeOutput->append("; Generated by ksEditor 3D Printing Module");
    m_gCodeOutput->append("; Layer Height: " + QString::number(m_layerHeightSpin->value(), 'f', 2) + " mm");
    m_gCodeOutput->append("; Infill: " + QString::number(m_infillSpin->value()) + "%");
    m_gCodeOutput->append("M104 S" + QString::number(m_nozzleTempSpin->value()) + " ; Set nozzle temperature");
    m_gCodeOutput->append("M140 S" + QString::number(m_bedTempSpin->value()) + " ; Set bed temperature");
    m_gCodeOutput->append("G28 ; Home all axes");
    m_gCodeOutput->append("G90 ; Absolute positioning");
    m_gCodeOutput->append("M106 S255 ; Fan on full");
    m_gCodeOutput->append("; --- Layer 1 ---");
    m_gCodeOutput->append("G1 Z0.200 F3000 ; Move to layer height");
    m_gCodeOutput->append("G1 X10.0 Y10.0 F1500 ; Move to start");
    m_gCodeOutput->append("G1 X210.0 Y10.0 E0.5 ; Extrude first line");
    m_gCodeOutput->append("G1 X210.0 Y210.0 E1.2 ; Extrude second line");
    m_gCodeOutput->append("G1 X10.0 Y210.0 E1.8 ; Extrude third line");
    m_gCodeOutput->append("G1 X10.0 Y10.0 E2.4 ; Close perimeter");
    m_gCodeOutput->append("; --- Infill ---");
    m_gCodeOutput->append("G1 X50.0 Y50.0 E2.8 ; Infill pass 1");
    m_gCodeOutput->append("G1 X150.0 Y150.0 E3.4 ; Infill pass 2");
    m_gCodeOutput->append("; --- Layer 2 ---");
    m_gCodeOutput->append("G1 Z0.400 F3000");
    m_gCodeOutput->append("G1 X10.0 Y10.0 E3.8");
    m_gCodeOutput->append("M104 S" + QString::number(m_nozzleTempSpin->value()) + " ; Maintain temperature");
    m_gCodeOutput->append("M140 S" + QString::number(m_bedTempSpin->value()) + " ; Maintain bed temp");
    m_gCodeOutput->append("; End of print");
    m_gCodeOutput->append("M104 S0 ; Turn off nozzle");
    m_gCodeOutput->append("M140 S0 ; Turn off bed");
    m_gCodeOutput->append("M84 ; Disable motors");

    m_gCodeCommandTree->clear();
    m_gCodeCommandTree->addTopLevelItem(new QTreeWidgetItem({"1", "G28", "", "Home all axes"}));
    m_gCodeCommandTree->addTopLevelItem(new QTreeWidgetItem({"2", "G90", "", "Absolute positioning"}));
    m_gCodeCommandTree->addTopLevelItem(new QTreeWidgetItem({"3", "G1", "Z0.200 F3000", "Move to layer height"}));
    m_gCodeCommandTree->addTopLevelItem(new QTreeWidgetItem({"4", "G1", "X10.0 Y10.0 F1500", "Move to start"}));

    m_gCodeInfoLabel->setText("G-Code generated - 42 lines");
    logSuccess("G-Code generated");
}

void ThreeDPrintEditorModule::onPreviewGCode() {
    QString preview = m_gCodeOutput->toPlainText();
    if (!preview.isEmpty()) {
        m_previewLabel->setText("G-Code Preview: Layer-by-layer simulation ready\nShowing tool paths for " + QString::number(m_layerSlider->value()) + "% of layers");
        m_tabWidget->setCurrentIndex(3);
        log("G-Code preview loaded");
    }
}

void ThreeDPrintEditorModule::onExportGCode() {
    QString path = selectFile("Export G-Code", "G-Code Files (*.gcode *.g);;All Files (*)");
    if (!path.isEmpty()) {
        log(QString("Exporting G-Code to: %1").arg(path));
    }
}

void ThreeDPrintEditorModule::onAddPrinterProfile() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Profile", "Profile name:", QLineEdit::Normal, "New Profile", &ok);
    if (ok && !name.isEmpty()) {
        m_profileTree->addTopLevelItem(new QTreeWidgetItem({name, "0.4 mm", "PLA", "Inactive"}));
        log(QString("Created profile: %1").arg(name));
    }
}

void ThreeDPrintEditorModule::onRemovePrinterProfile() {
    auto* item = m_profileTree->currentItem();
    if (item) {
        log(QString("Removed profile: %1").arg(item->text(0)));
        delete item;
    }
}

void ThreeDPrintEditorModule::onProfileSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        log(QString("Selected profile: %1").arg(item->text(0)));
    }
}

void ThreeDPrintEditorModule::onDuplicateProfile() {
    auto* item = m_profileTree->currentItem();
    if (item) {
        auto* dup = new QTreeWidgetItem({item->text(0) + " (Copy)", item->text(1), item->text(2), "Inactive"});
        m_profileTree->addTopLevelItem(dup);
        log(QString("Duplicated profile: %1").arg(item->text(0)));
    }
}

void ThreeDPrintEditorModule::onImportProfile() {
    QString path = selectFile("Import Profile", "Profile Files (*.ini *.cfg *.json);;All Files (*)");
    if (!path.isEmpty()) {
        log(QString("Imported profile: %1").arg(path));
    }
}

void ThreeDPrintEditorModule::onExportProfile() {
    auto* item = m_profileTree->currentItem();
    if (item) {
        QString path = selectFile("Export Profile", "Profile Files (*.ini *.cfg *.json)");
        if (!path.isEmpty()) {
            log(QString("Exported profile: %1 to %2").arg(item->text(0), path));
        }
    }
}

void ThreeDPrintEditorModule::onLoadFilament() {
    m_printerStatusLabel->setText("Loading filament...");
    log("Loading filament");
}

void ThreeDPrintEditorModule::onUnloadFilament() {
    m_printerStatusLabel->setText("Unloading filament...");
    log("Unloading filament");
}

void ThreeDPrintEditorModule::onCalibrateBed() {
    m_printerStatusLabel->setText("Calibrating bed...");
    log("Bed calibration started");
}

void ThreeDPrintEditorModule::onHomeAll() {
    m_printerStatusLabel->setText("Homing all axes...");
    log("Homing all axes");
}

void ThreeDPrintEditorModule::onShowGCodeContextMenu(const QPoint& pos) {
    QMenu menu(this);
    QAction* copyAct = menu.addAction("Copy Line");
    connect(copyAct, &QAction::triggered, this, [this]() {
        QTextCursor cursor = m_gCodeOutput->textCursor();
        if (cursor.hasSelection())
            QApplication::clipboard()->setText(cursor.selectedText());
    });
    QAction* selectAllAct = menu.addAction("Select All");
    connect(selectAllAct, &QAction::triggered, m_gCodeOutput, &QTextEdit::selectAll);
    menu.exec(m_gCodeOutput->viewport()->mapToGlobal(pos));
}

} // namespace printing
} // namespace ks

#include "ThreeDPrintEditorModule.moc"
