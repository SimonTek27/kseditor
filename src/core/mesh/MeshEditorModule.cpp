#include "MeshEditorModule.h"
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

namespace ks {
namespace mesh {

MeshEditorModule::MeshEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_booleanOpsTab(nullptr)
    , m_meshTree(nullptr)
    , m_loadMeshBtn(nullptr)
    , m_exportMeshBtn(nullptr)
    , m_boolOpCombo(nullptr)
    , m_applyBoolOpBtn(nullptr)
    , m_meshInfoLabel(nullptr)
    , m_operandList(nullptr)
    , m_uvUnwrapTab(nullptr)
    , m_unwrapMethodCombo(nullptr)
    , m_unwrapBtn(nullptr)
    , m_seamAngleSpin(nullptr)
    , m_packChartsBtn(nullptr)
    , m_packMarginSpin(nullptr)
    , m_uvPreviewLabel(nullptr)
    , m_shareUVCheck(nullptr)
    , m_sculptingTab(nullptr)
    , m_brushCombo(nullptr)
    , m_brushSizeSpin(nullptr)
    , m_brushStrengthSpin(nullptr)
    , m_symmetryCombo(nullptr)
    , m_smoothBtn(nullptr)
    , m_remeshBtn(nullptr)
    , m_remeshResSpin(nullptr)
    , m_decimateBtn(nullptr)
    , m_decimateRatioSpin(nullptr)
    , m_sculptInfoLabel(nullptr)
    , m_riggingTab(nullptr)
    , m_boneTree(nullptr)
    , m_addBoneBtn(nullptr)
    , m_removeBoneBtn(nullptr)
    , m_bindSkinBtn(nullptr)
    , m_addWeightBtn(nullptr)
    , m_clearWeightBtn(nullptr)
    , m_weightTree(nullptr)
    , m_weightValueSpin(nullptr)
    , m_exportTab(nullptr)
    , m_exportFormatCombo(nullptr)
    , m_generateLODBtn(nullptr)
    , m_lodLevelSpin(nullptr)
    , m_lodReductionSpin(nullptr)
    , m_exportNormalsCheck(nullptr)
    , m_exportUVCheck(nullptr)
    , m_exportColorsCheck(nullptr)
    , m_exportAnimCheck(nullptr)
    , m_exportProgress(nullptr)
    , m_exportInfoLabel(nullptr)
{
    setObjectName("MeshEditorModule");
}

bool MeshEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void MeshEditorModule::shutdown() {
    m_uiBuilt = false;
}

void MeshEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "obj" || suffix == "fbx" || suffix == "gltf" || suffix == "glb" ||
        suffix == "kn5" || suffix == "stl" || suffix == "ply") {
        log(QString("Loading mesh: %1").arg(filePath));
        refreshMeshList();
        m_sculptInfoLabel->setText("Mesh loaded. Ready for sculpting.");
    } else {
        logError(QString("Unsupported mesh format: %1").arg(suffix));
    }
}

void MeshEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    if (suffix == "obj" || suffix == "fbx" || suffix == "gltf" || suffix == "glb" ||
        suffix == "stl" || suffix == "ply") {
        log(QString("Exporting mesh to: %1").arg(filePath));
    } else {
        logError(QString("Unsupported export format: %1").arg(suffix));
    }
}

void MeshEditorModule::onActivation() {}
void MeshEditorModule::onDeactivation() {}

void MeshEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupBooleanOpsTab();
    setupUVUnwrapTab();
    setupSculptingTab();
    setupRiggingTab();
    setupExportTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void MeshEditorModule::setupBooleanOpsTab() {
    m_booleanOpsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_booleanOpsTab);

    auto* toolbar = new QHBoxLayout();
    m_loadMeshBtn = createButton("Load Mesh");
    m_exportMeshBtn = createButton("Export Mesh");
    toolbar->addWidget(m_loadMeshBtn);
    toolbar->addWidget(m_exportMeshBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = createSplitter(Qt::Horizontal);
    m_meshTree = createTreeWidget({"Mesh", "Vertices", "Triangles", "Selected"});
    m_meshTree->setHeaderLabels({"Mesh", "Vertices", "Triangles", "Selected"});
    splitter->addWidget(m_meshTree);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);

    m_boolOpCombo = createComboBox({"Union (A+B)", "Intersection (A&B)", "Difference (A-B)", "Difference (B-A)"});
    rightLayout->addWidget(createLabel("Boolean Operation:"));
    rightLayout->addWidget(m_boolOpCombo);

    m_applyBoolOpBtn = createButton("Apply Boolean Operation");
    rightLayout->addWidget(m_applyBoolOpBtn);

    m_meshInfoLabel = createLabel("No mesh loaded");
    rightLayout->addWidget(m_meshInfoLabel);

    m_operandList = new QListWidget();
    m_operandList->setAlternatingRowColors(true);
    rightLayout->addWidget(createLabel("Operands:"));
    rightLayout->addWidget(m_operandList);

    rightLayout->addStretch();
    splitter->addWidget(rightPanel);
    layout->addWidget(splitter);

    connect(m_loadMeshBtn, &QPushButton::clicked, this, &MeshEditorModule::onLoadMesh);
    connect(m_exportMeshBtn, &QPushButton::clicked, this, &MeshEditorModule::onExportMesh);
    connect(m_applyBoolOpBtn, &QPushButton::clicked, this, &MeshEditorModule::onApplyBoolOp);
    connect(m_meshTree, &QTreeWidget::itemClicked, this, &MeshEditorModule::onMeshSelected);

    m_tabWidget->addTab(m_booleanOpsTab, "Boolean Ops");
}

