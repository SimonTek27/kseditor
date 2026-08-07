#include "PaintEditorWidget.h"
#include "PaintPainter.h"
#include "VectorDesignCanvas.h"
#include "../../core/paint/PaintEditor.h"
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>

namespace ks {

PaintEditorWidget::PaintEditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(PaintEditor::instance())
    , m_painterWidget(nullptr)
    , m_paintEditor(nullptr)
    , m_updatingUI(false)
{
    setupUI();

    connect(m_editor, &PaintEditor::skinListChanged, this, &PaintEditorWidget::refreshSkinList);
    connect(m_editor, &PaintEditor::skinLoaded, this, [this](const QString& skinName) {
        refreshLayerList();
        if (m_viewport3D && !m_editor->carPath().isEmpty()) {
            m_viewport3D->setCarPath(m_editor->carPath());
            m_viewport3D->focusOnModel();
        }
    });
    connect(m_editor, &PaintEditor::paintModified, this, &PaintEditorWidget::paintModified);
    connect(m_editor, &PaintEditor::textureLoaded, this, [this](const QImage& tex) {
        if (m_paintEditor) {
            m_paintEditor->setTexture(tex);
        }
        if (m_painterWidget) {
            m_painterWidget->setTexture(tex);
        }
        if (m_viewport3D) {
            m_viewport3D->applyPaintTexture(tex);
        }
    });
}

void PaintEditorWidget::setCarPath(const QString& path)
{
    m_editor->setCarPath(path);
    refreshSkinList();
}

void PaintEditorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top options bar (PhotoGIMP/ksModeler style toolbar)
    setupToolBar();
    mainLayout->addWidget(m_toolBar);

    // Main content splitter: canvas left, tool/layer panels right
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(3);

    // ── Left: 2D texture canvas + 3D preview ─────────────────────────────
    QWidget* canvasArea = new QWidget(splitter);
    canvasArea->setObjectName("paintViewport");
    QVBoxLayout* canvasLayout = new QVBoxLayout(canvasArea);
    canvasLayout->setContentsMargins(0, 0, 0, 0);
    canvasLayout->setSpacing(0);

    QWidget* canvasToolbar = new QWidget(canvasArea);
    canvasToolbar->setObjectName("toolOptionsBar");
    canvasToolbar->setFixedHeight(28);
    QHBoxLayout* ctLayout = new QHBoxLayout(canvasToolbar);
    ctLayout->setContentsMargins(8, 0, 8, 0);
    QLabel* canvasTitle = new QLabel(tr("Texture Canvas"), canvasToolbar);
    canvasTitle->setStyleSheet("font-weight: 600;");
    ctLayout->addWidget(canvasTitle);
    ctLayout->addStretch();
    canvasLayout->addWidget(canvasToolbar);

    m_canvasStack = new QStackedWidget(canvasArea);
    m_canvasStack->setObjectName("paintCanvas");

    // Paint-style core editor (Paint-like, Qt-based)
    m_paintEditor = new ks::paint::PaintEditor(m_canvasStack);
    m_paintEditor->setObjectName("paintEditor");
    m_paintEditor->setMinimumHeight(200);
    m_canvasStack->addWidget(m_paintEditor);

    m_painterWidget = new PaintPainterWidget(m_canvasStack);
    m_painterWidget->setMinimumHeight(200);
    m_canvasStack->addWidget(m_painterWidget);

    m_vectorCanvas = new VectorDesignCanvas(m_canvasStack);
    m_vectorCanvas->setMinimumHeight(200);
    m_canvasStack->addWidget(m_vectorCanvas);
    m_canvasStack->setCurrentIndex(0);

    canvasLayout->addWidget(m_canvasStack, 1);

    m_viewport3D = new ks::paint::PaintViewport(canvasArea);
    m_viewport3D->setObjectName("paintViewport");
    m_viewport3D->setMinimumHeight(220);
    canvasLayout->addWidget(m_viewport3D);

    splitter->addWidget(canvasArea);

    // ── Right: panel column (mirrors ksModeler object list / tools) ──────
    QScrollArea* scrollArea = new QScrollArea(splitter);
    scrollArea->setObjectName("paintLayerPanel");
    scrollArea->setWidgetResizable(true);

    QWidget* panelContent = new QWidget(scrollArea);
    panelContent->setObjectName("paintToolPanel");
    QVBoxLayout* panelLayout = new QVBoxLayout(panelContent);
    panelLayout->setContentsMargins(6, 6, 6, 6);
    panelLayout->setSpacing(6);

    setupSkinPanel(panelContent, panelLayout);
    setupLayerPanel(panelContent, panelLayout);
    setupPaintPanel(panelContent, panelLayout);
    setupVectorPanel(panelContent, panelLayout);
    setupLicensePlatePanel(panelContent, panelLayout);
    setupPalettePanel(panelContent, panelLayout);
    panelLayout->addStretch();

    scrollArea->setWidget(panelContent);
    splitter->addWidget(scrollArea);

