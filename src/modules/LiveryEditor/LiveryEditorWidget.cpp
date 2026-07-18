#include "LiveryEditorWidget.h"
#include "LiveryPainter.h"
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QInputDialog>

namespace ks {

LiveryEditorWidget::LiveryEditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(LiveryEditor::instance())
    , m_painterWidget(nullptr)
    , m_updatingUI(false)
{
    setupUI();

    connect(m_editor, &LiveryEditor::skinListChanged, this, &LiveryEditorWidget::refreshSkinList);
    connect(m_editor, &LiveryEditor::skinLoaded, this, [this](const QString& skinName) {
        refreshLayerList();
        if (m_viewport3D && !m_editor->carPath().isEmpty()) {
            m_viewport3D->setCarPath(m_editor->carPath());
            m_viewport3D->focusOnModel();
        }
    });
    connect(m_editor, &LiveryEditor::liveryModified, this, &LiveryEditorWidget::liveryModified);
    connect(m_editor, &LiveryEditor::textureLoaded, this, [this](const QImage& tex) {
        if (m_painterWidget) {
            m_painterWidget->setTexture(tex);
        }
        if (m_viewport3D) {
            m_viewport3D->applyLiveryTexture(tex);
        }
    });
}

void LiveryEditorWidget::setCarPath(const QString& path)
{
    m_editor->setCarPath(path);
    refreshSkinList();
}

void LiveryEditorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    m_viewport3D = new LiveryViewport(this);
    m_viewport3D->setMinimumHeight(250);
    splitter->addWidget(m_viewport3D);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; }");

    QWidget* scrollContent = new QWidget();
    QFormLayout* formLayout = new QFormLayout(scrollContent);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(4);

    setupSkinPanel(scrollContent, formLayout);
    setupLayerPanel(scrollContent, formLayout);
    setupPaintPanel(scrollContent, formLayout);
    setupLicensePlatePanel(scrollContent, formLayout);

    QWidget* actionsWidget = new QWidget(scrollContent);
    QHBoxLayout* actionsLayout = new QHBoxLayout(actionsWidget);
    actionsLayout->setContentsMargins(0, 0, 0, 0);

    m_saveSkinBtn = new QPushButton(tr("Save Skin"), actionsWidget);
    m_saveSkinBtn->setStyleSheet("QPushButton { background: #3a6a3a; color: #fff; border: 1px solid #4a7a4a; padding: 6px; }");
    actionsLayout->addWidget(m_saveSkinBtn);

    m_refreshBtn = new QPushButton(tr("Refresh"), actionsWidget);
    m_refreshBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; padding: 6px; }");
    actionsLayout->addWidget(m_refreshBtn);

    m_exportDdsBtn = new QPushButton(tr("Export DDS"), actionsWidget);
    m_exportDdsBtn->setStyleSheet("QPushButton { background: #5a5a8a; color: #fff; border: 1px solid #6a6a9a; padding: 6px; }");
    m_exportDdsBtn->setToolTip(tr("Export livery texture as DDS (AC format)"));
    actionsLayout->addWidget(m_exportDdsBtn);

    m_importDecalBtn = new QPushButton(tr("Import Decal"), actionsWidget);
    m_importDecalBtn->setStyleSheet("QPushButton { background: #5a5a8a; color: #fff; border: 1px solid #6a6a9a; padding: 6px; }");
    actionsLayout->addWidget(m_importDecalBtn);

    m_templateBtn = new QPushButton(tr("Template..."), actionsWidget);
    m_templateBtn->setStyleSheet("QPushButton { background: #8a7a4a; color: #fff; border: 1px solid #9a8a5a; padding: 6px; }");
    m_templateBtn->setToolTip(tr("Create new skin from template"));
    actionsLayout->addWidget(m_templateBtn);

    m_undoBtn = new QPushButton(tr("Undo"), actionsWidget);
    m_undoBtn->setStyleSheet("QPushButton { background: #6a5a3a; color: #fff; border: 1px solid #7a6a4a; padding: 6px; }");
    m_undoBtn->setEnabled(false);
    actionsLayout->addWidget(m_undoBtn);

    m_redoBtn = new QPushButton(tr("Redo"), actionsWidget);
    m_redoBtn->setStyleSheet("QPushButton { background: #6a5a3a; color: #fff; border: 1px solid #7a6a4a; padding: 6px; }");
    m_redoBtn->setEnabled(false);
    actionsLayout->addWidget(m_redoBtn);

    formLayout->addRow("", actionsWidget);

    // Color palette row
    QWidget* paletteWidget = new QWidget(scrollContent);
    QHBoxLayout* paletteLayout = new QHBoxLayout(paletteWidget);
    paletteLayout->setContentsMargins(0, 0, 0, 0);
    paletteLayout->setSpacing(2);

    auto defaultPalette = LiverySystem::getDefaultPalette();
    m_paletteCount = qMin(defaultPalette.size(), 10);
    for (int i = 0; i < m_paletteCount; ++i) {
        QPushButton* btn = new QPushButton(paletteWidget);
        btn->setFixedSize(22, 22);
        btn->setStyleSheet(QString("QPushButton { background: %1; border: 1px solid #666; border-radius: 3px; }")
                           .arg(defaultPalette[i].color.name()));
        btn->setToolTip(defaultPalette[i].name);
        m_paletteBtns[i] = btn;
        paletteLayout->addWidget(btn);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            auto palette = LiverySystem::getDefaultPalette();
            if (i < palette.size()) onPaletteColorSelected(palette[i].color);
        });
    }
    paletteLayout->addStretch();
    formLayout->addRow(tr("Colors:"), paletteWidget);

    scrollArea->setWidget(scrollContent);
    splitter->addWidget(scrollArea);
    mainLayout->addWidget(splitter);

    connect(m_createSkinBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onCreateSkin);
    connect(m_deleteSkinBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onDeleteSkin);
    connect(m_duplicateSkinBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onDuplicateSkin);
    connect(m_skinList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        onSkinSelected(current);
    });
    connect(m_addLayerBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onAddLayer);
    connect(m_removeLayerBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onRemoveLayer);
    connect(m_moveUpBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onMoveLayerUp);
    connect(m_moveDownBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onMoveLayerDown);
    connect(m_layerList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        onLayerSelected(current);
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, &LiveryEditorWidget::onLayerOpacityChanged);
    connect(m_visibleCheck, &QCheckBox::stateChanged, this, &LiveryEditorWidget::onLayerVisibilityChanged);
    connect(m_colorBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onColorSelected);
    connect(m_brushSizeSlider, &QSlider::valueChanged, this, &LiveryEditorWidget::onBrushSizeChanged);
    connect(m_brushHardnessSlider, &QSlider::valueChanged, this, &LiveryEditorWidget::onBrushHardnessChanged);
    connect(m_generatePlateBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onLicensePlateGenerate);
    connect(m_saveSkinBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onSaveSkin);
    connect(m_refreshBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onRefreshSkins);
    connect(m_exportDdsBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onExportDDS);
    connect(m_importDecalBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onImportDecal);
    connect(m_templateBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onCreateFromTemplate);
    connect(m_undoBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onUndo);
    connect(m_redoBtn, &QPushButton::clicked, this, &LiveryEditorWidget::onRedo);

    // Update undo/redo button states
    connect(m_editor, &LiveryEditor::liveryModified, this, [this]() {
        m_undoBtn->setEnabled(LiverySystem::canUndo());
        m_redoBtn->setEnabled(LiverySystem::canRedo());
    });

    refreshSkinList();
    clearLayerUI();
}