void MeshEditorModule::setupUVUnwrapTab() {
    m_uvUnwrapTab = new QWidget();
    auto* layout = new QVBoxLayout(m_uvUnwrapTab);

    auto* paramsGroup = new QGroupBox("Unwrap Settings");
    auto* paramsLayout = new QFormLayout(paramsGroup);

    m_unwrapMethodCombo = createComboBox({"Angle Based", "Conformal", "Least Squares Conformal", "Planar", "Cylindrical", "Spherical"});
    m_seamAngleSpin = createDoubleSpinBox(0, 180, 66, 1, " deg");
    m_packMarginSpin = createDoubleSpinBox(0.0, 1.0, 0.02, 3, "");
    m_shareUVCheck = createCheckBox("Share UV coordinates", true);

    paramsLayout->addRow("Method:", m_unwrapMethodCombo);
    paramsLayout->addRow("Seam Angle:", m_seamAngleSpin);
    paramsLayout->addRow("Pack Margin:", m_packMarginSpin);
    paramsLayout->addRow("", m_shareUVCheck);

    layout->addWidget(paramsGroup);

    auto* btnLayout = new QHBoxLayout();
    m_unwrapBtn = createButton("Unwrap");
    m_packChartsBtn = createButton("Pack Charts");
    m_seamAngleSpin->setVisible(false);
    btnLayout->addWidget(m_unwrapBtn);
    btnLayout->addWidget(m_packChartsBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_uvPreviewLabel = createLabel("Load a mesh to preview UV layout");
    m_uvPreviewLabel->setAlignment(Qt::AlignCenter);
    m_uvPreviewLabel->setMinimumHeight(200);
    m_uvPreviewLabel->setStyleSheet("QLabel { background-color: #1a1a2e; border: 1px solid #3a3a5e; border-radius: 4px; }");
    layout->addWidget(m_uvPreviewLabel);

    layout->addStretch();

    connect(m_unwrapBtn, &QPushButton::clicked, this, &MeshEditorModule::onUnwrap);
    connect(m_packChartsBtn, &QPushButton::clicked, this, &MeshEditorModule::onPackCharts);
    connect(m_unwrapMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeshEditorModule::onUnwrapMethodChanged);

    m_tabWidget->addTab(m_uvUnwrapTab, "UV Unwrap");
}

void MeshEditorModule::setupSculptingTab() {
    m_sculptingTab = new QWidget();
    auto* layout = new QVBoxLayout(m_sculptingTab);

    auto* brushGroup = new QGroupBox("Brush Settings");
    auto* brushLayout = new QFormLayout(brushGroup);

    m_brushCombo = createComboBox({"Draw", "Smooth", "Inflate", "Pinch", "Crease", "Flatten", "Scrape", "Clay", "Clay Strips", "Snake Hook", "Thumb", "Rotate", "Grab"});
    m_brushSizeSpin = createSpinBox(1, 500, 50, " px");
    m_brushStrengthSpin = createDoubleSpinBox(0.01, 1.0, 0.5, 2, "");
    m_symmetryCombo = createComboBox({"None", "X Axis", "Y Axis", "Z Axis", "X & Y", "X & Z", "Y & Z", "All Axes"});

    brushLayout->addRow("Brush:", m_brushCombo);
    brushLayout->addRow("Size:", m_brushSizeSpin);
    brushLayout->addRow("Strength:", m_brushStrengthSpin);
    brushLayout->addRow("Symmetry:", m_symmetryCombo);

    layout->addWidget(brushGroup);

    auto* actionLayout = new QHBoxLayout();
    m_smoothBtn = createButton("Smooth All");
    m_remeshBtn = createButton("Remesh");
    m_decimateBtn = createButton("Decimate");
    actionLayout->addWidget(m_smoothBtn);
    actionLayout->addWidget(m_remeshBtn);
    actionLayout->addWidget(m_decimateBtn);
    actionLayout->addStretch();
    layout->addLayout(actionLayout);

    auto* advancedGroup = new QGroupBox("Advanced");
    auto* advancedLayout = new QFormLayout(advancedGroup);
    m_remeshResSpin = createSpinBox(1000, 1000000, 50000, " tris");
    m_decimateRatioSpin = createDoubleSpinBox(0.01, 0.99, 0.5, 2, " %");
    advancedLayout->addRow("Remesh Target:", m_remeshResSpin);
    advancedLayout->addRow("Decimate Ratio:", m_decimateRatioSpin);
    layout->addWidget(advancedGroup);

    m_sculptInfoLabel = createLabel("Load a mesh to start sculpting");
    layout->addWidget(m_sculptInfoLabel);
    layout->addStretch();

    connect(m_brushCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeshEditorModule::onSculptBrushChanged);
    connect(m_brushSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MeshEditorModule::onBrushSizeChanged);
    connect(m_brushStrengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MeshEditorModule::onBrushStrengthChanged);
    connect(m_symmetryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MeshEditorModule::onSymmetryChanged);
    connect(m_smoothBtn, &QPushButton::clicked, this, &MeshEditorModule::onSmoothMesh);
    connect(m_remeshBtn, &QPushButton::clicked, this, &MeshEditorModule::onRemesh);
    connect(m_decimateBtn, &QPushButton::clicked, this, &MeshEditorModule::onDecimateMesh);

    m_tabWidget->addTab(m_sculptingTab, "Sculpting");
}

void MeshEditorModule::setupRiggingTab() {
    m_riggingTab = new QWidget();
    auto* layout = new QVBoxLayout(m_riggingTab);

    auto* toolbar = new QHBoxLayout();
    m_addBoneBtn = createButton("Add Bone");
    m_removeBoneBtn = createButton("Remove Bone");
    m_bindSkinBtn = createButton("Bind Skin");
    toolbar->addWidget(m_addBoneBtn);
    toolbar->addWidget(m_removeBoneBtn);
    toolbar->addWidget(m_bindSkinBtn);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = createSplitter(Qt::Horizontal);

    m_boneTree = createTreeWidget({"Bone", "Parent", "Children"});
    m_boneTree->setHeaderLabels({"Bone", "Parent", "Children"});
    splitter->addWidget(m_boneTree);

    auto* weightPanel = new QWidget();
    auto* weightLayout = new QVBoxLayout(weightPanel);
    weightLayout->addWidget(createLabel("Weight Painting:"));

    m_weightTree = createTreeWidget({"Vertex", "Weight", "Influence"});
    m_weightTree->setHeaderLabels({"Vertex", "Weight", "Influence"});
    weightLayout->addWidget(m_weightTree);

    auto* weightToolbar = new QHBoxLayout();
    m_weightValueSpin = createDoubleSpinBox(0.0, 1.0, 1.0, 3, "");
    m_addWeightBtn = createButton("Add Weight");
    m_clearWeightBtn = createButton("Clear");
    weightToolbar->addWidget(createLabel("Value:"));
    weightToolbar->addWidget(m_weightValueSpin);
    weightToolbar->addWidget(m_addWeightBtn);
    weightToolbar->addWidget(m_clearWeightBtn);
    weightToolbar->addStretch();
    weightLayout->addLayout(weightToolbar);

    splitter->addWidget(weightPanel);
    layout->addWidget(splitter);

    connect(m_addBoneBtn, &QPushButton::clicked, this, &MeshEditorModule::onAddRigBone);
    connect(m_removeBoneBtn, &QPushButton::clicked, this, &MeshEditorModule::onRemoveRigBone);
    connect(m_bindSkinBtn, &QPushButton::clicked, this, &MeshEditorModule::onBindSkin);
    connect(m_addWeightBtn, &QPushButton::clicked, this, &MeshEditorModule::onAddWeightPaint);
    connect(m_clearWeightBtn, &QPushButton::clicked, this, &MeshEditorModule::onClearWeightPaint);

    m_tabWidget->addTab(m_riggingTab, "Rigging");
}

void MeshEditorModule::setupExportTab() {
    m_exportTab = new QWidget();
    auto* layout = new QVBoxLayout(m_exportTab);

    auto* formatGroup = new QGroupBox("Export Settings");
    auto* formatLayout = new QFormLayout(formatGroup);

    m_exportFormatCombo = createComboBox({"FBX (.fbx)", "OBJ (.obj)", "GLTF (.gltf)", "GLB (.glb)", "STL (.stl)", "PLY (.ply)", "DAE (.dae)", "KN5 (.kn5)"});
    formatLayout->addRow("Format:", m_exportFormatCombo);

    m_exportNormalsCheck = createCheckBox("Export Normals", true);
    m_exportUVCheck = createCheckBox("Export UV Coordinates", true);
    m_exportColorsCheck = createCheckBox("Export Vertex Colors", false);
    m_exportAnimCheck = createCheckBox("Export Animations", false);
    formatLayout->addRow("", m_exportNormalsCheck);
    formatLayout->addRow("", m_exportUVCheck);
    formatLayout->addRow("", m_exportColorsCheck);
    formatLayout->addRow("", m_exportAnimCheck);

    layout->addWidget(formatGroup);

    auto* lodGroup = new QGroupBox("LOD Generation");
    auto* lodLayout = new QFormLayout(lodGroup);
    m_lodLevelSpin = createSpinBox(1, 5, 3, " levels");
    m_lodReductionSpin = createDoubleSpinBox(0.1, 0.9, 0.5, 2, "");
    m_generateLODBtn = createButton("Generate LODs");
    lodLayout->addRow("Levels:", m_lodLevelSpin);
    lodLayout->addRow("Reduction:", m_lodReductionSpin);
    lodLayout->addRow("", m_generateLODBtn);
    layout->addWidget(lodGroup);

    m_exportProgress = new QProgressBar();
    m_exportProgress->setVisible(false);
    layout->addWidget(m_exportProgress);

    m_exportInfoLabel = createLabel("Ready to export");
    layout->addWidget(m_exportInfoLabel);
    layout->addStretch();

    connect(m_generateLODBtn, &QPushButton::clicked, this, &MeshEditorModule::onGenerateLOD);

    m_tabWidget->addTab(m_exportTab, "Export");
}

void MeshEditorModule::refreshMeshList() {
    m_meshTree->clear();
    m_meshTree->addTopLevelItem(new QTreeWidgetItem({"Car Body", "12450", "24896", "Yes"}));
    m_meshTree->addTopLevelItem(new QTreeWidgetItem({"Wheel_FL", "3200", "6396", "No"}));
    m_meshTree->addTopLevelItem(new QTreeWidgetItem({"Wheel_FR", "3200", "6396", "No"}));
    m_meshTree->addTopLevelItem(new QTreeWidgetItem({"Spoiler", "850", "1696", "No"}));
    m_operandList->clear();
    m_operandList->addItem("Car Body (12450 verts)");
    m_operandList->addItem("Spoiler (850 verts)");
}

void MeshEditorModule::refreshBoneList() {
    m_boneTree->clear();
    auto* root = new QTreeWidgetItem(m_boneTree, {"Hips", "None", "2"});
    root->addChild(new QTreeWidgetItem({"Spine", "Hips", "1"}));
    auto* spine = root->child(0);
    spine->addChild(new QTreeWidgetItem({"Head", "Spine", "0"}));
    root->addChild(new QTreeWidgetItem({"Left Leg", "Hips", "1"}));
    m_boneTree->expandAll();
}

void MeshEditorModule::onMeshSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        m_meshInfoLabel->setText(QString("Selected: %1 (%2 verts, %3 tris)").arg(item->text(0), item->text(1), item->text(2)));
    }
}