    splitter->setStretchFactor(0, 7);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter, 1);

    connect(m_createSkinBtn, &QPushButton::clicked, this, &PaintEditorWidget::onCreateSkin);
    connect(m_deleteSkinBtn, &QPushButton::clicked, this, &PaintEditorWidget::onDeleteSkin);
    connect(m_duplicateSkinBtn, &QPushButton::clicked, this, &PaintEditorWidget::onDuplicateSkin);
    connect(m_skinList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        onSkinSelected(current);
    });
    connect(m_addLayerBtn, &QPushButton::clicked, this, &PaintEditorWidget::onAddLayer);
    connect(m_removeLayerBtn, &QPushButton::clicked, this, &PaintEditorWidget::onRemoveLayer);
    connect(m_moveUpBtn, &QPushButton::clicked, this, &PaintEditorWidget::onMoveLayerUp);
    connect(m_moveDownBtn, &QPushButton::clicked, this, &PaintEditorWidget::onMoveLayerDown);
    connect(m_layerList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        onLayerSelected(current);
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, &PaintEditorWidget::onLayerOpacityChanged);
    connect(m_visibleCheck, &QCheckBox::stateChanged, this, &PaintEditorWidget::onLayerVisibilityChanged);
    connect(m_colorBtn, &QPushButton::clicked, this, &PaintEditorWidget::onColorSelected);
    connect(m_secondaryColorBtn, &QPushButton::clicked, this, &PaintEditorWidget::onSecondaryColorSelected);
    connect(m_brushTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PaintEditorWidget::onBrushTypeChanged);
    connect(m_brushSizeSlider, &QSlider::valueChanged, this, &PaintEditorWidget::onBrushSizeChanged);
    connect(m_brushHardnessSlider, &QSlider::valueChanged, this, &PaintEditorWidget::onBrushHardnessChanged);
    connect(m_brushStrengthSlider, &QSlider::valueChanged, this, &PaintEditorWidget::onBrushStrengthChanged);
    connect(m_brushFlowSlider, &QSlider::valueChanged, this, &PaintEditorWidget::onBrushFlowChanged);
    connect(m_generatePlateBtn, &QPushButton::clicked, this, &PaintEditorWidget::onLicensePlateGenerate);
    connect(m_saveSkinBtn, &QPushButton::clicked, this, &PaintEditorWidget::onSaveSkin);
    connect(m_refreshBtn, &QPushButton::clicked, this, &PaintEditorWidget::onRefreshSkins);
    connect(m_exportDdsBtn, &QPushButton::clicked, this, &PaintEditorWidget::onExportDDS);
    connect(m_importDecalBtn, &QPushButton::clicked, this, &PaintEditorWidget::onImportDecal);
    connect(m_templateBtn, &QPushButton::clicked, this, &PaintEditorWidget::onCreateFromTemplate);
    connect(m_undoBtn, &QPushButton::clicked, this, &PaintEditorWidget::onUndo);
    connect(m_redoBtn, &QPushButton::clicked, this, &PaintEditorWidget::onRedo);

    // Vector tool connections
    connect(m_editModeGroup, &QButtonGroup::idClicked,
            this, &PaintEditorWidget::onEditModeChanged);
    connect(m_vectorToolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaintEditorWidget::onVectorToolChanged);
    connect(m_vectorFillColorBtn, &QPushButton::clicked, this, &PaintEditorWidget::onVectorFillColor);
    connect(m_vectorStrokeColorBtn, &QPushButton::clicked, this, &PaintEditorWidget::onVectorStrokeColor);
    connect(m_vectorStrokeWidthSlider, &QSlider::valueChanged, this, &PaintEditorWidget::onVectorStrokeWidthChanged);
    connect(m_vectorFilledCheck, &QCheckBox::stateChanged, this, &PaintEditorWidget::onVectorDrawFilledChanged);
    connect(m_vectorDeleteBtn, &QPushButton::clicked, this, &PaintEditorWidget::onVectorDeleteSelected);
    connect(m_vectorClearBtn, &QPushButton::clicked, this, &PaintEditorWidget::onVectorClearAll);
    connect(m_vectorCanvas, &VectorDesignCanvas::shapesChanged, this, &PaintEditorWidget::onVectorShapesChanged);

    // Update undo/redo button states
    connect(m_editor, &PaintEditor::paintModified, this, [this]() {
        m_undoBtn->setEnabled(PaintSystem::canUndo());
        m_redoBtn->setEnabled(PaintSystem::canRedo());
    });

    connect(m_painterWidget, &PaintPainterWidget::textureChanged, this, [this](const QImage& tex) {
        if (m_viewport3D)
            m_viewport3D->applyPaintTexture(tex);
    });

    connect(m_paintEditor, &ks::paint::PaintEditor::textureChanged, this, [this](const QImage& tex) {
        if (m_viewport3D)
            m_viewport3D->applyPaintTexture(tex);
    });

    connect(m_paintEditor, &ks::paint::PaintEditor::imageEdited, this, [this]() {
        emit paintModified();
    });

    // Update undo/redo button states from paint editor history
    connect(m_paintEditor, &ks::paint::PaintEditor::historyChanged, this, [this]() {
        if (m_paintEditor && m_canvasStack->currentIndex() == 0) {
            m_undoBtn->setEnabled(m_paintEditor->canUndo());
            m_redoBtn->setEnabled(m_paintEditor->canRedo());
        }
    });

    refreshSkinList();
    clearLayerUI();
}

void PaintEditorWidget::setupToolBar()
{
    m_toolBar = new QWidget(this);
    m_toolBar->setObjectName("toolOptionsBar");
    m_toolBar->setFixedHeight(40);

    QHBoxLayout* tbLayout = new QHBoxLayout(m_toolBar);
    tbLayout->setContentsMargins(8, 3, 8, 3);
    tbLayout->setSpacing(6);

    QLabel* modeLabel = new QLabel(tr("Mode:"), m_toolBar);
    tbLayout->addWidget(modeLabel);

    m_editModeGroup = new QButtonGroup(m_toolBar);
    m_rasterModeRadio = new QRadioButton(tr("Raster"), m_toolBar);
    m_rasterModeRadio->setChecked(true);
    m_vectorModeRadio = new QRadioButton(tr("Vector"), m_toolBar);
    m_editModeGroup->addButton(m_rasterModeRadio, 0);
    m_editModeGroup->addButton(m_vectorModeRadio, 1);
    tbLayout->addWidget(m_rasterModeRadio);
    tbLayout->addWidget(m_vectorModeRadio);

    tbLayout->addStretch();

    m_saveSkinBtn = new QPushButton(tr("Save Skin"), m_toolBar);
    m_saveSkinBtn->setObjectName("saveBtn");
    tbLayout->addWidget(m_saveSkinBtn);

    m_undoBtn = new QPushButton(tr("Undo"), m_toolBar);
    m_undoBtn->setEnabled(false);
    tbLayout->addWidget(m_undoBtn);

    m_redoBtn = new QPushButton(tr("Redo"), m_toolBar);
    m_redoBtn->setEnabled(false);
    tbLayout->addWidget(m_redoBtn);

    m_importDecalBtn = new QPushButton(tr("Import Decal"), m_toolBar);
    m_importDecalBtn->setToolTip(tr("Import an image as a new decal layer"));
    tbLayout->addWidget(m_importDecalBtn);

    m_exportDdsBtn = new QPushButton(tr("Export DDS"), m_toolBar);
    m_exportDdsBtn->setToolTip(tr("Export paint texture as DDS (AC format)"));
    tbLayout->addWidget(m_exportDdsBtn);

    m_templateBtn = new QPushButton(tr("Template..."), m_toolBar);
    m_templateBtn->setToolTip(tr("Create new skin from template"));
    tbLayout->addWidget(m_templateBtn);

    m_refreshBtn = new QPushButton(tr("Refresh"), m_toolBar);
    tbLayout->addWidget(m_refreshBtn);
}

