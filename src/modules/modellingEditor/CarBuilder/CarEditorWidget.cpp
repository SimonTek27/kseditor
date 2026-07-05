#include "CarEditorWidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QSplitter>
#include <QLabel>

namespace ks {

CarEditorWidget::CarEditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(CarEditor::instance())
    , m_updatingUI(false)
{
    setupUI();
    refreshPartList();
}

void CarEditorWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* toolbar = new QHBoxLayout;
    m_addBtn = new QPushButton("Add Part");
    m_removeBtn = new QPushButton("Remove Part");
    toolbar->addWidget(m_addBtn);
    toolbar->addWidget(m_removeBtn);
    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    m_partList = new QListWidget;
    m_partList->setMinimumWidth(180);
    splitter->addWidget(m_partList);

    auto* propsWidget = new QWidget;
    auto* propsLayout = new QFormLayout(propsWidget);
    propsLayout->setContentsMargins(8, 8, 8, 8);

    m_nameEdit = new QLineEdit;
    m_meshFileEdit = new QLineEdit;
    m_meshFileEdit->setReadOnly(true);
    m_parentCombo = new QComboBox;

    propsLayout->addRow("Name:", m_nameEdit);
    propsLayout->addRow("Mesh:", m_meshFileEdit);
    propsLayout->addRow("Parent:", m_parentCombo);

    auto* posGroup = new QGroupBox("Position");
    auto* posLayout = new QHBoxLayout(posGroup);
    m_posX = new QDoubleSpinBox; m_posX->setRange(-10000, 10000); m_posX->setDecimals(3);
    m_posY = new QDoubleSpinBox; m_posY->setRange(-10000, 10000); m_posY->setDecimals(3);
    m_posZ = new QDoubleSpinBox; m_posZ->setRange(-10000, 10000); m_posZ->setDecimals(3);
    posLayout->addWidget(new QLabel("X:")); posLayout->addWidget(m_posX);
    posLayout->addWidget(new QLabel("Y:")); posLayout->addWidget(m_posY);
    posLayout->addWidget(new QLabel("Z:")); posLayout->addWidget(m_posZ);
    propsLayout->addRow(posGroup);

    auto* rotGroup = new QGroupBox("Rotation");
    auto* rotLayout = new QHBoxLayout(rotGroup);
    m_rotX = new QDoubleSpinBox; m_rotX->setRange(-360, 360); m_rotX->setDecimals(1);
    m_rotY = new QDoubleSpinBox; m_rotY->setRange(-360, 360); m_rotY->setDecimals(1);
    m_rotZ = new QDoubleSpinBox; m_rotZ->setRange(-360, 360); m_rotZ->setDecimals(1);
    rotLayout->addWidget(new QLabel("X:")); rotLayout->addWidget(m_rotX);
    rotLayout->addWidget(new QLabel("Y:")); rotLayout->addWidget(m_rotY);
    rotLayout->addWidget(new QLabel("Z:")); rotLayout->addWidget(m_rotZ);
    propsLayout->addRow(rotGroup);

    auto* scaleGroup = new QGroupBox("Scale");
    auto* scaleLayout = new QHBoxLayout(scaleGroup);
    m_scaleX = new QDoubleSpinBox; m_scaleX->setRange(0.001, 1000); m_scaleX->setValue(1.0); m_scaleX->setDecimals(3);
    m_scaleY = new QDoubleSpinBox; m_scaleY->setRange(0.001, 1000); m_scaleY->setValue(1.0); m_scaleY->setDecimals(3);
    m_scaleZ = new QDoubleSpinBox; m_scaleZ->setRange(0.001, 1000); m_scaleZ->setValue(1.0); m_scaleZ->setDecimals(3);
    scaleLayout->addWidget(new QLabel("X:")); scaleLayout->addWidget(m_scaleX);
    scaleLayout->addWidget(new QLabel("Y:")); scaleLayout->addWidget(m_scaleY);
    scaleLayout->addWidget(new QLabel("Z:")); scaleLayout->addWidget(m_scaleZ);
    propsLayout->addRow(scaleGroup);

    m_visibilityBtn = new QPushButton("Visible");
    m_visibilityBtn->setCheckable(true);
    m_visibilityBtn->setChecked(true);
    propsLayout->addRow("Visibility:", m_visibilityBtn);