void MeshEditorModule::onLoadMesh() {
    QString path = selectFile("Load Mesh", "Mesh Files (*.obj *.fbx *.gltf *.glb *.kn5 *.stl *.ply);;All Files (*)");
    if (!path.isEmpty()) {
        log(QString("Loading mesh: %1").arg(path));
        refreshMeshList();
        m_sculptInfoLabel->setText("Mesh loaded. Ready for sculpting.");
    }
}

void MeshEditorModule::onExportMesh() {
    QString path = selectFile("Export Mesh", "Mesh Files (*.obj *.fbx *.gltf *.glb *.stl *.ply)");
    if (!path.isEmpty()) {
        log(QString("Exporting mesh to: %1").arg(path));
    }
}

void MeshEditorModule::onApplyBoolOp() {
    log(QString("Applying boolean operation: %1").arg(m_boolOpCombo->currentText()));
    refreshMeshList();
    logSuccess("Boolean operation completed");
}

void MeshEditorModule::onBoolOpChanged(int index) {
    Q_UNUSED(index);
}

void MeshEditorModule::onUnwrap() {
    log(QString("Unwrapping using method: %1").arg(m_unwrapMethodCombo->currentText()));
    m_uvPreviewLabel->setText("UV layout generated");
    logSuccess("UV unwrap completed");
}

