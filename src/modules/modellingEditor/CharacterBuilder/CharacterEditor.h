#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <QPair>

namespace ks {

struct Bone {
    QString name;
    int parentIndex = -1;
    QVector3D head = QVector3D(0, 0, 0);
    QVector3D tail = QVector3D(0, 1, 0);
    QVector3D length = QVector3D(0, 1, 0);
    QQuaternion rotation;
    float roll = 0.0f;
    QMatrix4x4 localMatrix;
    QMatrix4x4 worldMatrix;
    bool selected = false;
    bool visible = true;
};

struct VertexWeight {
    int boneIndex = -1;
    float weight = 0.0f;
};

struct CharacterSkeleton {
    QString name;
    QVector<Bone> bones;
    QVector<QVector<VertexWeight>> vertexWeights;
    
    int addBone(const QString& name, int parentIndex = -1) {
        Bone bone;
        bone.name = name;
        bone.parentIndex = parentIndex;
        bones.append(bone);
        return bones.size() - 1;
    }
    
    void updateMatrices() {
        for (int i = 0; i < bones.size(); ++i) {
            updateBoneMatrix(i);
        }
    }
    
    void updateBoneMatrix(int index) {
        if (index < 0 || index >= bones.size()) return;
        
        Bone& bone = bones[index];
        
        QMatrix4x4 local;
        local.translate(bone.head);
        local.rotate(bone.rotation);
        bone.localMatrix = local;
        
        if (bone.parentIndex >= 0) {
            bone.worldMatrix = bones[bone.parentIndex].worldMatrix * local;
        } else {
            bone.worldMatrix = local;
        }
    }
    
    QVector3D getBonePosition(int index) const {
        if (index < 0 || index >= bones.size()) return QVector3D();
        return bones[index].head;
    }
    
    QVector3D getBoneDirection(int index) const {
        if (index < 0 || index >= bones.size()) return QVector3D();
        QVector3D dir = bones[index].tail - bones[index].head;
        return dir.normalized();
    }
};

struct CharacterPose {
    QString name;
    QMap<int, QQuaternion> boneRotations;
    QMap<int, QVector3D> boneTranslations;
};

class CharacterEditor : public QObject {
    Q_OBJECT
public:
    static CharacterEditor* instance();
    
    CharacterSkeleton* skeleton() { return &m_skeleton; }
    void setSkeleton(const CharacterSkeleton& skeleton) { m_skeleton = skeleton; }
    
    void createHumanoidSkeleton(float height = 1.8f);
    void createQuadrupedSkeleton(float height = 1.5f, float length = 2.0f);
    
    int addBone(const QString& name, int parentIndex, const QVector3D& position);
    void removeBone(int index);
    void setBoneParent(int boneIndex, int newParentIndex);
    
    void selectBone(int index);
    void deselectAll();
    int selectedBone() const { return m_selectedBone; }
    QVector<int> selectedBones() const;
    
    void moveBone(int index, const QVector3D& delta);
    void rotateBone(int index, const QQuaternion& rotation);
    
    QVector<QVector3D> getBoneWorldPositions() const;
    QVector<QPair<QVector3D, QVector3D>> getBoneSegments() const;
    
    void addPose(const QString& name);
    void applyPose(const QString& name);
    void removePose(const QString& name);
    CharacterPose getCurrentPose() const;
    QStringList poses() const { return m_poses.keys(); }
    
    void bindToMesh(const QString& meshId);
    void paintWeights(int boneIndex, const QVector3D& center, float radius, float strength);
    void smoothWeights(int boneIndex, float radius);
    void normalizeWeights();
    
    QVector<VertexWeight> getVertexWeights(int vertexIndex) const;
    void setVertexWeights(int vertexIndex, const QVector<VertexWeight>& weights);
    
signals:
    void skeletonModified();
    void boneSelected(int index);
    void weightsModified();

private:
    explicit CharacterEditor(QObject* parent = nullptr);
    static CharacterEditor* s_instance;
    
    CharacterSkeleton m_skeleton;
    int m_selectedBone = -1;
    int m_nextBoneId = 0;
    
    QMap<QString, CharacterPose> m_poses;
    QMap<QString, QVector<QVector<VertexWeight>>> m_meshWeights;
    QVector<QVector3D> m_meshPositions;
    int m_boneCount = 0;
};

class SkeletonTool {
public:
    static QVector3D calculateBoneAxis(const Bone& bone);
    static QQuaternion lookAt(const QVector3D& from, const QVector3D& to, const QVector3D& up);
    static QVector3D quaternionToEuler(const QQuaternion& q);
    static QQuaternion eulerToQuaternion(const QVector3D& euler);
    
    static float distanceToLine(const QVector3D& point, const QVector3D& lineStart, const QVector3D& lineEnd);
    static QVector3D closestPointOnLine(const QVector3D& point, const QVector3D& lineStart, const QVector3D& lineEnd);
    
    static bool solveTwoBoneIK(const QVector3D& root, const QVector3D& midTarget,
                               const QVector3D& endTarget, float upperLen, float lowerLen,
                               QVector3D& outMid, QVector3D& outEnd);
    
    static QVector<QVector3D> interpolatePath(const QVector<QVector3D>& from, 
                                               const QVector<QVector3D>& to, float t);
    
    static void applyFK(CharacterSkeleton& skeleton, int boneIndex, const QVector3D& rotation);
    static void applyIK(CharacterSkeleton& skeleton, int rootBone, int endBone, 
                       const QVector3D& target, bool stretch = false);
};

} // namespace ks