void LiveryEditorWidget::setupSkinPanel(QWidget* parent, QFormLayout* layout)
{
    m_skinsGroup = new QGroupBox(tr("Skins"), parent);
    m_skinsGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QVBoxLayout* skinLayout = new QVBoxLayout(m_skinsGroup);
    skinLayout->setContentsMargins(6, 10, 6, 6);
    skinLayout->setSpacing(4);

    m_skinList = new QListWidget(m_skinsGroup);
    m_skinList->setMinimumHeight(100);
    m_skinList->setStyleSheet(
        "QListWidget { background: #2d2d2d; color: #cccccc; border: 1px solid #444; }"
        "QListWidget::item:selected { background: #3a6ea5; }"
    );
    skinLayout->addWidget(m_skinList);

    QWidget* btnRow = new QWidget(m_skinsGroup);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(4);

    m_createSkinBtn = new QPushButton("+", btnRow);
    m_createSkinBtn->setFixedSize(28, 28);
    m_createSkinBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; font-weight: bold; }");
    btnLayout->addWidget(m_createSkinBtn);

    m_deleteSkinBtn = new QPushButton("-", btnRow);
    m_deleteSkinBtn->setFixedSize(28, 28);
    m_deleteSkinBtn->setStyleSheet("QPushButton { background: #6a3a3a; color: #fff; border: 1px solid #775; font-weight: bold; }");
    btnLayout->addWidget(m_deleteSkinBtn);

    m_duplicateSkinBtn = new QPushButton("D", btnRow);
    m_duplicateSkinBtn->setFixedSize(28, 28);
    m_duplicateSkinBtn->setStyleSheet("QPushButton { background: #4a4a6a; color: #fff; border: 1px solid #557; font-weight: bold; }");
    btnLayout->addWidget(m_duplicateSkinBtn);

    btnLayout->addStretch();
    skinLayout->addWidget(btnRow);

    layout->addRow("", m_skinsGroup);
}

