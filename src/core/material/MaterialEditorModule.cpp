#include "MaterialEditorModule.h"
#include "ShaderGraphWidget.h"
#include "core/editor/ModuleGuiBase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColorDialog>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QProcess>
#include <QStandardPaths>
#include <QPixmap>
#include <QMimeData>

namespace ks {
namespace material {

MaterialEditorModule::MaterialEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_pbrTab(nullptr)
    , m_materialTypeCombo(nullptr)
    , m_shaderCombo(nullptr)
    , m_parameterTree(nullptr)
    , m_saveBtn(nullptr)
    , m_loadBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_previewBtn(nullptr)
    , m_livePreviewCheck(nullptr)
    , m_shaderGraphTab(nullptr)
    , m_shaderGraphSplitter(nullptr)
    , m_nodePalette(nullptr)
    , m_shaderGraphWidget(nullptr)
    , m_graphProps(nullptr)
    , m_texturePaintTab(nullptr)
    , m_brushToolCombo(nullptr)
    , m_textureSlotCombo(nullptr)
    , m_brushSizeSpin(nullptr)
    , m_brushStrengthSpin(nullptr)
    , m_paintBtn(nullptr)
    , m_eraseBtn(nullptr)
    , m_fillBtn(nullptr)
    , m_bakeBtn(nullptr)
    , m_presetsTab(nullptr)
    , m_presetList(nullptr)
    , m_applyPresetBtn(nullptr)
    , m_saveAsPresetBtn(nullptr)
    , m_deletePresetBtn(nullptr)
    , m_presetDesc(nullptr)
    , m_currentMaterialPath("")
    , m_currentPreset("")
    , m_livePreview(false)
{
    setObjectName("MaterialEditorModule");
}

bool MaterialEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    loadPresets();
    return true;
}

void MaterialEditorModule::shutdown() {
    savePresets();
    ModuleGuiBase::shutdown();
}

void MaterialEditorModule::importFile(const QString& filePath) {
    QFileInfo info(filePath);
    if (info.suffix().toLower() == "mat" || info.suffix().toLower() == "json") {
        loadMaterial(filePath);
    } else {
        logError(QString("Unsupported file format: %1").arg(info.suffix()));
    }
}

void MaterialEditorModule::exportFile(const QString& filePath) {
    saveMaterial(filePath);
}

void MaterialEditorModule::buildUI() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );
    
    setupMaterialTabs();
    m_mainLayout->insertWidget(1, m_tabWidget, 1);
}

void MaterialEditorModule::setupMaterialTabs() {
    setupPbrTab();
    setupShaderGraphTab();
    setupTexturePaintTab();
    setupPresetsTab();
    
    m_tabWidget->addTab(m_pbrTab, "PBR Material");
    m_tabWidget->addTab(m_shaderGraphTab, "Shader Graph");
    m_tabWidget->addTab(m_texturePaintTab, "Texture Paint");
    m_tabWidget->addTab(m_presetsTab, "Presets");
}

void MaterialEditorModule::setupPbrTab() {
    m_pbrTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_pbrTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Material type and shader selection
    QGroupBox* typeGroup = createGroupBox("Material Type");
    QHBoxLayout* typeLayout = new QHBoxLayout(typeGroup);
    
    typeLayout->addWidget(createLabel("Type:"));
    m_materialTypeCombo = createComboBox({"Standard PBR", "Car Paint", "Glass", "Chrome", "Carbon Fiber", "Fabric", "Rubber", "Custom"});
    connect(m_materialTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaterialEditorModule::onMaterialTypeChanged);
    typeLayout->addWidget(m_materialTypeCombo);
    
    typeLayout->addWidget(createLabel("Shader:"));
    m_shaderCombo = createComboBox({"ksPerPixel", "ksPerPixelNM", "ksPerPixelAT", "ksPerPixelMultiMap", "ksSkinnedMesh", "ksSimpleShader", "Custom"});
    connect(m_shaderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaterialEditorModule::onShaderChanged);
    typeLayout->addWidget(m_shaderCombo);
    
    typeLayout->addStretch();
    layout->addWidget(typeGroup);
    
    // Main content splitter
    QSplitter* mainSplitter = createSplitter();
    
    // Left: Parameter tree
    QGroupBox* paramGroup = createGroupBox("Parameters");
    QVBoxLayout* paramLayout = new QVBoxLayout(paramGroup);
    
    m_parameterTree = createTreeWidget({"Parameter", "Type", "Value"});
    m_parameterTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_parameterTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_parameterTree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    paramLayout->addWidget(m_parameterTree);
    
    mainSplitter->addWidget(paramGroup);
    
    // Right: Texture slots and preview
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    QGroupBox* textureGroup = createGroupBox("Texture Slots");
    QVBoxLayout* textureLayout = new QVBoxLayout(textureGroup);
    
    QStringList textureSlots = {"Albedo", "Normal", "Roughness", "Metalness", "AO", "Emissive", "Detail Albedo", "Detail Normal"};
    for (const QString& slot : textureSlots) {
        QHBoxLayout* slotLayout = new QHBoxLayout();
        slotLayout->addWidget(createLabel(slot + ":"));
        QPushButton* texBtn = createButton("None");
        texBtn->setProperty("slot", slot);
        texBtn->setMaximumWidth(120);
        connect(texBtn, &QPushButton::clicked, this, [this, slot]() { onTextureSelected(slot); });
        slotLayout->addWidget(texBtn);
        slotLayout->addStretch();
        textureLayout->addLayout(slotLayout);
    }
    rightLayout->addWidget(textureGroup);
    
    // Preview controls
    QGroupBox* previewGroup = createGroupBox("Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    
    m_livePreviewCheck = createCheckBox("Live Preview", true);
    m_livePreview = true;
    connect(m_livePreviewCheck, &QCheckBox::toggled, this, &MaterialEditorModule::onPreviewToggle);
    previewLayout->addWidget(m_livePreviewCheck);
    
    QHBoxLayout* previewBtnLayout = new QHBoxLayout();
    m_previewBtn = createButton("Refresh Preview");
    connect(m_previewBtn, &QPushButton::clicked, this, &MaterialEditorModule::updateMaterialPreview);
    previewBtnLayout->addWidget(m_previewBtn);
    previewBtnLayout->addStretch();
    previewLayout->addLayout(previewBtnLayout);
    
    rightLayout->addWidget(previewGroup);
    rightLayout->addStretch();
    
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);
    
    layout->addWidget(mainSplitter, 1);
    
    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    m_saveBtn = createButton("Save Material", "success");
    connect(m_saveBtn, &QPushButton::clicked, this, &MaterialEditorModule::onSaveMaterial);
    actionLayout->addWidget(m_saveBtn);
    
    m_loadBtn = createButton("Load Material");
    connect(m_loadBtn, &QPushButton::clicked, this, &MaterialEditorModule::onLoadMaterial);
    actionLayout->addWidget(m_loadBtn);
    
    m_exportBtn = createButton("Export Shader", "warning");
    connect(m_exportBtn, &QPushButton::clicked, this, &MaterialEditorModule::onExportShader);
    actionLayout->addWidget(m_exportBtn);
    
    actionLayout->addStretch();
    layout->addLayout(actionLayout);
    
    // Initialize with default parameters
    onMaterialTypeChanged(0);
}

