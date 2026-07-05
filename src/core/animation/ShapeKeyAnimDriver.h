#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include "../mesh/MeshOperations.h"
#include "../mesh/ShapeKeyData.h"
#include "AnimationSystem.h"

namespace ks {

struct ShapeKeyAnimChannel {
    QString shapeKeyName;
    GraphCurve curve;
    int targetIndex = -1;
};

class ShapeKeyAnimDriver : public QObject {
    Q_OBJECT

public:
    explicit ShapeKeyAnimDriver(QObject* parent = nullptr);

    void setTargetMesh(MeshData* mesh);
    MeshData* targetMesh() const { return m_mesh; }

    void addChannel(const QString& shapeKeyName);
    void removeChannel(const QString& shapeKeyName);
    void clearChannels();

    int channelCount() const { return m_channels.size(); }
    QStringList channelNames() const;

    void setKeyframe(const QString& shapeKeyName, int frame, float value, Keyframe::Interpolation interp = Keyframe::InterpolationLinear);
    void removeKeyframe(const QString& shapeKeyName, int frame);
    void clearKeyframes(const QString& shapeKeyName);

    void evaluateAtFrame(int frame);
    void evaluateAtTime(float timeSeconds, int fps = 30);

    QVector<ShapeKeyAnimChannel> channels() const { return m_channels; }

    void connectToTimeline(AnimationTimeline* timeline);
    void disconnectFromTimeline();

signals:
    void channelAdded(const QString& name);
    void channelRemoved(const QString& name);
    void evaluationApplied();
    void weightChanged(const QString& name, float weight);

private:
    void applyShapeKeyWeight(const QString& name, float weight);

    MeshData* m_mesh = nullptr;
    QVector<ShapeKeyAnimChannel> m_channels;
    AnimationTimeline* m_timeline = nullptr;

private slots:
    void onFrameChanged(int frame);
};

inline ShapeKeyAnimDriver::ShapeKeyAnimDriver(QObject* parent)
    : QObject(parent) {}

inline void ShapeKeyAnimDriver::setTargetMesh(MeshData* mesh) {
    m_mesh = mesh;
}

inline void ShapeKeyAnimDriver::addChannel(const QString& shapeKeyName) {
    for (const auto& ch : m_channels) {
        if (ch.shapeKeyName == shapeKeyName) return;
    }
    ShapeKeyAnimChannel ch;
    ch.shapeKeyName = shapeKeyName;
    if (m_mesh) {
        ch.targetIndex = ShapeKeyManager::getShapeKeyIndexByName(*m_mesh, shapeKeyName);
    }
    m_channels.append(ch);
    emit channelAdded(shapeKeyName);
}

inline void ShapeKeyAnimDriver::removeChannel(const QString& shapeKeyName) {
    for (int i = 0; i < m_channels.size(); ++i) {
        if (m_channels[i].shapeKeyName == shapeKeyName) {
            m_channels.removeAt(i);
            emit channelRemoved(shapeKeyName);
            return;
        }
    }
}

inline void ShapeKeyAnimDriver::clearChannels() {
    m_channels.clear();
}

inline QStringList ShapeKeyAnimDriver::channelNames() const {
    QStringList names;
    for (const auto& ch : m_channels) {
        names.append(ch.shapeKeyName);
    }
    return names;
}

inline void ShapeKeyAnimDriver::setKeyframe(const QString& shapeKeyName, int frame, float value, Keyframe::Interpolation interp) {
    for (auto& ch : m_channels) {
        if (ch.shapeKeyName == shapeKeyName) {
            Keyframe kf;
            kf.frame = frame;
            kf.value = value;
            kf.interpolation = interp;
            ch.curve.addKeyframe(kf);
            return;
        }
    }
}

inline void ShapeKeyAnimDriver::removeKeyframe(const QString& shapeKeyName, int frame) {
    for (auto& ch : m_channels) {
        if (ch.shapeKeyName == shapeKeyName) {
            ch.curve.removeKeyframe(frame);
            return;
        }
    }
}

inline void ShapeKeyAnimDriver::clearKeyframes(const QString& shapeKeyName) {
    for (auto& ch : m_channels) {
        if (ch.shapeKeyName == shapeKeyName) {
            ch.curve.keyframes.clear();
            return;
        }
    }
}

inline void ShapeKeyAnimDriver::evaluateAtFrame(int frame) {
    for (const auto& ch : m_channels) {
        if (ch.curve.keyframes.isEmpty()) continue;
        float value = ch.curve.evaluate((float)frame);
        applyShapeKeyWeight(ch.shapeKeyName, value);
    }
    if (m_mesh) {
        ShapeKeyManager::applyAllShapeKeys(*m_mesh);
    }
    emit evaluationApplied();
}

inline void ShapeKeyAnimDriver::evaluateAtTime(float timeSeconds, int fps) {
    int frame = qRound(timeSeconds * fps);
    evaluateAtFrame(frame);
}

inline void ShapeKeyAnimDriver::applyShapeKeyWeight(const QString& name, float weight) {
    if (!m_mesh) return;
    int idx = ShapeKeyManager::getShapeKeyIndexByName(*m_mesh, name);
    if (idx < 0) return;
    ShapeKeyManager::setShapeKeyWeight(*m_mesh, idx, weight);
    emit weightChanged(name, weight);
}

inline void ShapeKeyAnimDriver::connectToTimeline(AnimationTimeline* timeline) {
    if (m_timeline) disconnectFromTimeline();
    m_timeline = timeline;
    if (m_timeline) {
        connect(m_timeline, &AnimationTimeline::currentFrameChanged, this, &ShapeKeyAnimDriver::onFrameChanged);
    }
}

inline void ShapeKeyAnimDriver::disconnectFromTimeline() {
    if (m_timeline) {
        disconnect(m_timeline, &AnimationTimeline::currentFrameChanged, this, &ShapeKeyAnimDriver::onFrameChanged);
        m_timeline = nullptr;
    }
}

inline void ShapeKeyAnimDriver::onFrameChanged(int frame) {
    evaluateAtFrame(frame);
}

} // namespace ks