void LiveryEditorWidget::setupLayerPanel(QWidget* parent, QFormLayout* layout)
{
    m_layersGroup = new QGroupBox(tr("Layers"), parent);
    m_layersGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QVBoxLayout* layerLayout = new QVBoxLayout(m_layersGroup);
    layerLayout->setContentsMargins(6, 10, 6, 6);
    layerLayout->setSpacing(4);

    QWidget* listRow = new QWidget(m_layersGroup);
    QHBoxLayout* listLayout = new QHBoxLayout(listRow);
    listLayout->setContentsMargins(0, 0, 0, 0);

    m_layerList = new QListWidget(listRow);
    m_layerList->setMinimumHeight(100);
    m_layerList->setStyleSheet(
        "QListWidget { background: #2d2d2d; color: #cccccc; border: 1px solid #444; }"
        "QListWidget::item:selected { background: #3a6ea5; }"
    );
    listLayout->addWidget(m_layerList);

    QWidget* btnCol = new QWidget(listRow);
    QVBoxLayout* btnColLayout = new QVBoxLayout(btnCol);
    btnColLayout->setContentsMargins(2, 0, 0, 0);
    btnColLayout->setSpacing(4);

    m_addLayerBtn = new QPushButton("+", btnCol);
    m_addLayerBtn->setFixedSize(28, 28);
    m_addLayerBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; font-weight: bold; }");
    btnColLayout->addWidget(m_addLayerBtn);

    m_removeLayerBtn = new QPushButton("-", btnCol);
    m_removeLayerBtn->setFixedSize(28, 28);
    m_removeLayerBtn->setStyleSheet("QPushButton { background: #6a3a3a; color: #fff; border: 1px solid #775; font-weight: bold; }");
    btnColLayout->addWidget(m_removeLayerBtn);

    m_moveUpBtn = new QPushButton("^", btnCol);
    m_moveUpBtn->setFixedSize(28, 28);
    m_moveUpBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; font-weight: bold; }");
    btnColLayout->addWidget(m_moveUpBtn);

    m_moveDownBtn = new QPushButton("v", btnCol);
    m_moveDownBtn->setFixedSize(28, 28);
    m_moveDownBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; font-weight: bold; }");
    btnColLayout->addWidget(m_moveDownBtn);

    btnColLayout->addStretch();
    listLayout->addWidget(btnCol);
    layerLayout->addWidget(listRow);

    m_layerPropsGroup = new QGroupBox(tr("Layer Properties"), m_layersGroup);
    m_layerPropsGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* propsLayout = new QFormLayout(m_layerPropsGroup);
    propsLayout->setContentsMargins(6, 10, 6, 6);
    propsLayout->setSpacing(4);

    m_layerNameEdit = new QLineEdit(m_layerPropsGroup);
    m_layerNameEdit->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    propsLayout->addRow(tr("Name:"), m_layerNameEdit);

    m_layerTypeCombo = new QComboBox(m_layerPropsGroup);
    m_layerTypeCombo->addItems({"decal", "paint", "texture"});
    m_layerTypeCombo->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
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
    layout->addRow("", m_layersGroup);
}