void MaterialEditorModule::setupShaderGraphTab() {
    m_shaderGraphTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_shaderGraphTab);
    layout->setContentsMargins(4, 4, 4, 4);

    m_shaderGraphSplitter = createSplitter();

    // Left: Node palette
    QGroupBox* paletteGroup = createGroupBox("Node Palette");
    QVBoxLayout* paletteLayout = new QVBoxLayout(paletteGroup);

    m_nodePalette = createTreeWidget({"Nodes"});
    m_nodePalette->setHeaderHidden(true);
    m_nodePalette->setDragEnabled(true);

    QStringList categories = {"Inputs", "Math", "Vector", "Texture", "Color", "Output"};
    QStringList inputNodes = {"Position", "Normal", "UV", "View Direction", "Light Direction", "Time", "Float", "Vector2", "Vector3", "Vector4", "Color"};
    QStringList mathNodes = {"Add", "Subtract", "Multiply", "Divide", "Power", "Sqrt", "Sin", "Cos", "Tan", "Abs", "Min", "Max", "Clamp", "Saturate", "Lerp", "Step", "Smoothstep"};
    QStringList vectorNodes = {"Dot", "Cross", "Normalize", "Length", "Distance", "Reflect", "Refract", "Face Forward", "Swizzle", "Combine", "Split"};
    QStringList textureNodes = {"Sample 2D", "Sample Cube", "Sample 3D", "Texture Coords", "Triplanar"};
    QStringList colorNodes = {"Gamma", "Linear", "HSV to RGB", "RGB to HSV", "Brightness", "Contrast", "Saturation", "Hue Shift", "Color Ramp"};
    QStringList outputNodes = {"Surface Output", "Emission", "Alpha", "Normal Output", "Displacement"};

    QMap<QString, QStringList> nodeMap = {
        {"Inputs", inputNodes}, {"Math", mathNodes}, {"Vector", vectorNodes},
        {"Texture", textureNodes}, {"Color", colorNodes}, {"Output", outputNodes}
    };

    for (const QString& cat : categories) {
        QTreeWidgetItem* catItem = new QTreeWidgetItem(m_nodePalette, QStringList() << cat);
        catItem->setExpanded(true);
        for (const QString& node : nodeMap[cat]) {
            QTreeWidgetItem* nodeItem = new QTreeWidgetItem(catItem, QStringList() << node);
            nodeItem->setData(0, Qt::UserRole, cat + "::" + node);
        }
    }
    m_nodePalette->expandAll();
    paletteLayout->addWidget(m_nodePalette);

    m_shaderGraphSplitter->addWidget(paletteGroup);

    // Center: Live shader graph canvas
    m_shaderGraphWidget = new ShaderGraphWidget();
    m_shaderGraphWidget->setNodePalette(m_nodePalette);
    m_shaderGraphWidget->setGraph(QUuid());  // creates default graph

    connect(m_shaderGraphWidget, &ShaderGraphWidget::graphChanged, this, [this]() {
        if (m_livePreview) updateMaterialPreview();
    });
    connect(m_shaderGraphWidget, &ShaderGraphWidget::statusMessage, this, [this](const QString& msg) {
        log(msg);
    });

    m_shaderGraphSplitter->addWidget(m_shaderGraphWidget);

    // Right: Node properties
    m_graphProps = new QWidget();
    QVBoxLayout* propsLayout = new QVBoxLayout(m_graphProps);

    QGroupBox* propsGroup = createGroupBox("Node Properties");
    QVBoxLayout* propsGroupLayout = new QVBoxLayout(propsGroup);

    QTreeWidget* nodeProps = createTreeWidget({"Property", "Value"});
    nodeProps->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    propsGroupLayout->addWidget(nodeProps);

    propsLayout->addWidget(propsGroup);
    propsLayout->addStretch();

    m_shaderGraphSplitter->addWidget(m_graphProps);
    m_shaderGraphSplitter->setStretchFactor(0, 1);
    m_shaderGraphSplitter->setStretchFactor(1, 3);
    m_shaderGraphSplitter->setStretchFactor(2, 1);

    layout->addWidget(m_shaderGraphSplitter, 1);

    // Compile/Export buttons
    QHBoxLayout* shaderBtnLayout = new QHBoxLayout();
    QPushButton* compileBtn = createButton("Compile Shader", "success");
    connect(compileBtn, &QPushButton::clicked, m_shaderGraphWidget, &ShaderGraphWidget::onCompile);
    shaderBtnLayout->addWidget(compileBtn);

    QPushButton* validateBtn = createButton("Validate");
    connect(validateBtn, &QPushButton::clicked, m_shaderGraphWidget, &ShaderGraphWidget::onValidate);
    shaderBtnLayout->addWidget(validateBtn);

    QPushButton* exportBtn = createButton("Export HLSL", "primary");
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        if (!m_shaderGraphWidget || m_shaderGraphWidget->graphId().isNull()) return;
        QString path = selectFile("Export Shader", "HLSL (*.hlsl);;GLSL (*.glsl);;Metal (*.metal)", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (path.isEmpty()) return;

        ShaderExportOptions opts;
        if (path.endsWith(".hlsl")) opts.target = ShaderExportOptions::Target::HLSL;
        else if (path.endsWith(".glsl")) opts.target = ShaderExportOptions::Target::GLSL;
        else if (path.endsWith(".metal")) opts.target = ShaderExportOptions::Target::Metal;
        opts.outputDirectory = QFileInfo(path).absolutePath();

        if (ShaderExporter::instance()->exportGraph(m_shaderGraphWidget->graphId(), opts)) {
            logSuccess("Shader exported to: " + path);
        } else {
            logError("Shader export failed");
        }
    });
    shaderBtnLayout->addWidget(exportBtn);

    shaderBtnLayout->addStretch();
    layout->addLayout(shaderBtnLayout);
}

