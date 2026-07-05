#include "TrackEditorWidget.h"
#include <QFormLayout>
#include <QSplitter>

namespace ks {

TrackEditorWidget::TrackEditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(TrackEditor::instance())
    , m_terrainEditor(nullptr)
{
    setupUI();
}

void TrackEditorWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* toolbar = new QHBoxLayout;
    m_addSplineBtn = new QPushButton("Add Spline");
    m_removeSplineBtn = new QPushButton("Remove Spline");
    toolbar->addWidget(m_addSplineBtn);
    toolbar->addWidget(m_removeSplineBtn);
    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    auto* toolGroup = new QGroupBox("Terrain Tools");
    auto* toolLayout = new QFormLayout(toolGroup);

    m_toolCombo = new QComboBox;
    m_toolCombo->addItems({"Raise", "Lower", "Flatten", "Smooth", "Noise", "Sculpt"});
    toolLayout->addRow("Tool:", m_toolCombo);

    m_brushSize = new QDoubleSpinBox;
    m_brushSize->setRange(1.0, 100.0);
    m_brushSize->setValue(10.0);
    m_brushSize->setSuffix(" m");
    toolLayout->addRow("Brush Size:", m_brushSize);

    m_brushStrength = new QDoubleSpinBox;
    m_brushStrength->setRange(0.01, 10.0);
    m_brushStrength->setValue(1.0);
    m_brushStrength->setDecimals(2);
    toolLayout->addRow("Brush Strength:", m_brushStrength);

    mainLayout->addWidget(toolGroup);

    auto* splitter = new QSplitter(Qt::Horizontal);

    m_splineList = new QListWidget;
    m_splineList->setMinimumWidth(180);
    splitter->addWidget(m_splineList);

    m_terrainInfo = new QLabel("No track loaded");
    m_terrainInfo->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_terrainInfo->setWordWrap(true);
    splitter->addWidget(m_terrainInfo);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    connect(m_toolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TrackEditorWidget::onToolChanged);
    connect(m_brushSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TrackEditorWidget::onBrushSizeChanged);
    connect(m_brushStrength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TrackEditorWidget::onBrushStrengthChanged);
    connect(m_splineList, &QListWidget::currentItemChanged, this, &TrackEditorWidget::onSplineSelected);
    connect(m_addSplineBtn, &QPushButton::clicked, this, &TrackEditorWidget::onAddSpline);
    connect(m_removeSplineBtn, &QPushButton::clicked, this, &TrackEditorWidget::onRemoveSpline);
}

void TrackEditorWidget::newTrack(const QString& name, int width, int height) {
    m_editor->newTrack(name, width, height);
    m_terrainInfo->setText(QString("Track: %1\nSize: %2x%3").arg(name).arg(width).arg(height));
    m_splineList->clear();
}

void TrackEditorWidget::loadTrack(const QString& path) {
    m_editor->loadTrack(path);
    m_terrainInfo->setText(QString("Loaded: %1").arg(path));
}

void TrackEditorWidget::saveTrack(const QString& path) {
    m_editor->saveTrack(path);
}

void TrackEditorWidget::onToolChanged(int index) {
    emit toolChanged(index);
}

void TrackEditorWidget::onBrushSizeChanged(double value) {
    if (m_editor) m_editor->setBrushSize(static_cast<float>(value));
    if (m_terrainEditor) m_terrainEditor->setBrushSize(static_cast<float>(value));
    emit terrainModified();
}

void TrackEditorWidget::onBrushStrengthChanged(double value) {
    float v = static_cast<float>(value);
    if (m_editor) m_editor->setBrushStrength(v);
    if (m_terrainEditor) m_terrainEditor->setBrushStrength(v);
    emit terrainModified();
}

void TrackEditorWidget::onSplineSelected(QListWidgetItem* current) {
    if (!current) return;
    for (int i = 0; i < m_splineList->count(); ++i) {
        m_splineList->item(i)->setSelected(m_splineList->item(i) == current);
    }
    emit terrainModified();
}

void TrackEditorWidget::onAddSpline() {
    m_splineList->addItem(QString("Spline %1").arg(m_splineList->count() + 1));
}

void TrackEditorWidget::onRemoveSpline() {
    int idx = m_splineList->currentRow();
    if (idx >= 0) {
        delete m_splineList->takeItem(idx);
    }
}

} // namespace ks