void PaintEditorWidget::setupSkinPanel(QWidget* parent, QVBoxLayout* layout)
{
    m_skinsGroup = new QGroupBox(tr("Skins"), parent);
    QVBoxLayout* skinLayout = new QVBoxLayout(m_skinsGroup);
    skinLayout->setContentsMargins(6, 10, 6, 6);
    skinLayout->setSpacing(4);

    m_skinList = new QListWidget(m_skinsGroup);
    m_skinList->setMinimumHeight(90);
    skinLayout->addWidget(m_skinList);

    QWidget* btnRow = new QWidget(m_skinsGroup);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(4);

    m_createSkinBtn = new QPushButton("+", btnRow);
    m_createSkinBtn->setObjectName("toolBtn");
    m_createSkinBtn->setFixedSize(26, 26);
    m_createSkinBtn->setToolTip(tr("Create new skin"));
    btnLayout->addWidget(m_createSkinBtn);

    m_duplicateSkinBtn = new QPushButton("D", btnRow);
    m_duplicateSkinBtn->setObjectName("toolBtn");
    m_duplicateSkinBtn->setFixedSize(26, 26);
    m_duplicateSkinBtn->setToolTip(tr("Duplicate skin"));
    btnLayout->addWidget(m_duplicateSkinBtn);

    m_deleteSkinBtn = new QPushButton("-", btnRow);
    m_deleteSkinBtn->setObjectName("dangerBtn");
    m_deleteSkinBtn->setFixedSize(26, 26);
    m_deleteSkinBtn->setToolTip(tr("Delete skin"));
    btnLayout->addWidget(m_deleteSkinBtn);

    btnLayout->addStretch();
    skinLayout->addWidget(btnRow);

    layout->addWidget(m_skinsGroup);
}

void PaintEditorWidget::setupLayerPanel(QWidget* parent, QVBoxLayout* layout)
{
    m_layersGroup = new QGroupBox(tr("Layers"), parent);
    QVBoxLayout* layerLayout = new QVBoxLayout(m_layersGroup);
    layerLayout->setContentsMargins(6, 10, 6, 6);
    layerLayout->setSpacing(4);

    QWidget* listRow = new QWidget(m_layersGroup);
    QHBoxLayout* listLayout = new QHBoxLayout(listRow);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(4);

    m_layerList = new QListWidget(listRow);
    m_layerList->setMinimumHeight(90);
    listLayout->addWidget(m_layerList);

    QWidget* btnCol = new QWidget(listRow);
    btnCol->setFixedWidth(32);
    QVBoxLayout* btnColLayout = new QVBoxLayout(btnCol);
    btnColLayout->setContentsMargins(0, 0, 0, 0);
    btnColLayout->setSpacing(4);

    m_addLayerBtn = new QPushButton("+", btnCol);
    m_addLayerBtn->setObjectName("toolBtn");
    m_addLayerBtn->setFixedSize(28, 28);
    m_addLayerBtn->setToolTip(tr("Add layer"));
    btnColLayout->addWidget(m_addLayerBtn);

    m_removeLayerBtn = new QPushButton("-", btnCol);
    m_removeLayerBtn->setObjectName("toolBtn");
    m_removeLayerBtn->setFixedSize(28, 28);
    m_removeLayerBtn->setToolTip(tr("Remove layer"));
    btnColLayout->addWidget(m_removeLayerBtn);

    m_moveUpBtn = new QPushButton("^", btnCol);
    m_moveUpBtn->setObjectName("toolBtn");
    m_moveUpBtn->setFixedSize(28, 28);
    m_moveUpBtn->setToolTip(tr("Move layer up"));
    btnColLayout->addWidget(m_moveUpBtn);

    m_moveDownBtn = new QPushButton("v", btnCol);
    m_moveDownBtn->setObjectName("toolBtn");
    m_moveDownBtn->setFixedSize(28, 28);
    m_moveDownBtn->setToolTip(tr("Move layer down"));
    btnColLayout->addWidget(m_moveDownBtn);

    btnColLayout->addStretch();
    listLayout->addWidget(btnCol);
    layerLayout->addWidget(listRow);

    m_layerPropsGroup = new QGroupBox(tr("Layer Properties"), m_layersGroup);
    QFormLayout* propsLayout = new QFormLayout(m_layerPropsGroup);
    propsLayout->setContentsMargins(6, 10, 6, 6);
    propsLayout->setSpacing(4);

    m_layerNameEdit = new QLineEdit(m_layerPropsGroup);
    propsLayout->addRow(tr("Name:"), m_layerNameEdit);

    m_layerTypeCombo = new QComboBox(m_layerPropsGroup);
    m_layerTypeCombo->addItems({"decal", "paint", "texture", "vector"});
    propsLayout->addRow(tr("Type:"), m_layerTypeCombo);

    QWidget* opacityWidget = new QWidget(m_layerPropsGroup);
    QHBoxLayout* opacityLayout = new QHBoxLayout(opacityWidget);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    m_opacitySlider = new QSlider(Qt::Horizontal, opacityWidget);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    m_opacityLabel = new QLabel("100%", opacityWidget);
    m_opacityLabel->setFixedWidth(35);
    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityLabel);
    propsLayout->addRow(tr("Opacity:"), opacityWidget);

    m_visibleCheck = new QCheckBox(tr("Visible"), m_layerPropsGroup);
    m_visibleCheck->setChecked(true);
    propsLayout->addRow("", m_visibleCheck);

    layerLayout->addWidget(m_layerPropsGroup);
    layout->addWidget(m_layersGroup);
}

