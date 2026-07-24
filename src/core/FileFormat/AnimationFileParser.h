#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include "Math/MathCore.h"

namespace ks {

struct AnimKeyframe {
    float time;
    float value;
    Vec3 value3;
    Vec4 value4; // or Quat
    QString interpolation = "linear";
};

struct AnimChannel {
    QString name;
    QString targetPath;
    QString property; // "translation", "rotation", "scale", "color", "visibility", "custom"
    int componentCount = 1; // 1=scalar, 3=vec3, 4=vec4/quat

    QVector<AnimKeyframe> keyframes;
};

struct AnimClip {
    QString name;
    float duration = 1.0f;
    float startTime = 0.0f;
    float endTime = 1.0f;
    bool loop = false;
    QString wrapMode = "clamp";

    QVector<AnimChannel> channels;
};

struct AnimFile {
    QString version = "1.0";
    QVector<AnimClip> clips;
    QMap<QString, QString> metadata;
};

class AnimationFileParser {
public:
    static bool load(const QString& filePath, AnimFile& outAnim);
    static bool save(const QString& filePath, const AnimFile& anim);
    static QString lastError() { return s_lastError; }

    // Utilities
    static float sampleChannel(const AnimChannel& channel, float time);
    static Vec3 sampleChannel3(const AnimChannel& channel, float time);
    static Quat sampleRotation(const AnimChannel& channel, float time);
    static int findKeyframeIndex(const QVector<AnimKeyframe>& keyframes, float time);

private:
    static QString s_lastError;
};

} // namespace ks