void MaterialEditorModule::setupTexturePaintTab() {
    m_texturePaintTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_texturePaintTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Toolbar
    QGroupBox* toolGroup = createGroupBox("Paint Tools");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    
    toolLayout->addWidget(createLabel("Tool:"));
    m_brushToolCombo = createComboBox({"Brush", "Eraser", "Fill", "Gradient", "Clone", "Blur", "Sharpen", "Dodge", "Burn"});
    toolLayout->addWidget(m_brushToolCombo);
    
    toolLayout->addWidget(createLabel("Slot:"));
    m_textureSlotCombo = createComboBox({"Albedo", "Normal", "Roughness", "Metalness", "AO", "Emissive", "Height"});
    toolLayout->addWidget(m_textureSlotCombo);
    
    toolLayout->addWidget(createLabel("Size:"));
    m_brushSizeSpin = createSpinBox(1, 512, 32, " px");
    m_brushSizeSpin->setMaximumWidth(80);
    toolLayout->addWidget(m_brushSizeSpin);
    
    toolLayout->addWidget(createLabel("Strength:"));
    m_brushStrengthSpin = createDoubleSpinBox(0.01, 1.0, 0.5, 2);
    m_brushStrengthSpin->setMaximumWidth(80);
    toolLayout->addWidget(m_brushStrengthSpin);
    
    // Update preview when slot changes
    connect(m_textureSlotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        m_currentTexturePaintSlot = m_textureSlotCombo->currentText();
        if (m_textureImages.contains(m_currentTexturePaintSlot)) {
            m_paintPreview->setPixmap(QPixmap::fromImage(m_textureImages[m_currentTexturePaintSlot]));
        }
    });
    
    toolLayout->addStretch();
    layout->addWidget(toolGroup);
    
    // Main area
    QSplitter* paintSplitter = createSplitter();
    
    // Texture paint canvas
    m_currentTexturePaintSlot = "Albedo";
    m_paintPreview = new QLabel();
    m_paintPreview->setMinimumSize(512, 512);
    m_paintPreview->setStyleSheet("background: #1a1a1a; border: 1px solid #3a3a3a;");
    m_paintPreview->setAlignment(Qt::AlignCenter);
    // Initialize paint textures
    QStringList texSlots = {"Albedo", "Normal", "Roughness", "Metalness", "AO", "Emissive", "Height"};
    for (const QString& slot : texSlots) {
        m_textureImages[slot] = QImage(512, 512, QImage::Format_ARGB32);
        m_textureImages[slot].fill(qRgba(128, 128, 128, 255));
    }
    // Paint a checkerboard on Albedo as default
    QPainter checker(&m_textureImages["Albedo"]);
    for (int y = 0; y < 512; y += 32) {
        for (int x = 0; x < 512; x += 32) {
            if ((x / 32 + y / 32) % 2 == 0)
                checker.fillRect(x, y, 32, 32, qRgb(180, 180, 180));
            else
                checker.fillRect(x, y, 32, 32, qRgb(140, 140, 140));
        }
    }
    checker.end();
    m_paintPreview->setPixmap(QPixmap::fromImage(m_textureImages["Albedo"]));
    paintSplitter->addWidget(m_paintPreview);
    
    // Layers panel
    QWidget* layersWidget = new QWidget();
    QVBoxLayout* layersLayout = new QVBoxLayout(layersWidget);
    
    QGroupBox* layersGroup = createGroupBox("Layers");
    QVBoxLayout* layersGroupLayout = new QVBoxLayout(layersGroup);
    
    QTreeWidget* layerTree = createTreeWidget({"Layer", "Mode", "Opacity", "Visible"});
    layerTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    layerTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layerTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layerTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    
    // Add default layer
    QTreeWidgetItem* baseLayer = new QTreeWidgetItem(layerTree, {"Base", "Normal", "100%", "âœ“"});
    baseLayer->setFlags(baseLayer->flags() | Qt::ItemIsUserCheckable);
    baseLayer->setCheckState(3, Qt::Checked);
    
    QPushButton* addLayerBtn = createButton("+ Add Layer", "success");
    connect(addLayerBtn, &QPushButton::clicked, this, [layerTree]() {
        int count = layerTree->topLevelItemCount();
        QTreeWidgetItem* item = new QTreeWidgetItem(layerTree, {QString("Layer %1").arg(count), "Normal", "100%", "âœ“"});
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(3, Qt::Checked);
    });
    layersGroupLayout->addWidget(layerTree);
    layersGroupLayout->addWidget(addLayerBtn);
    
    layersLayout->addWidget(layersGroup);
    
    // Brush settings
    QGroupBox* brushGroup = createGroupBox("Brush Settings");
    QFormLayout* brushLayout = new QFormLayout(brushGroup);
    
    QDoubleSpinBox* spacingSpin = createDoubleSpinBox(0.01, 1.0, 0.1, 2);
    brushLayout->addRow("Spacing:", spacingSpin);
    
    QDoubleSpinBox* jitterSpin = createDoubleSpinBox(0, 1.0, 0, 2);
    brushLayout->addRow("Jitter:", jitterSpin);
    
    QCheckBox* pressureCheck = createCheckBox("Pressure Sensitivity", true);
    brushLayout->addRow(pressureCheck);
    
    QCheckBox* symmetryCheck = createCheckBox("Mirror X", false);
    brushLayout->addRow(symmetryCheck);
    
    QCheckBox* symmetryYCheck = createCheckBox("Mirror Y", false);
    brushLayout->addRow(symmetryYCheck);
    
    layersLayout->addWidget(brushGroup);
    layersLayout->addStretch();
    
    paintSplitter->addWidget(layersWidget);
    paintSplitter->setStretchFactor(0, 3);
    paintSplitter->setStretchFactor(1, 1);
    
    layout->addWidget(paintSplitter, 1);
    
    // Action buttons
    QHBoxLayout* paintBtnLayout = new QHBoxLayout();
    m_paintBtn = createButton("Paint", "success");
    connect(m_paintBtn, &QPushButton::clicked, this, [this]() {
        m_currentTexturePaintSlot = m_textureSlotCombo->currentText();
        QImage& img = m_textureImages[m_currentTexturePaintSlot];
        QPainter painter(&img);
        int brushSize = m_brushSizeSpin->value();
        int cx = img.width() / 2, cy = img.height() / 2;
        QColor paintColor(220, 50, 50);
        painter.setBrush(paintColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPoint(cx, cy), brushSize / 2, brushSize / 2);
        painter.end();
        m_paintPreview->setPixmap(QPixmap::fromImage(img));
        log(QString("Painted on %1 slot").arg(m_currentTexturePaintSlot));
    });
    paintBtnLayout->addWidget(m_paintBtn);
    
    m_eraseBtn = createButton("Erase", "warning");
    connect(m_eraseBtn, &QPushButton::clicked, this, [this]() {
        m_currentTexturePaintSlot = m_textureSlotCombo->currentText();
        QImage& img = m_textureImages[m_currentTexturePaintSlot];
        QPainter painter(&img);
        int brushSize = m_brushSizeSpin->value();
        int cx = img.width() / 2, cy = img.height() / 2;
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.drawEllipse(QPoint(cx, cy), brushSize / 2, brushSize / 2);
        painter.end();
        m_paintPreview->setPixmap(QPixmap::fromImage(img));
        log(QString("Erased on %1 slot").arg(m_currentTexturePaintSlot));
    });
    paintBtnLayout->addWidget(m_eraseBtn);
    
    m_fillBtn = createButton("Fill");
    connect(m_fillBtn, &QPushButton::clicked, this, [this]() {
        m_currentTexturePaintSlot = m_textureSlotCombo->currentText();
        QImage& img = m_textureImages[m_currentTexturePaintSlot];
        img.fill(qRgba(160, 160, 160, 255));
        m_paintPreview->setPixmap(QPixmap::fromImage(img));
        log(QString("Filled %1 slot").arg(m_currentTexturePaintSlot));
    });
    paintBtnLayout->addWidget(m_fillBtn);
    
    m_bakeBtn = createButton("Bake Textures", "primary");
    connect(m_bakeBtn, &QPushButton::clicked, this, [this]() {
        QString bakeDir = selectFile("Select Bake Output Directory", "", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (bakeDir.isEmpty()) return;
        QStringList texSlots = {"Albedo", "Normal", "Roughness", "Metalness", "AO", "Emissive", "Height"};
        int baked = 0;
        for (const QString& slot : texSlots) {
            if (m_textureImages.contains(slot) && !m_textureImages[slot].isNull()) {
                QString path = bakeDir + "/" + slot.toLower() + ".png";
                if (m_textureImages[slot].save(path, "PNG")) baked++;
            }
        }
        logSuccess(QString("Baked %1/%2 textures to %3").arg(baked).arg(texSlots.size()).arg(bakeDir));
    });
    paintBtnLayout->addWidget(m_bakeBtn);
    
    paintBtnLayout->addStretch();
    layout->addLayout(paintBtnLayout);
}