    splitter->addWidget(propsWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    m_propsGroup = new QGroupBox("Part Properties");

    connect(m_partList, &QListWidget::currentItemChanged, this, &CarEditorWidget::onPartSelected);
    connect(m_addBtn, &QPushButton::clicked, this, &CarEditorWidget::onAddPart);
    connect(m_removeBtn, &QPushButton::clicked, this, &CarEditorWidget::onRemovePart);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &CarEditorWidget::onNameChanged);
    connect(m_posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onPositionChanged);
    connect(m_posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onPositionChanged);
    connect(m_posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onPositionChanged);
    connect(m_rotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onRotationChanged);
    connect(m_rotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onRotationChanged);
    connect(m_rotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onRotationChanged);
    connect(m_scaleX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onScaleChanged);
    connect(m_scaleY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onScaleChanged);
    connect(m_scaleZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CarEditorWidget::onScaleChanged);
    connect(m_parentCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &CarEditorWidget::onParentChanged);
    connect(m_visibilityBtn, &QPushButton::toggled, this, [this](bool checked) {
        onVisibilityChanged(checked ? Qt::Checked : Qt::Unchecked);
    });
}

void CarEditorWidget::loadCar(const QString& path) {
    m_editor->loadCar(path);
    refreshPartList();
}

void CarEditorWidget::saveCar(const QString& path) {
    m_editor->saveCar(path);
}

void CarEditorWidget::refreshPartList() {
    m_partList->clear();
    m_parentCombo->clear();
    m_parentCombo->addItem("(none)");

    QStringList parts = m_editor->getCarParts();
    for (const QString& partId : parts) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            m_partList->addItem(part->name);
            m_parentCombo->addItem(part->name);
        }
    }
}

void CarEditorWidget::onPartSelected(QListWidgetItem* current) {
    if (!current) { clearPartUI(); return; }
    int idx = m_partList->row(current);
    QStringList parts = m_editor->getCarParts();
    if (idx >= 0 && idx < parts.size()) {
        m_editor->setCurrentPart(parts[idx]);
        updatePartUI();
        emit partSelected(parts[idx]);
    }
}

void CarEditorWidget::onAddPart() {
    CarPart part;
    part.name = "New Part";
    part.id = QString("part_%1").arg(QDateTime::currentMSecsSinceEpoch());
    m_editor->addPart(part);
    refreshPartList();
    emit carModified();
}

void CarEditorWidget::onRemovePart() {
    int idx = m_partList->currentRow();
    QStringList parts = m_editor->getCarParts();
    if (idx >= 0 && idx < parts.size()) {
        m_editor->removePart(parts[idx]);
        refreshPartList();
        clearPartUI();
        emit carModified();
    }
}

void CarEditorWidget::onNameChanged(const QString& text) {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            part->name = text;
            refreshPartList();
            emit carModified();
        }
    }
}

void CarEditorWidget::onPositionChanged() {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        m_editor->updatePartTransform(partId,
            QVector3D(m_posX->value(), m_posY->value(), m_posZ->value()),
            QVector3D(m_rotX->value(), m_rotY->value(), m_rotZ->value()));
        emit carModified();
    }
}

void CarEditorWidget::onRotationChanged() {
    if (m_updatingUI) return;
    onPositionChanged();
}

void CarEditorWidget::onScaleChanged() {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            part->scale = QVector3D(m_scaleX->value(), m_scaleY->value(), m_scaleZ->value());
            emit carModified();
        }
    }
}

void CarEditorWidget::onMeshFileChanged(const QString& text) {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            part->meshFile = text;
            emit carModified();
        }
    }
}

void CarEditorWidget::onParentChanged(const QString& text) {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            part->parentId = text;
            emit carModified();
        }
    }
}

void CarEditorWidget::onVisibilityChanged(int state) {
    if (m_updatingUI) return;
    QString partId = m_editor->currentPart();
    if (!partId.isEmpty()) {
        CarPart* part = m_editor->getPart(partId);
        if (part) {
            part->visible = (state == Qt::Checked);
            emit carModified();
        }
    }
}

void CarEditorWidget::updatePartUI() {
    m_updatingUI = true;
    QString partId = m_editor->currentPart();
    CarPart* part = m_editor->getPart(partId);
    if (part) {
        m_nameEdit->setText(part->name);
        m_meshFileEdit->setText(part->meshFile);
        m_posX->setValue(part->position.x());
        m_posY->setValue(part->position.y());
        m_posZ->setValue(part->position.z());
        m_rotX->setValue(part->rotation.x());
        m_rotY->setValue(part->rotation.y());
        m_rotZ->setValue(part->rotation.z());
        m_scaleX->setValue(part->scale.x());
        m_scaleY->setValue(part->scale.y());
        m_scaleZ->setValue(part->scale.z());
        m_visibilityBtn->setChecked(part->visible);
    }
    m_updatingUI = false;
}

void CarEditorWidget::clearPartUI() {
    m_updatingUI = true;
    m_nameEdit->clear();
    m_meshFileEdit->clear();
    m_posX->setValue(0); m_posY->setValue(0); m_posZ->setValue(0);
    m_rotX->setValue(0); m_rotY->setValue(0); m_rotZ->setValue(0);
    m_scaleX->setValue(1); m_scaleY->setValue(1); m_scaleZ->setValue(1);
    m_visibilityBtn->setChecked(true);
    m_updatingUI = false;
}

} // namespace ks
