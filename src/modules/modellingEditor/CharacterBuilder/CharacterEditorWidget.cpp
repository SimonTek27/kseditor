#include "CharacterEditorWidget.h"
#include <QSplitter>
#include <QLineEdit>
#include <QFormLayout>

namespace ks {

CharacterEditorWidget::CharacterEditorWidget(QWidget* parent)
    : QWidget(parent)
    , m_editor(CharacterEditor::instance())
    , m_updatingUI(false)
{
    setupUI();
    refreshBoneList();
}

void CharacterEditorWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* toolbar = new QHBoxLayout;
    m_addBtn = new QPushButton("Add Bone");
    m_removeBtn = new QPushButton("Remove Bone");
    toolbar->addWidget(m_addBtn);
    toolbar->addWidget(m_removeBtn);
    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    auto* skeletonGroup = new QGroupBox("Skeleton Type");
    auto* skelLayout = new QHBoxLayout(skeletonGroup);
    m_skeletonTypeCombo = new QComboBox;
    m_skeletonTypeCombo->addItems({"Humanoid", "Quadruped"});
    m_createBtn = new QPushButton("Create");
    skelLayout->addWidget(m_skeletonTypeCombo);
    skelLayout->addWidget(m_createBtn);
    mainLayout->addWidget(skeletonGroup);

    auto* splitter = new QSplitter(Qt::Horizontal);

    m_boneList = new QListWidget;
    m_boneList->setMinimumWidth(180);
    splitter->addWidget(m_boneList);

    auto* propsWidget = new QWidget;
    auto* propsLayout = new QFormLayout(propsWidget);
    propsLayout->setContentsMargins(8, 8, 8, 8);

    m_nameEdit = new QLineEdit;
    propsLayout->addRow("Name:", m_nameEdit);

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

    m_boneInfo = new QLabel("Select a bone to edit");
    m_boneInfo->setWordWrap(true);
    propsLayout->addRow(m_boneInfo);

