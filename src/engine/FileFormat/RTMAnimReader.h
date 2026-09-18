#pragma once

#include "BISBinaryReader.h"
#include <QString>
#include <QVector>
#include <QQuaternion>
#include <QVector3D>
#include <QMatrix4x4>

namespace ks::fileformat {

// ============================================================================
// RTM Animation Reader - Rewritten from CWR Poseidon RTMAnimReader
// ============================================================================

struct RTMBone {
    QString name;
    int parentIndex = -1;
    QVector3D position;
    QQuaternion rotation;
    QMatrix4x4 inverseRestPose;
};

struct RTMKeyframe {
    float time = 0;
    QVector<QVector3D> bonePositions;
    QVector<QQuaternion> boneRotations;
};

struct RTMAnimation {
    QString name;
    QVector<RTMBone> bones;
    QVector<RTMKeyframe> keyframes;

    bool isValid() const { return !bones.isEmpty() && !keyframes.isEmpty(); }

    QMatrix4x4 getBoneTransform(const RTMBone& bone,
                                 const QVector3D& pos,
                                 const QQuaternion& rot) const {
        QMatrix4x4 m;
        m.translate(pos);
        m.rotate(rot);
        return m;
    }

    QMatrix4x4 getFullBoneTransform(int boneIndex,
                                     const RTMKeyframe& frame,
                                     const QVector<QMatrix4x4>& restPoseMatrices) const {
        if (boneIndex < 0 || boneIndex >= bones.size())
            return QMatrix4x4();

        const RTMBone& bone = bones[boneIndex];
        QMatrix4x4 localTransform = getBoneTransform(bone,
                                                      frame.bonePositions[boneIndex],
                                                      frame.boneRotations[boneIndex]);

        QMatrix4x4 result = localTransform;
        int parent = bone.parentIndex;
        while (parent >= 0 && parent < bones.size()) {
            const RTMBone& parentBone = bones[parent];
            result = getBoneTransform(parentBone,
                                       frame.bonePositions[parent],
                                       frame.boneRotations[parent]) * result;
            parent = parentBone.parentIndex;
        }

        if (boneIndex < restPoseMatrices.size())
            result = result * restPoseMatrices[boneIndex];

        return result;
    }
};

class RTMAnimReader {
public:
    RTMAnimation load(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return loadFromDevice(file);
    }

    RTMAnimation loadFromDevice(QIODevice& device) {
        BinaryReader reader(device);
        RTMAnimation anim;

        // Read header: "RTM\0" + version(int32)
        char sig[4];
        reader.readBytes(sig, 4);
        if (memcmp(sig, "RTM\0", 4) != 0)
            return {};

        int32_t version;
        reader.read(version);

        // Read bone count and keyframe count
        int32_t boneCount;
        reader.read(boneCount);

        int32_t keyframeCount;
        reader.read(keyframeCount);

        // Read bones
        anim.bones.resize(boneCount);
        for (int i = 0; i < boneCount; ++i) {
            RTMBone& bone = anim.bones[i];
            bone.name = reader.readString();
            reader.read(bone.parentIndex);
            read(reader, bone.position);

            // Read quaternion
            BisVector4 q;
            read(reader, q);
            bone.rotation = QQuaternion(q.w, q.x, q.y, q.z);

            // Read inverse rest pose matrix (4x3)
            BisMatrix4x3 invRest;
            read(reader, invRest);
            for (int r = 0; r < 4; ++r) {
                bone.inverseRestPose(r, 0) = invRest.rows[r].x;
                bone.inverseRestPose(r, 1) = invRest.rows[r].y;
                bone.inverseRestPose(r, 2) = invRest.rows[r].z;
            }
            bone.inverseRestPose(3, 3) = 1.0f;
        }

        // Read keyframes
        anim.keyframes.resize(keyframeCount);
        for (int i = 0; i < keyframeCount; ++i) {
            RTMKeyframe& frame = anim.keyframes[i];
            reader.read(frame.time);

            frame.bonePositions.resize(boneCount);
            frame.boneRotations.resize(boneCount);
            for (int j = 0; j < boneCount; ++j) {
                read(reader, frame.bonePositions[j]);
                BisVector4 q;
                read(reader, q);
                frame.boneRotations[j] = QQuaternion(q.w, q.x, q.y, q.z);
            }
        }

        return anim;
    }
};

} // namespace ks::fileformat
