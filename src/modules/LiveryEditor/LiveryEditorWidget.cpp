#include "LiveryEditorWidget.h"
#include "LiveryPainter.h"
#include <QHBoxLayout>
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
    connect(m_editor, &LiveryEditor::skinLoaded, this, [this](const QString&) {
        refreshLayerList();
    });
    connect(m_editor, &LiveryEditor::liveryModified, this, &LiveryEditorWidget::liveryModified);
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

    m_saveSkinBtn = new QPushButton("Save Skin", actionsWidget);
    m_saveSkinBtn->setStyleSheet("QPushButton { background: #3a6a3a; color: #fff; border: 1px solid #4a7a4a; padding: 6px; }");
    actionsLayout->addWidget(m_saveSkinBtn);

    m_refreshBtn = new QPushButton("Refresh", actionsWidget);
    m_refreshBtn->setStyleSheet("QPushButton { background: #4a4a4a; color: #fff; border: 1px solid #555; padding: 6px; }");
    actionsLayout->addWidget(m_refreshBtn);

    formLayout->addRow("", actionsWidget);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

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

    refreshSkinList();
    clearLayerUI();
}

void LiveryEditorWidget::setupSkinPanel(QWidget* parent, QFormLayout* layout)
{
    m_skinsGroup = new QGroupBox("Skins", parent);
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
    m_layersGroup = new QGroupBox("Layers", parent);
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

    m_layerPropsGroup = new QGroupBox("Layer Properties", m_layersGroup);
    m_layerPropsGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* propsLayout = new QFormLayout(m_layerPropsGroup);
    propsLayout->setContentsMargins(6, 10, 6, 6);
    propsLayout->setSpacing(4);

    m_layerNameEdit = new QLineEdit(m_layerPropsGroup);
    m_layerNameEdit->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    propsLayout->addRow("Name:", m_layerNameEdit);

    m_layerTypeCombo = new QComboBox(m_layerPropsGroup);
    m_layerTypeCombo->addItems({"decal", "paint", "texture"});
    m_layerTypeCombo->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    propsLayout->addRow("Type:", m_layerTypeCombo);

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
    propsLayout->addRow("Opacity:", opacityWidget);

    m_visibleCheck = new QCheckBox("Visible", m_layerPropsGroup);
    m_visibleCheck->setChecked(true);
    propsLayout->addRow("", m_visibleCheck);

    layerLayout->addWidget(m_layerPropsGroup);
    layout->addRow("", m_layersGroup);
}

void LiveryEditorWidget::setupPaintPanel(QWidget* parent, QFormLayout* layout)
{
    m_paintGroup = new QGroupBox("Paint Tools", parent);
    m_paintGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* paintLayout = new QFormLayout(m_paintGroup);
    paintLayout->setContentsMargins(6, 10, 6, 6);
    paintLayout->setSpacing(4);

    m_colorBtn = new QPushButton("Color", m_paintGroup);
    m_colorBtn->setStyleSheet("QPushButton { background: #cc0000; color: #fff; border: 1px solid #555; padding: 4px; }");
    paintLayout->addRow("Brush Color:", m_colorBtn);

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
    paintLayout->addRow("Brush Size:", sizeWidget);

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
    paintLayout->addRow("Hardness:", hardnessWidget);

    layout->addRow("", m_paintGroup);
}

void LiveryEditorWidget::setupLicensePlatePanel(QWidget* parent, QFormLayout* layout)
{
    m_plateGroup = new QGroupBox("License Plate", parent);
    m_plateGroup->setStyleSheet(
        "QGroupBox { color: #aaaaaa; border: 1px solid #444; font-size: 11px; margin-top: 4px; }"
    );
    QFormLayout* plateLayout = new QFormLayout(m_plateGroup);
    plateLayout->setContentsMargins(6, 10, 6, 6);
    plateLayout->setSpacing(4);

    m_plateText = new QLineEdit(m_plateGroup);
    m_plateText->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    m_plateText->setPlaceholderText("Enter plate text");
    plateLayout->addRow("Text:", m_plateText);

    m_plateCountry = new QComboBox(m_plateGroup);
    m_plateCountry->addItems(LiverySystem::getSupportedCountries());
    m_plateCountry->setStyleSheet("background: #2d2d2d; color: #fff; border: 1px solid #444; padding: 2px;");
    plateLayout->addRow("Country:", m_plateCountry);

    m_generatePlateBtn = new QPushButton("Generate", m_plateGroup);
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
        if (!layer.visible) label += " (hidden)";
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
    QString name = QInputDialog::getText(this, "Create Skin", "Skin name:", QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty()) {
        m_editor->createSkin(name);
    }
}

void LiveryEditorWidget::onDeleteSkin()
{
    QListWidgetItem* item = m_skinList->currentItem();
    if (!item) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Skin",
        QString("Delete skin '%1'?").arg(item->text()),
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
    QString name = QInputDialog::getText(this, "Duplicate Skin", "New name:", QLineEdit::Normal, item->text() + "_copy", &ok);
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

    m_editor->addLayer(layer);
    refreshLayerList();
}

void LiveryEditorWidget::onRemoveLayer()
{
    int row = m_layerList->currentRow();
    if (row < 0) return;
    m_editor->removeLayer(row);
    refreshLayerList();
    clearLayerUI();
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
    QColor color = QColorDialog::getColor(Qt::red, this, "Brush Color");
    if (color.isValid()) {
        m_colorBtn->setStyleSheet(QString("QPushButton { background: %1; color: #fff; border: 1px solid #555; padding: 4px; }").arg(color.name()));
    }
}

void LiveryEditorWidget::onLicensePlateGenerate()
{
    QString text = m_plateText->text().trimmed();
    QString country = m_plateCountry->currentText();

    if (text.isEmpty()) {
        QMessageBox::warning(this, "License Plate", "Please enter plate text.");
        return;
    }

    if (!LiverySystem::isValidPlateText(text, country)) {
        QMessageBox::warning(this, "License Plate", "Invalid plate text for selected country.");
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

} // namespace ks
