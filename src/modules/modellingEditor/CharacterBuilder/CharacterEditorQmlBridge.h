#pragma once

#include <QObject>
#include <QStringList>
#include <QVector3D>

namespace ks {

class CharacterEditor;

class CharacterEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList boneNames READ boneNames NOTIFY skeletonModified)
    Q_PROPERTY(bool hasSkeleton READ hasSkeleton NOTIFY skeletonModified)
    Q_PROPERTY(int boneCount READ boneCount NOTIFY skeletonModified)
    Q_PROPERTY(QStringList poseList READ poseList NOTIFY skeletonModified)

public:
    static CharacterEditorQmlBridge* instance();

    QStringList boneNames() const;
    bool hasSkeleton() const;
    int boneCount() const;
    QStringList poseList() const;

    Q_INVOKABLE void createHumanoid(float height = 1.8f);
    Q_INVOKABLE void createQuadruped(float height = 1.5f, float length = 2.0f);
    Q_INVOKABLE void addBone(const QString& name, int parentIndex, float x, float y, float z);
    Q_INVOKABLE void removeBone(int index);
    Q_INVOKABLE void selectBone(int index);
    Q_INVOKABLE int boneParent(int index);
    Q_INVOKABLE void moveBone(int index, float dx, float dy, float dz);
    Q_INVOKABLE void rotateBone(int index, float rx, float ry, float rz);
    Q_INVOKABLE void paintWeights(int boneIndex, float cx, float cy, float cz, float radius, float strength);
    Q_INVOKABLE void smoothWeights(int boneIndex, float radius);
    Q_INVOKABLE void normalizeWeights();
    Q_INVOKABLE void bindToMesh(const QString& meshId);
    Q_INVOKABLE void savePose(const QString& name);
    Q_INVOKABLE void applyPose(const QString& name);
    Q_INVOKABLE void removePose(const QString& name);

signals:
    void skeletonModified();
    void boneSelected(int index);
    void weightsModified();

private:
    explicit CharacterEditorQmlBridge(QObject* parent = nullptr);
    static CharacterEditorQmlBridge* s_instance;
    CharacterEditor* m_editor;
};

} // namespace ks