void LiveryEditorWidget::setupPaintPanel(QWidget* parent, QFormLayout* layout)
{
    m_paintGroup = new QGroupBox(tr("Paint Tools"), parent);
    m_paintGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* paintLayout = new QFormLayout(m_paintGroup);
    paintLayout->setContentsMargins(6, 10, 6, 6);
    paintLayout->setSpacing(4);

    m_colorBtn = new QPushButton(tr("Color"), m_paintGroup);
    m_colorBtn->setStyleSheet("QPushButton { background: #cc0000; color: #fff; border: 1px solid #555; padding: 4px; }");
    paintLayout->addRow(tr("Brush Color:"), m_colorBtn);

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

    QWidget* hardnessWidget = new QWidget(m_paintGroup);
    QHBoxLayout* hardnessLayout = new QHBoxLayout(hardnessWidget);
    hardnessLayout->setContentsMargins(0, 0, 0, 0);
    m_brushHardnessSlider = new QSlider(Qt::Horizontal, hardnessWidget);
    m_brushHardnessSlider->setRange(0, 100);
    m_brushHardnessSlider->setValue(50);
    m_brushHardnessLabel = new QLabel("50%", hardnessWidget);
    m_brushHardnessLabel->setFixedWidth(35);
    hardnessLayout->addWidget(m_brushHardnessSlider);
    hardnessLayout->addWidget(m_brushHardnessLabel);
    paintLayout->addRow(tr("Hardness:"), hardnessWidget);

    m_painterWidget = new LiveryPainterWidget(this);
    m_painterWidget->setMinimumHeight(200);
    m_painterWidget->setStyleSheet("background: #222; border: 1px solid #444;");
    paintLayout->addRow(tr("Canvas:"), m_painterWidget);

    connect(m_painterWidget, &LiveryPainterWidget::textureChanged, this, [this](const QImage& tex) {
        if (m_viewport3D)
            m_viewport3D->applyLiveryTexture(tex);
    });

    layout->addRow("", m_paintGroup);
}

void LiveryEditorWidget::setupLicensePlatePanel(QWidget* parent, QFormLayout* layout)
{
    m_plateGroup = new QGroupBox(tr("License Plate"), parent);
    m_plateGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* plateLayout = new QFormLayout(m_plateGroup);
    plateLayout->setContentsMargins(6, 10, 6, 6);
    plateLayout->setSpacing(4);

    m_plateText = new QLineEdit(m_plateGroup);
    m_plateText->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    m_plateText->setPlaceholderText(tr("Enter plate text"));
    plateLayout->addRow(tr("Text:"), m_plateText);

    m_plateCountry = new QComboBox(m_plateGroup);
    m_plateCountry->addItems(LiverySystem::getSupportedCountries());
    m_plateCountry->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    plateLayout->addRow(tr("Country:"), m_plateCountry);

    m_generatePlateBtn = new QPushButton(tr("Generate"), m_plateGroup);
    m_generatePlateBtn->setStyleSheet("QPushButton { background: #4a4a6a; color: #fff; border: 1px solid #557; padding: 4px; }");
    plateLayout->addRow("", m_generatePlateBtn);

    layout->addRow("", m_plateGroup);
}

void LiveryEditorWidget::refreshSkinList()
{
    m_skinList->clear();
    const auto names = m_editor->getSkinNames();
    for (const auto& name : names) {
        m_skinList->addItem(name);
    }
}

void LiveryEditorWidget::refreshLayerList()
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

void LiveryEditorWidget::onSkinSelected(QListWidgetItem* current)
{
    if (!current) return;
    m_editor->setCurrentSkin(current->text());
    refreshLayerList();
    emit skinSelected(current->text());
}

void LiveryEditorWidget::onCreateSkin()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Create Skin"), tr("Skin name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        m_editor->createSkin(name);
    }
}

void LiveryEditorWidget::onDeleteSkin()
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

void LiveryEditorWidget::onDuplicateSkin()
{
    QListWidgetItem* item = m_skinList->currentItem();
    if (!item) return;

    bool ok;
    QString name = QInputDialog::getText(this, tr("Duplicate Skin"), tr("New name:"), QLineEdit::Normal, item->text() + "_copy", &ok);
    if (ok && !name.isEmpty()) {
        m_editor->duplicateSkin(item->text(), name);
    }
}

void LiveryEditorWidget::onLayerSelected(QListWidgetItem* current)
{
    if (!current) {
        clearLayerUI();
        return;
    }
    updateLayerUI();
}

void LiveryEditorWidget::onAddLayer()
{
    LiverySystem::LiveryLayer layer;
    layer.name = QString("layer_%1").arg(m_editor->currentConfig().layers.size() + 1);
    layer.type = "decal";
    layer.opacity = 1.0f;
    layer.position[0] = 0.0f;
    layer.position[1] = 0.0f;
    layer.size[0] = 1.0f;
    layer.size[1] = 1.0f;
    layer.visible = true;

    // Record undo
    LiverySystem::UndoAction action;
    action.type = LiverySystem::UndoAction::LayerAdd;
    action.layerIndex = m_editor->currentConfig().layers.size();
    action.newLayer = layer;
    action.description = tr("Add layer: %1").arg(layer.name);
    LiverySystem::pushUndo(action);

    m_editor->addLayer(layer);
    refreshLayerList();
    m_undoBtn->setEnabled(true);
    m_redoBtn->setEnabled(false);
}

