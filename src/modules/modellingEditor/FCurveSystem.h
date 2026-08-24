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

// Extrapolation mode for values outside keyframe range.
enum class FCurveExtrapolation {
    Constant,   // hold first/last value
    Linear,     // extend with slope of nearest segment
    Cycle,      // repeat keyframe range
    CycleOffset // repeat with offset from last value
};

inline QString fcurveExtrapolationToString(FCurveExtrapolation e) {
    switch (e) {
    case FCurveExtrapolation::Constant:    return "Constant";
    case FCurveExtrapolation::Linear:      return "Linear";
    case FCurveExtrapolation::Cycle:       return "Cycle";
    case FCurveExtrapolation::CycleOffset: return "CycleOffset";
    }
    return "Constant";
}

inline FCurveExtrapolation fcurveExtrapolationFromString(const QString& s) {
    if (s == "Linear")      return FCurveExtrapolation::Linear;
    if (s == "Cycle")       return FCurveExtrapolation::Cycle;
    if (s == "CycleOffset") return FCurveExtrapolation::CycleOffset;
    return FCurveExtrapolation::Constant;
}

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

// Tangent handle mode for a keyframe.
enum class FCurveTangentMode {
    Auto,     // auto-computed from neighbors
    Free,     // independent in/out handles
    Aligned,  // in/out handles are collinear (direction locked)
    Broken,   // in/out handles independent (direction + length)
    Clamped,  // auto but clamped to not overshoot
    Vector    // tangent points toward neighbor key
};

// One keyframe: frame index, value, interpolation mode, optional tangents.
struct FCurveKey {
    float frame = 0.0f;
    float value = 0.0f;
    FCurveInterp interpolation = FCurveInterp::Cubic;
    float inTangent = 0.0f;   // slope (value per frame) entering the key
    float outTangent = 0.0f;  // slope leaving the key
    bool locked = false;
    FCurveTangentMode tangentMode = FCurveTangentMode::Auto;
    // Tangent handle positions in graph space (frame, value) relative to key.
    // Used for visual editing in the graph editor.
    float inHandleFrame = 0.0f;
    float inHandleValue = 0.0f;
    float outHandleFrame = 0.0f;
    float outHandleValue = 0.0f;

    QVariantMap toVariant() const;
};

// A single animated channel (e.g. position.x). Keys sorted by frame.
struct FCurveChannel {
    QString name;
    QVector<FCurveKey> keys;
    FCurveExtrapolation preExtrapolation = FCurveExtrapolation::Constant;
    FCurveExtrapolation postExtrapolation = FCurveExtrapolation::Constant;

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
    bool setTangentMode(int idx, FCurveTangentMode mode);

    // Compute auto-tangent for key at index idx from neighboring keys.
    void computeAutoTangent(int idx);
    // Update handle positions from tangent slopes.
    void updateHandlesFromTangent(int idx);
    // Update tangent slopes from handle positions.
    void updateTangentFromHandles(int idx);

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
    void clear() { channels.clear(); undoStack.clear(); redoStack.clear(); }

    // Undo/redo support
    void pushUndoState();
    bool canUndo() const { return !undoStack.isEmpty(); }
    bool canRedo() const { return !redoStack.isEmpty(); }
    void undo();
    void redo();

    QVariantMap toVariant() const;
    static FCurveData fromVariant(const QVariantMap& m);

private:
    QVector<QVector<FCurveChannel>> undoStack;
    QVector<QVector<FCurveChannel>> redoStack;
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