void PaintEditorWidget::setupPaintPanel(QWidget* parent, QVBoxLayout* layout)
{
    m_paintGroup = new QGroupBox(tr("Paint Tools"), parent);
    QFormLayout* paintLayout = new QFormLayout(m_paintGroup);
    paintLayout->setContentsMargins(6, 10, 6, 6);
    paintLayout->setSpacing(4);

    m_brushTypeCombo = new QComboBox(m_paintGroup);
    m_brushTypeCombo->addItems({
        tr("Brush"), tr("Airbrush"), tr("Square Brush"),
        tr("Eraser"), tr("Smudge"), tr("Blur"), tr("Sharpen"),
        tr("Clone"), tr("Healing"), tr("Dodge"), tr("Burn"),
        tr("Fill"), tr("Gradient"), tr("Stamp")
    });
    paintLayout->addRow(tr("Tool:"), m_brushTypeCombo);

    QWidget* colorRow = new QWidget(m_paintGroup);
    QHBoxLayout* colorLayout = new QHBoxLayout(colorRow);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(4);

    m_colorBtn = new QPushButton(tr("Color"), colorRow);
    m_colorBtn->setObjectName("colorSwatch");
    m_colorBtn->setMinimumHeight(24);
    m_colorBtn->setStyleSheet("QPushButton#colorSwatch { background: #cc0000; color: #fff; border: 1px solid #333336; border-radius: 3px; }");
    colorLayout->addWidget(m_colorBtn);

    m_secondaryColorBtn = new QPushButton(tr("2nd"), colorRow);
    m_secondaryColorBtn->setObjectName("colorSwatch");
    m_secondaryColorBtn->setMinimumHeight(24);
    m_secondaryColorBtn->setToolTip(tr("Secondary color (gradient end)"));
    m_secondaryColorBtn->setStyleSheet("QPushButton#colorSwatch { background: #ffffff; color: #111; border: 1px solid #333336; border-radius: 3px; }");
    colorLayout->addWidget(m_secondaryColorBtn);

    paintLayout->addRow(tr("Colors:"), colorRow);

    QWidget* sizeWidget = new QWidget(m_paintGroup);
    QHBoxLayout* sizeLayout = new QHBoxLayout(sizeWidget);
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    m_brushSizeSlider = new QSlider(Qt::Horizontal, sizeWidget);
    m_brushSizeSlider->setRange(1, 200);
    m_brushSizeSlider->setValue(20);
    m_brushSizeLabel = new QLabel("20", sizeWidget);
    m_brushSizeLabel->setFixedWidth(30);
    sizeLayout->addWidget(m_brushSizeSlider);
    sizeLayout->addWidget(m_brushSizeLabel);
    paintLayout->addRow(tr("Brush Size:"), sizeWidget);

    QWidget* hardnessSlider = new QWidget(m_paintGroup);
    QHBoxLayout* hardnessLayout = new QHBoxLayout(hardnessSlider);
    hardnessLayout->setContentsMargins(0, 0, 0, 0);
    m_brushHardnessSlider = new QSlider(Qt::Horizontal, hardnessSlider);
    m_brushHardnessSlider->setRange(0, 100);
    m_brushHardnessSlider->setValue(50);
    m_brushHardnessLabel = new QLabel("50%", hardnessSlider);
    m_brushHardnessLabel->setFixedWidth(35);
    hardnessLayout->addWidget(m_brushHardnessSlider);
    hardnessLayout->addWidget(m_brushHardnessLabel);
    paintLayout->addRow(tr("Hardness:"), hardnessSlider);

    QWidget* strengthSlider = new QWidget(m_paintGroup);
    QHBoxLayout* strengthLayout = new QHBoxLayout(strengthSlider);
    strengthLayout->setContentsMargins(0, 0, 0, 0);
    m_brushStrengthSlider = new QSlider(Qt::Horizontal, strengthSlider);
    m_brushStrengthSlider->setRange(1, 100);
    m_brushStrengthSlider->setValue(100);
    m_brushStrengthLabel = new QLabel("100%", strengthSlider);
    m_brushStrengthLabel->setFixedWidth(35);
    strengthLayout->addWidget(m_brushStrengthSlider);
    strengthLayout->addWidget(m_brushStrengthLabel);
    paintLayout->addRow(tr("Strength:"), strengthSlider);

    QWidget* flowSlider = new QWidget(m_paintGroup);
    QHBoxLayout* flowLayout = new QHBoxLayout(flowSlider);
    flowLayout->setContentsMargins(0, 0, 0, 0);
    m_brushFlowSlider = new QSlider(Qt::Horizontal, flowSlider);
    m_brushFlowSlider->setRange(1, 100);
    m_brushFlowSlider->setValue(100);
    m_brushFlowLabel = new QLabel("100%", flowSlider);
    m_brushFlowLabel->setFixedWidth(35);
    flowLayout->addWidget(m_brushFlowSlider);
    flowLayout->addWidget(m_brushFlowLabel);
    paintLayout->addRow(tr("Flow:"), flowSlider);

    layout->addWidget(m_paintGroup);
}

void PaintEditorWidget::setupLicensePlatePanel(QWidget* parent, QVBoxLayout* layout)
{
    m_plateGroup = new QGroupBox(tr("License Plate"), parent);
    QFormLayout* plateLayout = new QFormLayout(m_plateGroup);
    plateLayout->setContentsMargins(6, 10, 6, 6);
    plateLayout->setSpacing(4);

    m_plateText = new QLineEdit(m_plateGroup);
    m_plateText->setPlaceholderText(tr("Enter plate text"));
    plateLayout->addRow(tr("Text:"), m_plateText);

    m_plateCountry = new QComboBox(m_plateGroup);
    m_plateCountry->addItems(PaintSystem::getSupportedCountries());
    plateLayout->addRow(tr("Country:"), m_plateCountry);

    m_generatePlateBtn = new QPushButton(tr("Generate"), m_plateGroup);
    plateLayout->addRow("", m_generatePlateBtn);

    layout->addWidget(m_plateGroup);
}