void MaterialEditorModule::setupPresetsTab() {
    m_presetsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_presetsTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QSplitter* presetSplitter = createSplitter();
    
    // Left: Preset list
    QGroupBox* listGroup = createGroupBox("Material Presets");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);
    
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->addWidget(createLabel("Filter:"));
    QComboBox* filterCombo = createComboBox({"All", "Car Paint", "Metals", "Glass", "Fabric", "Rubber", "Organic", "Custom"});
    filterLayout->addWidget(filterCombo);
    filterLayout->addStretch();
    listLayout->addLayout(filterLayout);
    
    m_presetList = createListWidget();
    listLayout->addWidget(m_presetList);
    
    QHBoxLayout* presetBtnLayout = new QHBoxLayout();
    m_applyPresetBtn = createButton("Apply", "success");
    connect(m_applyPresetBtn, &QPushButton::clicked, this, [this]() {
        if (m_presetList->currentItem()) {
            applyPreset(m_presetList->currentItem()->text());
        }
    });
    presetBtnLayout->addWidget(m_applyPresetBtn);
    
    m_saveAsPresetBtn = createButton("Save as Preset");
    connect(m_saveAsPresetBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Save Preset", "Preset name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            saveCurrentAsPreset(name);
        }
    });
    presetBtnLayout->addWidget(m_saveAsPresetBtn);
    
    m_deletePresetBtn = createButton("Delete", "danger");
    connect(m_deletePresetBtn, &QPushButton::clicked, this, [this]() {
        if (m_presetList->currentItem()) {
            deletePreset(m_presetList->currentItem()->text());
        }
    });
    presetBtnLayout->addWidget(m_deletePresetBtn);
    
    listLayout->addLayout(presetBtnLayout);
    presetSplitter->addWidget(listGroup);
    
    // Right: Preset details
    QGroupBox* descGroup = createGroupBox("Description");
    QVBoxLayout* descLayout = new QVBoxLayout(descGroup);
    
    m_presetDesc = new QTextEdit();
    m_presetDesc->setReadOnly(true);
    m_presetDesc->setStyleSheet("QTextEdit { background: #1a1a1a; color: #c8c8c8; font-size: 11px; border: 1px solid #3a3a3a; }");
    descLayout->addWidget(m_presetDesc);
    
    QGroupBox* previewGroup = createGroupBox("Preview");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    QLabel* previewLabel = createLabel("Material Preview\n(Shader ball)", "color: #666; font-size: 14px;");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(200);
    previewLabel->setStyleSheet("border: 1px solid #3a3a3a; background: #1a1a1a; border-radius: 4px;");
    previewLayout->addWidget(previewLabel);
    
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->addWidget(descGroup);
    rightLayout->addWidget(previewGroup, 1);
    
    QWidget* rightWidget = new QWidget();
    rightWidget->setLayout(rightLayout);
    presetSplitter->addWidget(rightWidget);
    presetSplitter->setStretchFactor(0, 1);
    presetSplitter->setStretchFactor(1, 1);
    
    layout->addWidget(presetSplitter, 1);
    
    refreshPresetList();
}