    splitter->addWidget(propsWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    connect(m_boneList, &QListWidget::currentItemChanged, this, &CharacterEditorWidget::onBoneSelected);
    connect(m_addBtn, &QPushButton::clicked, this, &CharacterEditorWidget::onAddBone);
    connect(m_removeBtn, &QPushButton::clicked, this, &CharacterEditorWidget::onRemoveBone);
    connect(m_createBtn, &QPushButton::clicked, this, [this]() {
        if (m_skeletonTypeCombo->currentIndex() == 0) onCreateHumanoid();
        else onCreateQuadruped();
    });
    connect(m_nameEdit, &QLineEdit::textChanged, this, &CharacterEditorWidget::onNameChanged);
    connect(m_posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onPositionChanged);
    connect(m_posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onPositionChanged);
    connect(m_posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onPositionChanged);
    connect(m_rotX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onRotationChanged);
    connect(m_rotY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onRotationChanged);
    connect(m_rotZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CharacterEditorWidget::onRotationChanged);
}

void CharacterEditorWidget::createHumanoid(float height) {
    m_editor->createHumanoidSkeleton(height);
    refreshBoneList();
    emit skeletonModified();
}

void CharacterEditorWidget::createQuadruped(float height, float length) {
    m_editor->createQuadrupedSkeleton(height, length);
    refreshBoneList();
    emit skeletonModified();
}

CharacterSkeleton* CharacterEditorWidget::skeleton() const {
    return m_editor->skeleton();
}

void CharacterEditorWidget::applyFK(int boneIndex, const QVector3D& rotation) {
    SkeletonTool::applyFK(*m_editor->skeleton(), boneIndex, rotation);
    emit skeletonModified();
}

void CharacterEditorWidget::applyIK(int rootBone, int endBone, const QVector3D& target, bool stretch) {
    SkeletonTool::applyIK(*m_editor->skeleton(), rootBone, endBone, target, stretch);
    emit skeletonModified();
}

void CharacterEditorWidget::refreshBoneList() {
    m_boneList->clear();
    CharacterSkeleton* skel = m_editor->skeleton();
    if (!skel) return;

    for (int i = 0; i < skel->bones.size(); ++i) {
        const Bone& bone = skel->bones[i];
        QString prefix;
        if (bone.parentIndex >= 0) prefix = "  ";
        m_boneList->addItem(prefix + bone.name);
    }
}

void CharacterEditorWidget::onBoneSelected(QListWidgetItem* current) {
    if (!current) { clearBoneUI(); return; }
    int idx = m_boneList->row(current);
    m_editor->selectBone(idx);
    updateBoneUI();
    emit boneSelected(idx);
}

void CharacterEditorWidget::onAddBone() {
    int parentIdx = m_editor->selectedBone();
    QString name = QString("Bone_%1").arg(m_editor->skeleton()->bones.size());
    m_editor->addBone(name, parentIdx, QVector3D(0, -0.1f, 0));
    refreshBoneList();
    emit skeletonModified();
}

void CharacterEditorWidget::onRemoveBone() {
    int idx = m_boneList->currentRow();
    if (idx >= 0) {
        m_editor->removeBone(idx);
        refreshBoneList();
        clearBoneUI();
        emit skeletonModified();
    }
}

void CharacterEditorWidget::onNameChanged(const QString& text) {
    if (m_updatingUI) return;
    int idx = m_editor->selectedBone();
    CharacterSkeleton* skel = m_editor->skeleton();
    if (idx >= 0 && idx < skel->bones.size()) {
        skel->bones[idx].name = text;
        refreshBoneList();
        emit skeletonModified();
    }
}

void CharacterEditorWidget::onPositionChanged() {
    if (m_updatingUI) return;
    int idx = m_editor->selectedBone();
    CharacterSkeleton* skel = m_editor->skeleton();
    if (idx >= 0 && idx < skel->bones.size()) {
        QVector3D newPos(m_posX->value(), m_posY->value(), m_posZ->value());
        QVector3D delta = newPos - skel->bones[idx].head;
        m_editor->moveBone(idx, delta);
        skel->updateMatrices();
        emit skeletonModified();
    }
}

void CharacterEditorWidget::onRotationChanged() {
    if (m_updatingUI) return;
    int idx = m_editor->selectedBone();
    CharacterSkeleton* skel = m_editor->skeleton();
    if (idx >= 0 && idx < skel->bones.size()) {
        QVector3D euler(m_rotX->value(), m_rotY->value(), m_rotZ->value());
        QQuaternion rot = SkeletonTool::eulerToQuaternion(euler);
        m_editor->rotateBone(idx, rot);
        skel->updateMatrices();
        emit skeletonModified();
    }
}

void CharacterEditorWidget::onCreateHumanoid() {
    createHumanoid();
}

void CharacterEditorWidget::onCreateQuadruped() {
    createQuadruped();
}

void CharacterEditorWidget::updateBoneUI() {
    m_updatingUI = true;
    int idx = m_editor->selectedBone();
    CharacterSkeleton* skel = m_editor->skeleton();
    if (idx >= 0 && idx < skel->bones.size()) {
        const Bone& bone = skel->bones[idx];
        m_nameEdit->setText(bone.name);
        m_posX->setValue(bone.head.x());
        m_posY->setValue(bone.head.y());
        m_posZ->setValue(bone.head.z());
        QVector3D euler = SkeletonTool::quaternionToEuler(bone.rotation);
        m_rotX->setValue(euler.x());
        m_rotY->setValue(euler.y());
        m_rotZ->setValue(euler.z());
        m_boneInfo->setText(QString("Bone %1/%2\nParent: %3")
            .arg(idx + 1).arg(skel->bones.size())
            .arg(bone.parentIndex >= 0 ? skel->bones[bone.parentIndex].name : "root"));
    }
    m_updatingUI = false;
}

void CharacterEditorWidget::clearBoneUI() {
    m_updatingUI = true;
    m_nameEdit->clear();
    m_posX->setValue(0); m_posY->setValue(0); m_posZ->setValue(0);
    m_rotX->setValue(0); m_rotY->setValue(0); m_rotZ->setValue(0);
    m_boneInfo->setText("Select a bone to edit");
    m_updatingUI = false;
}

} // namespace ks
