#include "CharacterEditorQmlBridge.h"
#include "CharacterEditor.h"
#include <QQuaternion>

namespace ks {

CharacterEditorQmlBridge* CharacterEditorQmlBridge::s_instance = nullptr;

CharacterEditorQmlBridge::CharacterEditorQmlBridge(QObject* parent)
    : QObject(parent)
    , m_editor(CharacterEditor::instance())
{
    connect(m_editor, &CharacterEditor::skeletonModified, this, &CharacterEditorQmlBridge::skeletonModified);
    connect(m_editor, &CharacterEditor::boneSelected, this, &CharacterEditorQmlBridge::boneSelected);
    connect(m_editor, &CharacterEditor::weightsModified, this, &CharacterEditorQmlBridge::weightsModified);
}

CharacterEditorQmlBridge* CharacterEditorQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new CharacterEditorQmlBridge();
    }
    return s_instance;
}

QStringList CharacterEditorQmlBridge::boneNames() const {
    QStringList names;
    if (!m_editor->skeleton()) return names;
    for (const auto& bone : m_editor->skeleton()->bones) {
        names << bone.name;
    }
    return names;
}

bool CharacterEditorQmlBridge::hasSkeleton() const {
    return m_editor->skeleton() && m_editor->skeleton()->bones.size() > 0;
}

int CharacterEditorQmlBridge::boneCount() const {
    return m_editor->skeleton() ? m_editor->skeleton()->bones.size() : 0;
}

QStringList CharacterEditorQmlBridge::poseList() const {
    return m_editor->poses();
}

void CharacterEditorQmlBridge::createHumanoid(float height) {
    m_editor->createHumanoidSkeleton(height);
}

void CharacterEditorQmlBridge::createQuadruped(float height, float length) {
    m_editor->createQuadrupedSkeleton(height, length);
}

void CharacterEditorQmlBridge::addBone(const QString& name, int parentIndex, float x, float y, float z) {
    m_editor->addBone(name, parentIndex, QVector3D(x, y, z));
}

void CharacterEditorQmlBridge::removeBone(int index) {
    m_editor->removeBone(index);
}

void CharacterEditorQmlBridge::selectBone(int index) {
    m_editor->selectBone(index);
}

int CharacterEditorQmlBridge::boneParent(int index) {
    if (!m_editor->skeleton()) return -1;
    const auto& bones = m_editor->skeleton()->bones;
    if (index < 0 || index >= bones.size()) return -1;
    return bones[index].parentIndex;
}

void CharacterEditorQmlBridge::moveBone(int index, float dx, float dy, float dz) {
    m_editor->moveBone(index, QVector3D(dx, dy, dz));
}

void CharacterEditorQmlBridge::rotateBone(int index, float rx, float ry, float rz) {
    QQuaternion rot = QQuaternion::fromEulerAngles(rx, ry, rz);
    m_editor->rotateBone(index, rot);
}

void CharacterEditorQmlBridge::paintWeights(int boneIndex, float cx, float cy, float cz, float radius, float strength) {
    m_editor->paintWeights(boneIndex, QVector3D(cx, cy, cz), radius, strength);
}

void CharacterEditorQmlBridge::smoothWeights(int boneIndex, float radius) {
    m_editor->smoothWeights(boneIndex, radius);
}

void CharacterEditorQmlBridge::normalizeWeights() {
    m_editor->normalizeWeights();
}

void CharacterEditorQmlBridge::mirrorWeights(int axis) {
    m_editor->mirrorWeights(axis, 0.01f);
}

void CharacterEditorQmlBridge::bindToMesh(const QString& meshId) {
    m_editor->bindToMesh(meshId);
}

void CharacterEditorQmlBridge::savePose(const QString& name) {
    m_editor->addPose(name);
}

void CharacterEditorQmlBridge::applyPose(const QString& name) {
    m_editor->applyPose(name);
}

void CharacterEditorQmlBridge::removePose(const QString& name) {
    m_editor->removePose(name);
}

} // namespace ks
