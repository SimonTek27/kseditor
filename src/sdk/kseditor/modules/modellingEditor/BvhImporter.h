#pragma once

#include <QString>
#include <QVector>
#include <QVector3D>

namespace ks {

// Minimal Biovision BVH mocap importer (hierarchy + motion channels).
struct BvhJoint {
    QString name;
    int parent = -1;
    QVector<int> children;
    QVector3D offset;
    QStringList channels;     // e.g. Xposition Yposition Zposition Zrotation Xrotation Yrotation
    int channelOffset = 0;    // index of this joint's channels in the frame data
};

class BvhImporter {
public:
    bool load(const QString& path);

    const QVector<BvhJoint>& joints() const { return m_joints; }
    int jointCount() const { return m_joints.size(); }
    int frameCount() const { return m_frameCount; }
    float frameTime() const { return m_frameTime; }
    int channelCount() const { return m_channelCount; }

    // Raw channel values for the given frame.
    QVector<float> frame(int f) const;

private:
    QVector<BvhJoint> m_joints;
    QVector<QVector<float>> m_frames;
    int m_frameCount = 0;
    float m_frameTime = 0.0f;
    int m_channelCount = 0;
};

} // namespace ks
