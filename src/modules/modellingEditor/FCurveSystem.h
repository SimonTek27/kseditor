#pragma once

#include <QVector>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QtGlobal>
#include <cmath>
#include <algorithm>

namespace ks {

// Interpolation between two keyframes on an F-Curve.
enum class FCurveInterp {
    Linear,    // straight line between keys
    Step,      // hold previous value until next key
    Cubic,     // smooth curve using in/out tangents (Hermite)
    EaseIn,    // accelerate
    EaseOut,   // decelerate
    EaseInOut  // both
};

inline QString fcurveInterpToString(FCurveInterp i) {
    switch (i) {
    case FCurveInterp::Linear:    return "Linear";
    case FCurveInterp::Step:      return "Step";
    case FCurveInterp::Cubic:     return "Cubic";
    case FCurveInterp::EaseIn:    return "EaseIn";
    case FCurveInterp::EaseOut:   return "EaseOut";
    case FCurveInterp::EaseInOut: return "EaseInOut";
    }
    return "Linear";
}

inline FCurveInterp fcurveInterpFromString(const QString& s) {
    if (s == "Step")      return FCurveInterp::Step;
    if (s == "Cubic")     return FCurveInterp::Cubic;
    if (s == "EaseIn")    return FCurveInterp::EaseIn;
    if (s == "EaseOut")   return FCurveInterp::EaseOut;
    if (s == "EaseInOut") return FCurveInterp::EaseInOut;
    return FCurveInterp::Linear;
}

// One keyframe: frame index, value, interpolation mode, optional tangents.
struct FCurveKey {
    float frame = 0.0f;
    float value = 0.0f;
    FCurveInterp interpolation = FCurveInterp::Cubic;
    float inTangent = 0.0f;   // slope (value per frame) entering the key
    float outTangent = 0.0f;  // slope leaving the key
    bool locked = false;

    QVariantMap toVariant() const;
};

// A single animated channel (e.g. position.x). Keys sorted by frame.
struct FCurveChannel {
    QString name;
    QVector<FCurveKey> keys;

    void clear() { keys.clear(); }
    bool isEmpty() const { return keys.isEmpty(); }
    int size() const { return keys.size(); }

    // Adds/replaces a key at `frame`.
    void setKey(float frame, float value, FCurveInterp interp = FCurveInterp::Cubic);
    // Removes key closest to `frame` within `tolerance`; returns true if removed.
    bool removeKey(float frame, float tolerance = 0.01f);
    // Moves the key at index `idx` to `frame` (value kept).
    bool moveKey(int idx, float newFrame);
    // Sets the value of the key at index `idx`.
    bool setValue(int idx, float value);
    bool setInterpolation(int idx, FCurveInterp interp);

    int nearestKey(float frame) const;

    // Evaluate value at `frame` (clamped extrapolation: hold first/last).
    float evaluate(float frame) const;

    // Keys sorted ascending by frame.
    void sort();

    QVariantList toVariant() const;
};

// A complete F-Curve: one channel per animated property.
struct FCurveData {
    QString objectName;
    QVector<FCurveChannel> channels;

    FCurveChannel* channel(const QString& name);
    const FCurveChannel* channel(const QString& name) const;
    FCurveChannel& ensureChannel(const QString& name);
    QStringList channelNames() const;
    void clear() { channels.clear(); }

    QVariantMap toVariant() const;
    static FCurveData fromVariant(const QVariantMap& m);
};

// Standard channel names for a scene object transform.
namespace FCurveChannels {
inline QStringList transformChannels() {
    return { "position.x", "position.y", "position.z",
             "rotation.x", "rotation.y", "rotation.z",
             "scale.x", "scale.y", "scale.z" };
}
inline int indexOf(const QString& name) { return transformChannels().indexOf(name); }
}

} // namespace ks