void LiveryEditorWidget::onRemoveLayer()
{
    int row = m_layerList->currentRow();
    if (row < 0) return;

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        // Record undo
        LiverySystem::UndoAction action;
        action.type = LiverySystem::UndoAction::LayerRemove;
        action.layerIndex = row;
        action.oldLayer = config.layers[row];
        action.description = tr("Remove layer: %1").arg(config.layers[row].name);
        LiverySystem::pushUndo(action);
    }

    m_editor->removeLayer(row);
    refreshLayerList();
    clearLayerUI();
    m_undoBtn->setEnabled(true);
    m_redoBtn->setEnabled(false);
}

void LiveryEditorWidget::onMoveLayerUp()
{
    int row = m_layerList->currentRow();
    if (row <= 0) return;
    m_editor->moveLayer(row, row - 1);
    refreshLayerList();
    m_layerList->setCurrentRow(row - 1);
}

void LiveryEditorWidget::onMoveLayerDown()
{
    int row = m_layerList->currentRow();
    if (row < 0 || row >= m_editor->currentConfig().layers.size() - 1) return;
    m_editor->moveLayer(row, row + 1);
    refreshLayerList();
    m_layerList->setCurrentRow(row + 1);
}

void LiveryEditorWidget::onLayerOpacityChanged(int value)
{
    if (m_updatingUI) return;
    int row = m_layerList->currentRow();
    if (row < 0) return;

    m_opacityLabel->setText(QString("%1%").arg(value));

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        LiverySystem::LiveryLayer layer = config.layers[row];
        layer.opacity = value / 100.0f;
        m_editor->updateLayer(row, layer);
    }
}

void LiveryEditorWidget::onLayerVisibilityChanged(int state)
{
    if (m_updatingUI) return;
    int row = m_layerList->currentRow();
    if (row < 0) return;

    auto& config = m_editor->currentConfig();
    if (row < config.layers.size()) {
        LiverySystem::LiveryLayer layer = config.layers[row];
        layer.visible = (state != Qt::Unchecked);
        m_editor->updateLayer(row, layer);
        refreshLayerList();
    }
}

void LiveryEditorWidget::onBrushSizeChanged(int size)
{
    m_brushSizeLabel->setText(QString::number(size));
}

void LiveryEditorWidget::onBrushHardnessChanged(int value)
{
    m_brushHardnessLabel->setText(QString("%1%").arg(value));
}

void LiveryEditorWidget::onColorSelected()
{
    QColor color = QColorDialog::getColor(Qt::red, this, tr("Brush Color"));
    if (color.isValid()) {
        m_colorBtn->setStyleSheet(QString("QPushButton { background: %1; color: #fff; border: 1px solid #555; padding: 4px; }").arg(color.name()));
    }
}

void LiveryEditorWidget::onLicensePlateGenerate()
{
    QString text = m_plateText->text().trimmed();
    QString country = m_plateCountry->currentText();

    if (text.isEmpty()) {
        QMessageBox::warning(this, tr("License Plate"), tr("Please enter plate text."));
        return;
    }

    if (!LiverySystem::isValidPlateText(text, country)) {
        QMessageBox::warning(this, tr("License Plate"), tr("Invalid plate text for selected country."));
        return;
    }

    m_editor->generateLicensePlate(text, country);
    refreshLayerList();
}

void LiveryEditorWidget::onSaveSkin()
{
    m_editor->saveCurrentSkin();
}

void LiveryEditorWidget::onRefreshSkins()
{
    m_editor->loadSkins();
}

void LiveryEditorWidget::updateLayerUI()
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

void LiveryEditorWidget::clearLayerUI()
{
    m_updatingUI = true;
    m_layerNameEdit->clear();
    m_layerTypeCombo->setCurrentIndex(0);
    m_opacitySlider->setValue(100);
    m_opacityLabel->setText("100%");
    m_visibleCheck->setChecked(true);
    m_updatingUI = false;
}