void MaterialEditorModule::onMaterialTypeChanged(int index) {
    m_parameters.clear();
    m_parameterTree->clear();
    
    QString type = m_materialTypeCombo->currentText();
    
    if (type == "Standard PBR") {
        m_parameters = {
            {"Albedo Color", "color", QColor(180, 180, 180)},
            {"Metalness", "float", 0.0},
            {"Roughness", "float", 0.5},
            {"AO", "float", 1.0},
            {"Normal Scale", "float", 1.0},
            {"Height Scale", "float", 0.1},
            {"Emission Color", "color", QColor(0, 0, 0)},
            {"Emission Strength", "float", 0.0},
            {"Alpha", "float", 1.0},
            {"Alpha Cutoff", "float", 0.5},
            {"Double Sided", "bool", false},
            {"Receive Shadows", "bool", true},
            {"Cast Shadows", "bool", true}
        };
    } else if (type == "Car Paint") {
        m_parameters = {
            {"Base Color", "color", QColor(200, 30, 30)},
            {"Flake Color", "color", QColor(255, 200, 100)},
            {"Flake Density", "float", 0.5},
            {"Flake Scale", "float", 1.0},
            {"Flake Spread", "float", 0.5},
            {"Clearcoat", "float", 1.0},
            {"Clearcoat Roughness", "float", 0.03},
            {"Metalness", "float", 0.8},
            {"Roughness", "float", 0.3},
            {"AO", "float", 1.0}
        };
    } else if (type == "Glass") {
        m_parameters = {
            {"Tint Color", "color", QColor(200, 220, 255)},
            {"IOR", "float", 1.52},
            {"Thickness", "float", 2.0},
            {"Attenuation Distance", "float", 10.0},
            {"Attenuation Color", "color", QColor(255, 255, 255)},
            {"Roughness", "float", 0.0},
            {"Metalness", "float", 0.0},
            {"Alpha", "float", 0.9}
        };
    } else if (type == "Chrome") {
        m_parameters = {
            {"Reflectivity", "float", 1.0},
            {"Roughness", "float", 0.0},
            {"Tint Color", "color", QColor(255, 255, 255)},
            {"Anisotropy", "float", 0.0},
            {"Anisotropy Rotation", "float", 0.0}
        };
    } else if (type == "Carbon Fiber") {
        m_parameters = {
            {"Fiber Color", "color", QColor(20, 20, 20)},
            {"Resin Color", "color", QColor(60, 60, 60)},
            {"Weave Scale", "float", 1.0},
            {"Weave Angle", "float", 45.0},
            {"Layer Count", "float", 4.0},
            {"Resin Thickness", "float", 0.1},
            {"Clearcoat", "float", 1.0},
            {"Clearcoat Roughness", "float", 0.1}
        };
    } else if (type == "Fabric") {
        m_parameters = {
            {"Base Color", "color", QColor(100, 100, 100)},
            {"Weave Pattern", "combo", 0},
            {"Weave Scale", "float", 1.0},
            {"Fuzz Color", "color", QColor(150, 150, 150)},
            {"Fuzz Amount", "float", 0.3},
            {"Roughness", "float", 0.8},
            {"Normal Strength", "float", 1.0}
        };
    } else if (type == "Rubber") {
        m_parameters = {
            {"Color", "color", QColor(30, 30, 30)},
            {"Roughness", "float", 0.9},
            {"Metalness", "float", 0.0},
            {"Subsurface Amount", "float", 0.1},
            {"Subsurface Color", "color", QColor(50, 50, 50)},
            {"Normal Scale", "float", 1.0}
        };
    }
    
    for (const auto& param : m_parameters) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_parameterTree, {param.name, param.type, param.value.toString()});
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setData(0, Qt::UserRole, param.name);
    }
    
    m_parameterTree->expandAll();
    this->log(QString("Material type changed to: %1").arg(type));
}