void PaintEditorWidget::setupVectorPanel(QWidget* parent, QVBoxLayout* layout)
{
    m_vectorToolsGroup = new QGroupBox(tr("Vector Tools"), parent);
    m_vectorToolsGroup->setVisible(false);
    QFormLayout* vectorLayout = new QFormLayout(m_vectorToolsGroup);
    vectorLayout->setContentsMargins(6, 10, 6, 6);
    vectorLayout->setSpacing(4);

    m_vectorToolCombo = new QComboBox(m_vectorToolsGroup);
    m_vectorToolCombo->addItems({
        tr("Select"), tr("Rectangle"), tr("Ellipse"),
        tr("Line"), tr("Polygon"), tr("Pen")
    });
    vectorLayout->addRow(tr("Tool:"), m_vectorToolCombo);

    QWidget* vectorColorRow = new QWidget(m_vectorToolsGroup);
    QHBoxLayout* vectorColorLayout = new QHBoxLayout(vectorColorRow);
    vectorColorLayout->setContentsMargins(0, 0, 0, 0);
    vectorColorLayout->setSpacing(4);

    m_vectorFillColorBtn = new QPushButton(tr("Fill"), vectorColorRow);
    m_vectorFillColorBtn->setObjectName("colorSwatch");
    m_vectorFillColorBtn->setStyleSheet("QPushButton#colorSwatch { background: #cc0000; color: #fff; border: 1px solid #333336; border-radius: 3px; }");
    vectorColorLayout->addWidget(m_vectorFillColorBtn);

    m_vectorStrokeColorBtn = new QPushButton(tr("Stroke"), vectorColorRow);
    m_vectorStrokeColorBtn->setObjectName("colorSwatch");
    m_vectorStrokeColorBtn->setStyleSheet("QPushButton#colorSwatch { background: #000000; color: #fff; border: 1px solid #333336; border-radius: 3px; }");
    vectorColorLayout->addWidget(m_vectorStrokeColorBtn);

    m_vectorFilledCheck = new QCheckBox(tr("Filled"), vectorColorRow);
    m_vectorFilledCheck->setChecked(true);
    vectorColorLayout->addWidget(m_vectorFilledCheck);

    vectorLayout->addRow(tr("Colors:"), vectorColorRow);

    QWidget* strokeWidthWidget = new QWidget(m_vectorToolsGroup);
    QHBoxLayout* strokeWidthLayout = new QHBoxLayout(strokeWidthWidget);
    strokeWidthLayout->setContentsMargins(0, 0, 0, 0);
    m_vectorStrokeWidthSlider = new QSlider(Qt::Horizontal, strokeWidthWidget);
    m_vectorStrokeWidthSlider->setRange(1, 20);
    m_vectorStrokeWidthSlider->setValue(2);
    m_vectorStrokeWidthLabel = new QLabel("2px", strokeWidthWidget);
    m_vectorStrokeWidthLabel->setFixedWidth(30);
    strokeWidthLayout->addWidget(m_vectorStrokeWidthSlider);
    strokeWidthLayout->addWidget(m_vectorStrokeWidthLabel);
    vectorLayout->addRow(tr("Stroke W:"), strokeWidthWidget);

    QWidget* vectorActionsRow = new QWidget(m_vectorToolsGroup);
    QHBoxLayout* vectorActionsLayout = new QHBoxLayout(vectorActionsRow);
    vectorActionsLayout->setContentsMargins(0, 0, 0, 0);
    vectorActionsLayout->setSpacing(4);

    m_vectorDeleteBtn = new QPushButton(tr("Delete"), vectorActionsRow);
    m_vectorDeleteBtn->setObjectName("dangerBtn");
    vectorActionsLayout->addWidget(m_vectorDeleteBtn);

    m_vectorClearBtn = new QPushButton(tr("Clear All"), vectorActionsRow);
    m_vectorClearBtn->setObjectName("dangerBtn");
    vectorActionsLayout->addWidget(m_vectorClearBtn);

    vectorLayout->addRow(tr("Actions:"), vectorActionsRow);

    layout->addWidget(m_vectorToolsGroup);
}

void PaintEditorWidget::setupPalettePanel(QWidget* parent, QVBoxLayout* layout)
{
    QGroupBox* paletteGroup = new QGroupBox(tr("Colors"), parent);
    QHBoxLayout* paletteLayout = new QHBoxLayout(paletteGroup);
    paletteLayout->setContentsMargins(6, 10, 6, 6);
    paletteLayout->setSpacing(4);

    auto defaultPalette = PaintSystem::getDefaultPalette();
    m_paletteCount = qMin(defaultPalette.size(), 10);
    for (int i = 0; i < m_paletteCount; ++i) {
        QPushButton* btn = new QPushButton(paletteGroup);
        btn->setObjectName("colorSwatch");
        btn->setFixedSize(24, 24);
        btn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; border: 1px solid #333336; border-radius: 3px; }")
                           .arg(defaultPalette[i].color.name()));
        btn->setToolTip(defaultPalette[i].name);
        m_paletteBtns[i] = btn;
        paletteLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            auto palette = PaintSystem::getDefaultPalette();
            if (i < palette.size()) onPaletteColorSelected(palette[i].color);
        });
    }
    paletteLayout->addStretch();

    layout->addWidget(paletteGroup);
}

void PaintEditorWidget::refreshSkinList()
{
    m_skinList->clear();
    const auto names = m_editor->getSkinNames();
    for (const auto& name : names) {
        m_skinList->addItem(name);
    }
}

void PaintEditorWidget::refreshLayerList()
{
    m_layerList->clear();
    const auto& config = m_editor->currentConfig();
    for (int i = 0; i < config.layers.size(); ++i) {
        const auto& layer = config.layers[i];
        QString label = QString("[%1] %2").arg(layer.type).arg(layer.name);
        if (!layer.visible) label += tr(" (hidden)");
        m_layerList->addItem(label);
    }
}

void PaintEditorWidget::onSkinSelected(QListWidgetItem* current)
{
    if (!current) return;
    m_editor->setCurrentSkin(current->text());
    refreshLayerList();
    emit skinSelected(current->text());
}

void PaintEditorWidget::onCreateSkin()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Create Skin"), tr("Skin name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        m_editor->createSkin(name);
    }
}

void PaintEditorWidget::onDeleteSkin()
{
    QListWidgetItem* item = m_skinList->currentItem();
    if (!item) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Delete Skin"),
        tr("Delete skin '%1'?").arg(item->text()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_editor->deleteSkin(item->text());
    }
}

void PaintEditorWidget::onDuplicateSkin()
{
    QListWidgetItem* item = m_skinList->currentItem();
    if (!item) return;

    bool ok;
    QString name = QInputDialog::getText(this, tr("Duplicate Skin"), tr("New name:"), QLineEdit::Normal, item->text() + "_copy", &ok);
    if (ok && !name.isEmpty()) {
        m_editor->duplicateSkin(item->text(), name);
    }
}

void PaintEditorWidget::onLayerSelected(QListWidgetItem* current)
{
    if (!current) {
        clearLayerUI();
        return;
    }

    updateLayerUI();

    int row = m_layerList->currentRow();
    const auto& config = m_editor->currentConfig();

    if (row >= 0 && row < config.layers.size()) {
        const auto& layer = config.layers[row];
        bool isVector = (layer.type == "vector");

        // Block signals so setChecked doesn't trigger onEditModeChanged
        m_editModeGroup->blockSignals(true);
        m_rasterModeRadio->setChecked(!isVector);
        m_vectorModeRadio->setChecked(isVector);
        m_editModeGroup->blockSignals(false);

        m_paintGroup->setVisible(!isVector);
        m_vectorToolsGroup->setVisible(isVector);
        if (m_canvasStack)
            m_canvasStack->setCurrentIndex(isVector ? 2 : 0);

        if (isVector) {
            QJsonDocument doc = QJsonDocument::fromJson(layer.vectorData.toUtf8());
            if (doc.isArray()) {
                m_vectorCanvas->deserializeShapes(doc.array());
            } else {
                m_vectorCanvas->clearAll();
            }
        }
    }
}