void LiveryEditorWidget::onExportDDS()
{
    QString skinName = m_editor->currentSkin();
    if (skinName.isEmpty()) {
        QMessageBox::warning(this, tr("Export DDS"), tr("No skin selected."));
        return;
    }

    QString carPath = m_editor->carPath();
    QString skinPath = carPath + "/skins/" + skinName;

    QString outputPath = QFileDialog::getSaveFileName(this, tr("Export Livery as DDS"),
        skinPath + "/livery.dds",
        tr("DDS files (*.dds)"));
    if (outputPath.isEmpty()) return;

    if (LiverySystem::exportSkinAsDDS(skinPath, outputPath)) {
        QMessageBox::information(this, tr("Export DDS"), tr("Livery exported as DDS:\n%1").arg(outputPath));
    } else {
        QMessageBox::warning(this, tr("Export DDS"), tr("Failed to export DDS. Check that a livery texture exists."));
    }
}

void LiveryEditorWidget::onImportDecal()
{
    QString decalPath = QFileDialog::getOpenFileName(this, tr("Import Decal"),
        QString(),
        LiverySystem::getSupportedDecalFormats().join(";;"));
    if (decalPath.isEmpty()) return;

    QString skinName = m_editor->currentSkin();
    if (skinName.isEmpty()) {
        QMessageBox::warning(this, tr("Import Decal"), tr("No skin selected."));
        return;
    }

    QString skinPath = m_editor->carPath() + "/skins/" + skinName;
    if (!LiverySystem::importDecal(decalPath, skinPath)) {
        QMessageBox::warning(this, tr("Import Decal"), tr("Failed to import decal."));
        return;
    }

    QFileInfo fi(decalPath);
    LiverySystem::LiveryLayer layer;
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

void LiveryEditorWidget::onCreateFromTemplate()
{
    auto templates = LiverySystem::getBuiltinTemplates();
    QStringList names;
    for (const auto& t : templates) names << t.name;

    bool ok;
    QString selected = QInputDialog::getItem(this, tr("Create from Template"),
        tr("Choose livery template:"), names, 0, false, &ok);
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

    LiverySystem::createSkinFromTemplate(carPath, skinName, templates[idx]);
    m_editor->loadSkins();
    refreshSkinList();
    QMessageBox::information(this, tr("Template"), tr("Skin created from template: %1").arg(skinName));
}

void LiveryEditorWidget::onUndo()
{
    if (!LiverySystem::canUndo()) return;

    auto action = LiverySystem::undoLast();
    auto& config = m_editor->currentConfig();

    switch (action.type) {
    case LiverySystem::UndoAction::LayerAdd:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers.removeAt(action.layerIndex);
        }
        break;
    case LiverySystem::UndoAction::LayerRemove:
        if (action.layerIndex >= 0) {
            config.layers.insert(action.layerIndex, action.oldLayer);
        }
        break;
    case LiverySystem::UndoAction::LayerModify:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers[action.layerIndex] = action.oldLayer;
        }
        break;
    case LiverySystem::UndoAction::LayerMove:
        break;
    default:
        break;
    }

    m_editor->saveCurrentSkin();
    refreshLayerList();
    m_undoBtn->setEnabled(LiverySystem::canUndo());
    m_redoBtn->setEnabled(LiverySystem::canRedo());
}

void LiveryEditorWidget::onRedo()
{
    if (!LiverySystem::canRedo()) return;

    auto action = LiverySystem::redoLast();
    auto& config = m_editor->currentConfig();

    switch (action.type) {
    case LiverySystem::UndoAction::LayerAdd:
        if (action.layerIndex >= 0) {
            config.layers.insert(action.layerIndex, action.newLayer);
        }
        break;
    case LiverySystem::UndoAction::LayerRemove:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers.removeAt(action.layerIndex);
        }
        break;
    case LiverySystem::UndoAction::LayerModify:
        if (action.layerIndex >= 0 && action.layerIndex < config.layers.size()) {
            config.layers[action.layerIndex] = action.newLayer;
        }
        break;
    default:
        break;
    }

    m_editor->saveCurrentSkin();
    refreshLayerList();
    m_undoBtn->setEnabled(LiverySystem::canUndo());
    m_redoBtn->setEnabled(LiverySystem::canRedo());
}

void LiveryEditorWidget::onPaletteColorSelected(const QColor& color)
{
    Q_UNUSED(color);
    // Apply selected palette color to the brush
    m_colorBtn->setStyleSheet(QString("QPushButton { background: %1; color: #fff; border: 1px solid #555; padding: 4px; }")
                              .arg(color.name()));
}

} // namespace ks