void MaterialEditorModule::onShaderChanged(int index) {
    QString shader = m_shaderCombo->currentText();
    this->log(QString("Shader changed to: %1").arg(shader));
}

void MaterialEditorModule::onTextureSelected(const QString& slot) {
    QString file = selectFile(QString("Select %1 Texture").arg(slot), "Images (*.png *.jpg *.dds *.tga *.bmp *.hdr)");
    if (!file.isEmpty()) {
        this->log(QString("Selected %1 texture: %2").arg(slot, file));
        // Update the button text
        for (QObject* child : children()) {
            if (QPushButton* btn = qobject_cast<QPushButton*>(child)) {
                if (btn->property("slot").toString() == slot) {
                    btn->setText(QFileInfo(file).fileName());
                    btn->setToolTip(file);
                }
            }
        }
    }
}

void MaterialEditorModule::onParameterChanged(const QString& name, const QVariant& value) {
    for (auto& param : m_parameters) {
        if (param.name == name) {
            param.value = value;
            break;
        }
    }
    if (m_livePreview) {
        updateMaterialPreview();
    }
}

void MaterialEditorModule::onSaveMaterial() {
    QString file = selectFile("Save Material", "Material Files (*.mat *.json)", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (!file.isEmpty()) {
        saveMaterial(file);
    }
}

void MaterialEditorModule::onLoadMaterial() {
    QString file = selectFile("Load Material", "Material Files (*.mat *.json)", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (!file.isEmpty()) {
        loadMaterial(file);
    }
}

void MaterialEditorModule::onExportShader() {
    QString file = selectFile("Export Shader", "Shader Files (*.hlsl *.glsl *.fx *.json)", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (!file.isEmpty()) {
        exportShader(file);
    }
}

void MaterialEditorModule::onPreviewToggle() {
    m_livePreview = m_livePreviewCheck->isChecked();
    if (m_livePreview) {
        updateMaterialPreview();
    }
    this->log(QString("Live preview: %1").arg(m_livePreview ? "enabled" : "disabled"));
}

void MaterialEditorModule::saveMaterial(const QString& path) {
    QJsonObject obj;
    obj["type"] = m_materialTypeCombo->currentText();
    obj["shader"] = m_shaderCombo->currentText();
    
    QJsonArray paramsArray;
    for (const auto& param : m_parameters) {
        QJsonObject p;
        p["name"] = param.name;
        p["type"] = param.type;
        if (param.type == "color") {
            QColor c = param.value.value<QColor>();
            p["value"] = QString("#%1%2%3").arg(c.red(), 2, 16, QChar('0')).arg(c.green(), 2, 16, QChar('0')).arg(c.blue(), 2, 16, QChar('0'));
        } else if (param.type == "bool") {
            p["value"] = param.value.toBool();
        } else {
            p["value"] = param.value.toDouble();
        }
        paramsArray.append(p);
    }
    obj["parameters"] = paramsArray;
    
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        m_currentMaterialPath = path;
        logSuccess(QString("Material saved to: %1").arg(path));
    } else {
        logError("Failed to save material");
    }
}

void MaterialEditorModule::loadMaterial(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError("Failed to open material file");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    
    if (obj.contains("type")) {
        QString type = obj["type"].toString();
        int idx = m_materialTypeCombo->findText(type);
        if (idx >= 0) m_materialTypeCombo->setCurrentIndex(idx);
    }
    
    if (obj.contains("shader")) {
        QString shader = obj["shader"].toString();
        int idx = m_shaderCombo->findText(shader);
        if (idx >= 0) m_shaderCombo->setCurrentIndex(idx);
    }
    
    if (obj.contains("parameters")) {
        QJsonArray params = obj["parameters"].toArray();
        for (const auto& val : params) {
            QJsonObject p = val.toObject();
            QString name = p["name"].toString();
            QString type = p["type"].toString();
            QVariant value;
            
            if (type == "color") {
                QString hex = p["value"].toString();
                value = QColor(hex);
            } else if (type == "bool") {
                value = p["value"].toBool();
            } else {
                value = p["value"].toDouble();
            }
            
            for (auto& param : m_parameters) {
                if (param.name == name) {
                    param.value = value;
                    break;
                }
            }
        }
    }
    
    // Refresh tree
    for (int i = 0; i < m_parameterTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_parameterTree->topLevelItem(i);
        QString name = item->data(0, Qt::UserRole).toString();
        for (const auto& param : m_parameters) {
            if (param.name == name) {
                item->setText(2, param.value.toString());
                break;
            }
        }
    }
    
    m_currentMaterialPath = path;
    logSuccess(QString("Material loaded from: %1").arg(path));
    
    if (m_livePreview) {
        updateMaterialPreview();
    }
}

void MaterialEditorModule::exportShader(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        logError("Failed to open shader file for writing");
        return;
    }
    
    QTextStream out(&file);
    QString shader = m_shaderCombo->currentText();
    QString type = m_materialTypeCombo->currentText();
    
    out << "// Generated shader for " << type << " using " << shader << "\n";
    out << "// Generated by ksEditor Material Editor\n\n";
    
    if (shader.contains("ksPerPixel")) {
        out << "// HLSL Shader for Assetto Corsa\n";
        out << "Texture2D g_txDiffuse : register(t0);\n";
        out << "Texture2D g_txNormal : register(t1);\n";
        out << "Texture2D g_txRoughness : register(t2);\n";
        out << "Texture2D g_txMetalness : register(t3);\n";
        out << "Texture2D g_txAO : register(t4);\n";
        out << "Texture2D g_txEmissive : register(t5);\n\n";
        
        out << "SamplerState g_samLinear : register(s0);\n\n";
        
        out << "struct VS_INPUT {\n";
        out << "    float3 Pos : POSITION;\n";
        out << "    float3 Nor : NORMAL;\n";
        out << "    float2 Tex : TEXCOORD0;\n";
        out << "    float3 Tan : TANGENT;\n";
        out << "    float3 Bin : BINORMAL;\n";
        out << "};\n\n";
        
        out << "struct PS_INPUT {\n";
        out << "    float4 Pos : SV_POSITION;\n";
        out << "    float2 Tex : TEXCOORD0;\n";
        out << "    float3 Nor : TEXCOORD1;\n";
        out << "    float3 Tan : TEXCOORD2;\n";
        out << "    float3 Bin : TEXCOORD3;\n";
        out << "    float3 View : TEXCOORD4;\n";
        out << "};\n\n";
        
        out << "cbuffer MaterialCB : register(b0) {\n";
        out << "    float4 AlbedoColor;\n";
        out << "    float Metalness;\n";
        out << "    float Roughness;\n";
        out << "    float AO;\n";
        out << "    float NormalScale;\n";
        out << "    float EmissionStrength;\n";
        out << "    float4 EmissionColor;\n";
        out << "    float Alpha;\n";
        out << "    float AlphaCutoff;\n";
        out << "};\n\n";
        
        out << "PS_INPUT VSMain(VS_INPUT input) {\n";
        out << "    PS_INPUT output;\n";
        out << "    output.Pos = mul(float4(input.Pos, 1.0), g_mWorldViewProj);\n";
        out << "    output.Tex = input.Tex;\n";
        out << "    output.Nor = mul(input.Nor, (float3x3)g_mWorld);\n";
        out << "    output.Tan = mul(input.Tan, (float3x3)g_mWorld);\n";
        out << "    output.Bin = mul(input.Bin, (float3x3)g_mWorld);\n";
        out << "    output.View = normalize(g_vCameraPos - mul(input.Pos, g_mWorld));\n";
        out << "    return output;\n";
        out << "}\n\n";
        
        out << "float4 PSMain(PS_INPUT input) : SV_TARGET {\n";
        out << "    float3 N = normalize(input.Nor);\n";
        out << "    float3 V = normalize(input.View);\n";
        out << "    \n";
        out << "    float4 albedo = g_txDiffuse.Sample(g_samLinear, input.Tex) * AlbedoColor;\n";
        out << "    float3 normalMap = g_txNormal.Sample(g_samLinear, input.Tex).rgb * 2.0 - 1.0;\n";
        out << "    normalMap = normalize(normalMap * NormalScale + float3(0, 0, 1));\n";
        out << "    float roughness = g_txRoughness.Sample(g_samLinear, input.Tex).r * Roughness;\n";
        out << "    float metalness = g_txMetalness.Sample(g_samLinear, input.Tex).r * Metalness;\n";
        out << "    float ao = g_txAO.Sample(g_samLinear, input.Tex).r * AO;\n";
        out << "    float3 emissive = g_txEmissive.Sample(g_samLinear, input.Tex).rgb * EmissionColor.rgb * EmissionStrength;\n";
        out << "    \n";
        out << "    // PBR lighting calculation here\n";
        out << "    float3 color = albedo.rgb * ao + emissive;\n";
        out << "    \n";
        out << "    return float4(color, albedo.a * Alpha);\n";
        out << "}\n";
    }
    
    file.close();
    logSuccess(QString("Shader exported to: %1").arg(path));
}

void MaterialEditorModule::updateMaterialPreview() {
    this->log("Updating material preview...");
    // In a real implementation, this would update the viewport/preview
}

void MaterialEditorModule::refreshPresetList() {
    m_presetList->clear();
    
    // Built-in presets
    QStringList presets = {
        "Car Paint - Red", "Car Paint - Blue", "Car Paint - Metallic Silver",
        "Car Paint - Pearl White", "Car Paint - Matte Black",
        "Chrome - Bright", "Chrome - Brushed", "Chrome - Gold",
        "Glass - Clear", "Glass - Tinted", "Glass - Frosted",
        "Carbon Fiber - Standard", "Carbon Fiber - Forged", "Carbon Fiber - Colored",
        "Fabric - Denim", "Fabric - Leather", "Fabric - Alcantara", "Fabric - Suede",
        "Rubber - Tire", "Rubber - Soft", "Rubber - Hard",
        "Metal - Steel", "Metal - Aluminum", "Metal - Copper", "Metal - Brass",
        "Plastic - Glossy", "Plastic - Matte", "Plastic - Textured"
    };
    
    m_presetList->addItems(presets);
    
    // Load custom presets from settings
    QSettings settings;
    settings.beginGroup("MaterialPresets");
    for (const QString& key : settings.childKeys()) {
        if (!presets.contains(key)) {
            m_presetList->addItem(key + " (custom)");
        }
    }
    settings.endGroup();
}

void MaterialEditorModule::applyPreset(const QString& name) {
    QString cleanName = name;
    if (cleanName.endsWith(" (custom)")) {
        cleanName = cleanName.left(cleanName.length() - 9);
    }
    
    this->log(QString("Applying preset: %1").arg(cleanName));
    
    // Built-in preset logic
    if (cleanName.startsWith("Car Paint")) {
        m_materialTypeCombo->setCurrentText("Car Paint");
        if (cleanName.contains("Red")) {
            setParameter("Base Color", QColor(200, 30, 30));
        } else if (cleanName.contains("Blue")) {
            setParameter("Base Color", QColor(30, 80, 200));
        } else if (cleanName.contains("Silver")) {
            setParameter("Base Color", QColor(180, 180, 190));
            setParameter("Flake Density", 0.7);
        } else if (cleanName.contains("White")) {
            setParameter("Base Color", QColor(240, 240, 245));
        } else if (cleanName.contains("Black")) {
            setParameter("Base Color", QColor(30, 30, 30));
            setParameter("Clearcoat Roughness", 0.3);
        }
    } else if (cleanName.startsWith("Chrome")) {
        m_materialTypeCombo->setCurrentText("Chrome");
        if (cleanName.contains("Gold")) {
            setParameter("Tint Color", QColor(255, 200, 100));
        } else if (cleanName.contains("Brushed")) {
            setParameter("Roughness", 0.15);
            setParameter("Anisotropy", 0.8);
        }
    } else if (cleanName.startsWith("Glass")) {
        m_materialTypeCombo->setCurrentText("Glass");
        if (cleanName.contains("Tinted")) {
            setParameter("Tint Color", QColor(100, 150, 255));
        } else if (cleanName.contains("Frosted")) {
            setParameter("Roughness", 0.4);
            setParameter("Alpha", 0.7);
        }
    } else if (cleanName.startsWith("Carbon Fiber")) {
        m_materialTypeCombo->setCurrentText("Carbon Fiber");
        if (cleanName.contains("Forged")) {
            setParameter("Weave Pattern", 1);
            setParameter("Weave Scale", 0.5);
        } else if (cleanName.contains("Colored")) {
            setParameter("Fiber Color", QColor(60, 20, 20));
            setParameter("Resin Color", QColor(100, 40, 40));
        }
    } else if (cleanName.startsWith("Fabric")) {
        m_materialTypeCombo->setCurrentText("Fabric");
        if (cleanName.contains("Leather")) {
            setParameter("Weave Pattern", 2);
            setParameter("Roughness", 0.6);
        } else if (cleanName.contains("Alcantara") || cleanName.contains("Suede")) {
            setParameter("Weave Pattern", 3);
            setParameter("Fuzz Amount", 0.6);
            setParameter("Roughness", 0.9);
        }
    } else if (cleanName.startsWith("Rubber")) {
        m_materialTypeCombo->setCurrentText("Rubber");
        if (cleanName.contains("Tire")) {
            setParameter("Color", QColor(20, 20, 20));
            setParameter("Roughness", 0.95);
        }
    }
    
    // Refresh tree
    onMaterialTypeChanged(m_materialTypeCombo->currentIndex());
    
    if (m_livePreview) {
        updateMaterialPreview();
    }
    
    logSuccess(QString("Preset applied: %1").arg(cleanName));
}

void MaterialEditorModule::saveCurrentAsPreset(const QString& name) {
    QSettings settings;
    settings.beginGroup("MaterialPresets");
    
    QJsonObject obj;
    obj["type"] = m_materialTypeCombo->currentText();
    obj["shader"] = m_shaderCombo->currentText();
    
    QJsonArray paramsArray;
    for (const auto& param : m_parameters) {
        QJsonObject p;
        p["name"] = param.name;
        p["type"] = param.type;
        if (param.type == "color") {
            QColor c = param.value.value<QColor>();
            p["value"] = QString("#%1%2%3").arg(c.red(), 2, 16, QChar('0')).arg(c.green(), 2, 16, QChar('0')).arg(c.blue(), 2, 16, QChar('0'));
        } else if (param.type == "bool") {
            p["value"] = param.value.toBool();
        } else {
            p["value"] = param.value.toDouble();
        }
        paramsArray.append(p);
    }
    obj["parameters"] = paramsArray;
    
    settings.setValue(name, QJsonDocument(obj).toJson(QJsonDocument::Compact));
    settings.endGroup();
    
    refreshPresetList();
    logSuccess(QString("Saved as preset: %1").arg(name));
}

void MaterialEditorModule::deletePreset(const QString& name) {
    QString cleanName = name;
    if (cleanName.endsWith(" (custom)")) {
        cleanName = cleanName.left(cleanName.length() - 9);
    }
    
    QSettings settings;
    settings.beginGroup("MaterialPresets");
    if (settings.contains(cleanName)) {
        settings.remove(cleanName);
        refreshPresetList();
        logSuccess(QString("Deleted preset: %1").arg(cleanName));
    }
    settings.endGroup();
}

void MaterialEditorModule::setParameter(const QString& name, const QVariant& value) {
    for (auto& param : m_parameters) {
        if (param.name == name) {
            param.value = value;
            break;
        }
    }
}

void MaterialEditorModule::loadPresets() {
    // Already handled in refreshPresetList
}

void MaterialEditorModule::savePresets() {
    // Already handled via QSettings
}

void MaterialEditorModule::onActivation() {
    ModuleGuiBase::onActivation();
}

void MaterialEditorModule::onDeactivation() {
    ModuleGuiBase::onDeactivation();
}

void MaterialEditorModule::onNewMaterial() {
}

void MaterialEditorModule::startMaterialDrag(const QString& materialId, const QVector3D& worldPos) {
    // Material drag-and-drop: store the dragged material id for the viewport
    // drop handler. A minimal implementation keeps the active drag material
    // alive until the drop is finished.
    QMimeData* data = mimeData(materialId);
    if (data && m_livePreviewCheck) {
        // Store for viewport preview/drop target handling.
        m_dragMaterialId = materialId;
    }
}

void MaterialEditorModule::finishMaterialDrag() {
    m_dragMaterialId.clear();
}

QStringList MaterialEditorModule::mimeTypes() const {
    return QStringList() << "application/x-ksmodeler-material";
}

QMimeData* MaterialEditorModule::mimeData(const QString& materialId) const {
    QMimeData* data = new QMimeData();
    data->setData("application/x-ksmodeler-material", materialId.toUtf8());
    return data;
}

void MaterialEditorModule::showMaterialContextMenu(const QVector3D& worldPos) {
    // Context menu for materials in the viewport (assign / edit / export).
}

} // namespace material
} // namespace ks