void PaintEditorWidget::onAddLayer()
{
    PaintSystem::PaintLayer layer;
    layer.name = QString("layer_%1").arg(m_editor->currentConfig().layers.size() + 1);
    layer.type = m_vectorModeRadio->isChecked() ? "vector" : "decal";
    layer.opacity = 1.0f;
    layer.position[0] = 0.0f;
    layer.position[1] = 0.0f;
    layer.size[0] = 1.0f;
    layer.size[1] = 1.0f;
    layer.visible = true;

    // Record undo
    PaintSystem::UndoAction action;
    action.type = PaintSystem::UndoAction::LayerAdd;
    action.layerIndex = m_editor->currentConfig().layers.size();
    action.newLayer = layer;
    action.description = tr("Add layer: %1").arg(layer.name);
    PaintSystem::pushUndo(action);

    m_editor->addLayer(layer);
    refreshLayerList();
    m_undoBtn->setEnabled(true);
    m_redoBtn->setEnabled(false);
}

void PaintEditorWidget::onRemoveLayer()
{
    int row = m_layerList->currentRow();
    if (row < 0) return;

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        // Record undo
        PaintSystem::UndoAction action;
        action.type = PaintSystem::UndoAction::LayerRemove;
        action.layerIndex = row;
        action.oldLayer = config.layers[row];
        action.description = tr("Remove layer: %1").arg(config.layers[row].name);
        PaintSystem::pushUndo(action);
    }

    m_editor->removeLayer(row);
    refreshLayerList();
    clearLayerUI();
    m_undoBtn->setEnabled(true);
    m_redoBtn->setEnabled(false);
}

void PaintEditorWidget::onMoveLayerUp()
{
    int row = m_layerList->currentRow();
    if (row <= 0) return;
    m_editor->moveLayer(row, row - 1);
    refreshLayerList();
    m_layerList->setCurrentRow(row - 1);
}

void PaintEditorWidget::onMoveLayerDown()
{
    int row = m_layerList->currentRow();
    if (row < 0 || row >= m_editor->currentConfig().layers.size() - 1) return;
    m_editor->moveLayer(row, row + 1);
    refreshLayerList();
    m_layerList->setCurrentRow(row + 1);
}

void PaintEditorWidget::onLayerOpacityChanged(int value)
{
    if (m_updatingUI) return;
    int row = m_layerList->currentRow();
    if (row < 0) return;

    m_opacityLabel->setText(QString("%1%").arg(value));

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        PaintSystem::PaintLayer layer = config.layers[row];
        layer.opacity = value / 100.0f;
        m_editor->updateLayer(row, layer);
    }
}

void PaintEditorWidget::onLayerVisibilityChanged(int state)
{
    if (m_updatingUI) return;
    int row = m_layerList->currentRow();
    if (row < 0) return;

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        PaintSystem::PaintLayer layer = config.layers[row];
        layer.visible = (state != Qt::Unchecked);
        m_editor->updateLayer(row, layer);
        refreshLayerList();
    }
}

void PaintEditorWidget::onBrushTypeChanged(int index)
{
    using paint::PaintTool;
    static const PaintTool toolMap[] = {
        PaintTool::Brush, PaintTool::Airbrush, PaintTool::Brush,   // Brush, Airbrush, Square Brush
        PaintTool::Eraser, PaintTool::Smudge, PaintTool::Blur,     // Eraser, Smudge, Blur
        PaintTool::Sharpen, PaintTool::Clone, PaintTool::Healing,  // Sharpen, Clone, Healing
        PaintTool::Dodge, PaintTool::Burn, PaintTool::BucketFill,  // Dodge, Burn, Fill
        PaintTool::Gradient, PaintTool::Brush                      // Gradient, Stamp
    };
    if (index >= 0 && index < int(sizeof(toolMap) / sizeof(toolMap[0])) && m_paintEditor) {
        m_paintEditor->setCurrentTool(toolMap[index]);
    }

    PaintBrushConfig brush;
    brush.type = static_cast<PaintBrushConfig::Type>(index);
    if (m_painterWidget) m_painterWidget->setBrush(brush);
}

void PaintEditorWidget::onBrushSizeChanged(int size)
{
    m_brushSizeLabel->setText(QString::number(size));
    if (m_paintEditor) m_paintEditor->setBrushSize(size);
}

void PaintEditorWidget::onBrushHardnessChanged(int value)
{
    m_brushHardnessLabel->setText(QString("%1%").arg(value));
    if (m_paintEditor) m_paintEditor->setBrushHardness(value);
}

void PaintEditorWidget::onBrushStrengthChanged(int value)
{
    m_brushStrengthLabel->setText(QString("%1%").arg(value));
    if (m_paintEditor) m_paintEditor->setBrushStrength(value);
}

void PaintEditorWidget::onBrushFlowChanged(int value)
{
    m_brushFlowLabel->setText(QString("%1%").arg(value));
    if (m_paintEditor) m_paintEditor->setBrushFlow(value);
}

void PaintEditorWidget::onColorSelected(bool)
{
    QColor color = QColorDialog::getColor(Qt::red, this, tr("Brush Color"));
    if (color.isValid()) {
        m_colorBtn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; color: #fff; border: 1px solid #333336; border-radius: 3px; }").arg(color.name()));
        if (m_paintEditor) m_paintEditor->setPrimaryColor(color);
        if (m_painterWidget) m_painterWidget->onColorSelected(color);
    }
}

void PaintEditorWidget::onSecondaryColorSelected(bool)
{
    QColor color = QColorDialog::getColor(Qt::white, this, tr("Secondary Color"));
    if (color.isValid()) {
        m_secondaryColorBtn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; color: #111; border: 1px solid #333336; border-radius: 3px; }").arg(color.name()));
    }
}

void PaintEditorWidget::onLicensePlateGenerate()
{
    QString text = m_plateText->text().trimmed();
    QString country = m_plateCountry->currentText();

    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("License Plate"), tr("Please enter plate text."));
        return;
    }

    if (!PaintSystem::isValidPlateText(text, country)) {
        QMessageBox::warning(this, tr("License Plate"), tr("Invalid plate text for selected country."));
        return;
    }

    m_editor->generateLicensePlate(text, country);
    refreshLayerList();
}

void PaintEditorWidget::onSaveSkin()
{
    m_editor->saveCurrentSkin();

    // Persist the paint editor's flattened texture into the skin folder
    if (m_paintEditor && m_paintEditor->hasDocument()) {
        QString skinName = m_editor->currentSkin();
        if (!skinName.isEmpty() && !m_editor->carPath().isEmpty()) {
            QString skinPath = m_editor->carPath() + "/skins/" + skinName;
            m_editor->savePaintTexture(m_paintEditor->currentTexture(), skinPath);
        }
    }
}