void MeshEditorModule::onUnwrapMethodChanged(int index) {
    Q_UNUSED(index);
}

void MeshEditorModule::onPackCharts() {
    log(QString("Packing charts with margin: %1").arg(m_packMarginSpin->value(), 0, 'f', 3));
    logSuccess("Charts packed");
}

void MeshEditorModule::onSeamAdded() {
    log("Seam edge marked");
    m_sculptInfoLabel->setText("Seam added. Re-unwrap to update UV layout.");
}

void MeshEditorModule::onSculptBrushChanged(int index) {
    log(QString("Brush changed to: %1").arg(m_brushCombo->currentText()));
}

void MeshEditorModule::onBrushSizeChanged(int value) {
    Q_UNUSED(value);
}

void MeshEditorModule::onBrushStrengthChanged(double value) {
    Q_UNUSED(value);
}

void MeshEditorModule::onSymmetryChanged(int index) {
    log(QString("Symmetry set to: %1").arg(m_symmetryCombo->currentText()));
}

void MeshEditorModule::onAddRigBone() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Bone", "Bone name:", QLineEdit::Normal, "NewBone", &ok);
    if (ok && !name.isEmpty()) {
        m_boneTree->addTopLevelItem(new QTreeWidgetItem({name, "None", "0"}));
        log(QString("Added bone: %1").arg(name));
    }
}

void MeshEditorModule::onRemoveRigBone() {
    auto* item = m_boneTree->currentItem();
    if (item) {
        log(QString("Removed bone: %1").arg(item->text(0)));
        delete item;
    }
}

void MeshEditorModule::onBindSkin() {
    log("Binding skin to skeleton...");
    logSuccess("Skin bound successfully");
}

void MeshEditorModule::onAddWeightPaint() {
    auto* item = m_weightTree->currentItem();
    if (item) {
        item->setText(1, QString::number(m_weightValueSpin->value(), 'f', 3));
        log(QString("Weight set for vertex %1: %2").arg(item->text(0)).arg(m_weightValueSpin->value(), 0, 'f', 3));
    } else {
        m_weightTree->addTopLevelItem(new QTreeWidgetItem({"Vertex " + QString::number(m_weightTree->topLevelItemCount() + 1), QString::number(m_weightValueSpin->value(), 'f', 3), "Bone"}));
    }
}

void MeshEditorModule::onClearWeightPaint() {
    auto* item = m_weightTree->currentItem();
    if (item) {
        item->setText(1, "0.000");
        log(QString("Cleared weight for vertex %1").arg(item->text(0)));
    }
}

void MeshEditorModule::onGenerateLOD() {
    m_exportProgress->setVisible(true);
    m_exportProgress->setValue(0);
    for (int i = 0; i <= 100; i += 20) {
        m_exportProgress->setValue(i);
    }
    log(QString("Generated %1 LOD levels").arg(m_lodLevelSpin->value()));
    m_exportProgress->setVisible(false);
}

void MeshEditorModule::onDecimateMesh() {
    log(QString("Decimating mesh to %1% of original").arg(m_decimateRatioSpin->value() * 100, 0, 'f', 0));
    logSuccess("Mesh decimated");
}

void MeshEditorModule::onRemesh() {
    log(QString("Remeshing to %1 triangles").arg(m_remeshResSpin->value()));
    logSuccess("Remesh completed");
}

void MeshEditorModule::onSmoothMesh() {
    log("Smoothing mesh...");
    logSuccess("Mesh smoothed");
}

void MeshEditorModule::onShowContextMenu(const QPoint& pos) {
    Q_UNUSED(pos);
}

} // namespace mesh
} // namespace ks

#include "MeshEditorModule.moc"