void PaintEditorWidget::onRefreshSkins()
{
    m_editor->loadSkins();
}

void PaintEditorWidget::updateLayerUI()
{
    m_updatingUI = true;

    int row = m_layerList->currentRow();
    const auto& config = m_editor->currentConfig();

    if (row >= 0 && row < config.layers.size()) {
        const auto& layer = config.layers[row];
        m_layerNameEdit->setText(layer.name);
        int typeIdx = m_layerTypeCombo->findText(layer.type);
        if (typeIdx >= 0) m_layerTypeCombo->setCurrentIndex(typeIdx);
        m_opacitySlider->setValue(static_cast<int>(layer.opacity * 100));
        m_opacityLabel->setText(QString("%1%").arg(static_cast<int>(layer.opacity * 100)));
        m_visibleCheck->setChecked(layer.visible);
    }

    m_updatingUI = false;
}

void PaintEditorWidget::clearLayerUI()
{
    m_updatingUI = true;
    m_layerNameEdit->clear();
    m_layerTypeCombo->setCurrentIndex(0);
    m_opacitySlider->setValue(100);
    m_opacityLabel->setText("100%");
    m_visibleCheck->setChecked(true);
    m_updatingUI = false;
}

void PaintEditorWidget::onExportDDS()
{
    QString skinName = m_editor->currentSkin();
    if (skinName.isEmpty()) {
        QMessageBox::warning(this, tr("Export DDS"), tr("No skin selected."));
        return;
    }

    QString carPath = m_editor->carPath();
    QString skinPath = carPath + "/skins/" + skinName;

    QString outputPath = QFileDialog::getSaveFileName(this, tr("Export Paint as DDS"),
        skinPath + "/paint.dds",
        tr("DDS files (*.dds)"));
    if (outputPath.isEmpty()) return;

    // If paint editor is active, export its current texture directly
    if (m_paintEditor && m_canvasStack->currentIndex() == 0 && m_paintEditor->hasDocument()) {
        QImage texture = m_paintEditor->currentTexture();
        if (PaintSystem::saveTextureAsDDS(texture, outputPath)) {
            QMessageBox::information(this, tr("Export DDS"), tr("Paint exported as DDS:\n%1").arg(outputPath));
        } else {
            QMessageBox::warning(this, tr("Export DDS"), tr("Failed to export DDS."));
        }
        return;
    }

    // Legacy path for painter/vector canvas
    if (PaintSystem::exportSkinAsDDS(skinPath, outputPath)) {
        QMessageBox::information(this, tr("Export DDS"), tr("Paint exported as DDS:\n%1").arg(outputPath));
    } else {
        QMessageBox::warning(this, tr("Export DDS"), tr("Failed to export DDS. Check that a paint texture exists."));
    }
}

void PaintEditorWidget::onImportDecal()
{
    QString decalPath = QFileDialog::getOpenFileName(this, tr("Import Decal"),
        QString(),
        PaintSystem::getSupportedDecalFormats().join(";;"));
    if (decalPath.isEmpty()) return;

    QString skinName = m_editor->currentSkin();
    if (skinName.isEmpty()) {
        QMessageBox::warning(this, tr("Import Decal"), tr("No skin selected."));
        return;
    }

    QString skinPath = m_editor->carPath() + "/skins/" + skinName;
    if (!PaintSystem::importDecal(decalPath, skinPath)) {
        QMessageBox::warning(this, tr("Import Decal"), tr("Failed to import decal."));
        return;
    }

    QFileInfo fi(decalPath);
    PaintSystem::PaintLayer layer;
    layer.name = fi.completeBaseName();
    layer.type = "decal";
    layer.opacity = 1.0f;
    layer.texturePath = skinPath + "/" + fi.fileName();
    layer.size[0] = 0.3f;
    layer.size[1] = 0.3f;
    layer.visible = true;

    m_editor->addLayer(layer);
    refreshLayerList();
    QMessageBox::information(this, tr("Import Decal"), tr("Decal imported: %1").arg(fi.fileName()));
}

void PaintEditorWidget::onCreateFromTemplate()
{
    auto templates = PaintSystem::getBuiltinTemplates();
    QStringList names;
    for (const auto& t : templates) names << t.name;

    bool ok;
    QString selected = QInputDialog::getItem(this, tr("Create from Template"),
        tr("Choose paint template:"), names, 0, false, &ok);
    if (!ok || selected.isEmpty()) return;

    int idx = names.indexOf(selected);
    if (idx < 0 || idx >= templates.size()) return;

    QString skinName = QInputDialog::getText(this, tr("Skin Name"),
        tr("Enter name for new skin:"), QLineEdit::Normal, selected.replace(" ", "_"), &ok);
    if (!ok || skinName.isEmpty()) return;

    QString carPath = m_editor->carPath();
    if (carPath.isEmpty()) {
        QMessageBox::warning(this, tr("Template"), tr("No car loaded. Set car path first."));
        return;
    }

    PaintSystem::createSkinFromTemplate(carPath, skinName, templates[idx]);
    m_editor->loadSkins();
    refreshSkinList();
    QMessageBox::information(this, tr("Template"), tr("Skin created from template: %1").arg(skinName));
}

void PaintEditorWidget::onUndo()
{
    // If paint editor is active, use its undo
    if (m_paintEditor && m_canvasStack->currentIndex() == 0 && m_paintEditor->canUndo()) {
        m_paintEditor->undo();
        m_undoBtn->setEnabled(m_paintEditor->canUndo());
        m_redoBtn->setEnabled(m_paintEditor->canRedo());
        return;
    }

    // Legacy PaintSystem undo for painter/vector canvas
    if (!PaintSystem::canUndo()) return;

    auto action = PaintSystem::undoLast();
    auto& config = m_editor->currentConfig();

    switch (action.type) {
    case PaintSystem::UndoAction::LayerAdd:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers.removeAt(action.layerIndex);
        }
        break;
    case PaintSystem::UndoAction::LayerRemove:
        if (action.layerIndex >= 0) {
            config.layers.insert(action.layerIndex, action.oldLayer);
        }
        break;
    case PaintSystem::UndoAction::LayerModify:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers[action.layerIndex] = action.oldLayer;
        }
        break;
    case PaintSystem::UndoAction::LayerMove:
        break;
    default:
        break;
    }

    m_editor->saveCurrentSkin();
    refreshLayerList();
    m_undoBtn->setEnabled(PaintSystem::canUndo());
    m_redoBtn->setEnabled(PaintSystem::canRedo());
}

void PaintEditorWidget::onRedo()
{
    // If paint editor is active, use its redo
    if (m_paintEditor && m_canvasStack->currentIndex() == 0 && m_paintEditor->canRedo()) {
        m_paintEditor->redo();
        m_undoBtn->setEnabled(m_paintEditor->canUndo());
        m_redoBtn->setEnabled(m_paintEditor->canRedo());
        return;
    }

    // Legacy PaintSystem redo for painter/vector canvas
    if (!PaintSystem::canRedo()) return;

    auto action = PaintSystem::redoLast();
    auto& config = m_editor->currentConfig();

    switch (action.type) {
    case PaintSystem::UndoAction::LayerAdd:
        if (action.layerIndex >= 0) {
            config.layers.insert(action.layerIndex, action.newLayer);
        }
        break;
    case PaintSystem::UndoAction::LayerRemove:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers.removeAt(action.layerIndex);
        }
        break;
    case PaintSystem::UndoAction::LayerModify:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers[action.layerIndex] = action.newLayer;
        }
        break;
    default:
        break;
    }

    m_editor->saveCurrentSkin();
    refreshLayerList();
    m_undoBtn->setEnabled(PaintSystem::canUndo());
    m_redoBtn->setEnabled(PaintSystem::canRedo());
}

void PaintEditorWidget::onPaletteColorSelected(const QColor& color)
{
    Q_UNUSED(color);
    // Apply selected palette color to the brush
    m_colorBtn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; color: #fff; border: 1px solid #333336; border-radius: 3px; }")
                               .arg(color.name()));
    if (m_paintEditor) m_paintEditor->setPrimaryColor(color);
}

// Wrapper slots for ribbon button connections (accept bool from clicked signal)
void PaintEditorWidget::onMergeLayerDown(bool)
{
    if (m_paintEditor && m_paintEditor->document()) {
        m_paintEditor->document()->mergeDown(m_paintEditor->document()->currentLayerIndex());
    }
}

void PaintEditorWidget::onFlattenImage(bool)
{
    if (m_paintEditor) m_paintEditor->onExportPng(); // Placeholder - no flatten yet
}

void PaintEditorWidget::onSwapColors(bool)
{
    if (m_paintEditor) m_paintEditor->swapColors();
}

void PaintEditorWidget::onResetColors(bool)
{
    if (m_paintEditor) m_paintEditor->resetColors();
}

void PaintEditorWidget::onSelectNone(bool)
{
    if (m_paintEditor) m_paintEditor->onSelectNone();
}

void PaintEditorWidget::onSelectInvert(bool)
{
    if (m_paintEditor) m_paintEditor->onSelectInvert();
}

void PaintEditorWidget::onZoomIn(bool)
{
    if (m_paintEditor) m_paintEditor->onZoomIn();
}

void PaintEditorWidget::onZoomOut(bool)
{
    if (m_paintEditor) m_paintEditor->onZoomOut();
}

void PaintEditorWidget::onZoomFit(bool)
{
    if (m_paintEditor) m_paintEditor->onZoomFit();
}

void PaintEditorWidget::onZoomTool(bool)
{
    onBrushTypeChanged(2); // Zoom tool
}

// ══════════════════════════════════════════════════════════════════════
// Vector Design Tool Slots
// ══════════════════════════════════════════════════════════════════════

void PaintEditorWidget::onEditModeChanged(int mode)
{
    bool isVector = (mode == 1);

    m_paintGroup->setVisible(!isVector);
    m_vectorToolsGroup->setVisible(isVector);
    if (m_canvasStack)
        m_canvasStack->setCurrentIndex(isVector ? 2 : 0);

    if (isVector) {
        // Vector mode: sync current layer's vector data to canvas
        int row = m_layerList->currentRow();
        if (row >= 0) {
            const auto& config = m_editor->currentConfig();
            if (row < config.layers.size() && config.layers[row].type == "vector") {
                QJsonDocument doc = QJsonDocument::fromJson(config.layers[row].vectorData.toUtf8());
                if (doc.isArray()) {
                    m_vectorCanvas->deserializeShapes(doc.array());
                }
            }
        }
    } else {
        // Raster mode: save vector layer data
        syncVectorLayerData();
    }

    emit paintModified();
}

void PaintEditorWidget::onVectorToolChanged(int index)
{
    m_vectorCanvas->setActiveTool(static_cast<VectorDesignCanvas::Tool>(index));
}

void PaintEditorWidget::onVectorFillColor()
{
    QColor color = QColorDialog::getColor(m_vectorCanvas->fillColor(), this, tr("Fill Color"));
    if (color.isValid()) {
        m_vectorCanvas->setFillColor(color);
        m_vectorFillColorBtn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; color: #fff; border: 1px solid #333336; border-radius: 3px; }").arg(color.name()));
    }
}

void PaintEditorWidget::onVectorStrokeColor()
{
    QColor color = QColorDialog::getColor(m_vectorCanvas->strokeColor(), this, tr("Stroke Color"));
    if (color.isValid()) {
        m_vectorCanvas->setStrokeColor(color);
        m_vectorStrokeColorBtn->setStyleSheet(QString("QPushButton#colorSwatch { background: %1; color: #fff; border: 1px solid #333336; border-radius: 3px; }").arg(color.name()));
    }
}

void PaintEditorWidget::onVectorStrokeWidthChanged(int value)
{
    m_vectorStrokeWidthLabel->setText(QString("%1px").arg(value));
    m_vectorCanvas->setStrokeWidth(value);
}

void PaintEditorWidget::onVectorDrawFilledChanged(int state)
{
    m_vectorCanvas->setDrawFilled(state == Qt::Checked);
}

void PaintEditorWidget::onVectorDeleteSelected()
{
    m_vectorCanvas->deleteSelected();
}

void PaintEditorWidget::onVectorClearAll()
{
    m_vectorCanvas->clearAll();
}

void PaintEditorWidget::onVectorShapesChanged()
{
    syncVectorLayerData();
    emit paintModified();
}

void PaintEditorWidget::syncVectorLayerData()
{
    int row = m_layerList->currentRow();
    if (row < 0) return;

    auto& config = m_editor->currentConfig();
    if (row >= config.layers.size()) return;

    if (config.layers[row].type == "vector") {
        QJsonArray shapes = m_vectorCanvas->serializeShapes();
        QJsonDocument doc(shapes);
        config.layers[row].vectorData = QString::fromUtf8(doc.toJson());
    }
}

} // namespace ks