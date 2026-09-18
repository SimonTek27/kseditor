#include "FCurveSystem.h"

namespace ks {

// ============================================================================
// FCurveKey
// ============================================================================

QVariantMap FCurveKey::toVariant() const
{
    QVariantMap m;
    m["frame"] = frame;
    m["value"] = value;
    m["interpolation"] = fcurveInterpToString(interpolation);
    m["inTangent"] = inTangent;
    m["outTangent"] = outTangent;
    m["locked"] = locked;
    m["tangentMode"] = static_cast<int>(tangentMode);
    m["inHandleFrame"] = inHandleFrame;
    m["inHandleValue"] = inHandleValue;
    m["outHandleFrame"] = outHandleFrame;
    m["outHandleValue"] = outHandleValue;
    return m;
}

// ============================================================================
// FCurveChannel
// ============================================================================

void FCurveChannel::sort()
{
    std::stable_sort(keys.begin(), keys.end(),
        [](const FCurveKey& a, const FCurveKey& b) { return a.frame < b.frame; });
}

void FCurveChannel::setKey(float frame, float value, FCurveInterp interp)
{
    for (int i = 0; i < keys.size(); ++i) {
        if (qAbs(keys[i].frame - frame) < 0.001f) {
            keys[i].value = value;
            keys[i].interpolation = interp;
            keys[i].locked = false;
            sort();
            int newIdx = nearestKey(frame);
            if (newIdx >= 0 && keys[newIdx].tangentMode == FCurveTangentMode::Auto) {
                computeAutoTangent(newIdx);
            }
            return;
        }
    }
    FCurveKey k;
    k.frame = frame;
    k.value = value;
    k.interpolation = interp;
    keys.append(k);
    sort();
    int newIdx = nearestKey(frame);
    if (newIdx >= 0) {
        computeAutoTangent(newIdx);
        // Update neighbors if they are in Auto mode
        if (newIdx > 0 && keys[newIdx-1].tangentMode == FCurveTangentMode::Auto) {
            computeAutoTangent(newIdx - 1);
        }
        if (newIdx < keys.size() - 1 && keys[newIdx+1].tangentMode == FCurveTangentMode::Auto) {
            computeAutoTangent(newIdx + 1);
        }
    }
}

bool FCurveChannel::removeKey(float frame, float tolerance)
{
    for (int i = 0; i < keys.size(); ++i) {
        if (qAbs(keys[i].frame - frame) <= tolerance) {
            keys.removeAt(i);
            return true;
        }
    }
    return false;
}

bool FCurveChannel::moveKey(int idx, float newFrame)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].frame = newFrame;
    sort();
    // Recompute tangents for moved key and neighbors if in Auto mode
    int newIdx = nearestKey(newFrame);
    if (newIdx >= 0) {
        if (keys[newIdx].tangentMode == FCurveTangentMode::Auto || keys[newIdx].tangentMode == FCurveTangentMode::Clamped) {
            computeAutoTangent(newIdx);
        }
        if (newIdx > 0 && (keys[newIdx-1].tangentMode == FCurveTangentMode::Auto || keys[newIdx-1].tangentMode == FCurveTangentMode::Clamped)) {
            computeAutoTangent(newIdx - 1);
        }
        if (newIdx < keys.size() - 1 && (keys[newIdx+1].tangentMode == FCurveTangentMode::Auto || keys[newIdx+1].tangentMode == FCurveTangentMode::Clamped)) {
            computeAutoTangent(newIdx + 1);
        }
    }
    return true;
}

bool FCurveChannel::setValue(int idx, float value)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].value = value;
    // Recompute tangents for changed key and neighbors if in Auto mode
    if (keys[idx].tangentMode == FCurveTangentMode::Auto || keys[idx].tangentMode == FCurveTangentMode::Clamped) {
        computeAutoTangent(idx);
    }
    if (idx > 0 && (keys[idx-1].tangentMode == FCurveTangentMode::Auto || keys[idx-1].tangentMode == FCurveTangentMode::Clamped)) {
        computeAutoTangent(idx - 1);
    }
    if (idx < keys.size() - 1 && (keys[idx+1].tangentMode == FCurveTangentMode::Auto || keys[idx+1].tangentMode == FCurveTangentMode::Clamped)) {
        computeAutoTangent(idx + 1);
    }
    return true;
}

bool FCurveChannel::setInterpolation(int idx, FCurveInterp interp)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].interpolation = interp;
    return true;
}

bool FCurveChannel::setTangentMode(int idx, FCurveTangentMode mode)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].tangentMode = mode;
    if (mode == FCurveTangentMode::Auto || mode == FCurveTangentMode::Clamped) {
        computeAutoTangent(idx);
    }
    return true;
}

void FCurveChannel::computeAutoTangent(int idx)
{
    if (idx < 0 || idx >= keys.size()) return;
    FCurveKey& k = keys[idx];

    float prevSlope = 0.0f;
    float nextSlope = 0.0f;

    if (idx > 0) {
        float span = k.frame - keys[idx-1].frame;
        if (span > 1e-6f) {
            prevSlope = (k.value - keys[idx-1].value) / span;
        }
    }
    if (idx < keys.size() - 1) {
        float span = keys[idx+1].frame - k.frame;
        if (span > 1e-6f) {
            nextSlope = (keys[idx+1].value - k.value) / span;
        }
    }

    if (k.tangentMode == FCurveTangentMode::Clamped) {
        // Clamp tangent to not overshoot neighbor values
        float minVal = (idx > 0) ? qMin(keys[idx-1].value, k.value) : k.value;
        float maxVal = (idx > 0) ? qMax(keys[idx-1].value, k.value) : k.value;
        if (idx < keys.size() - 1) {
            minVal = qMin(minVal, keys[idx+1].value);
            maxVal = qMax(minVal, keys[idx+1].value);
        }
        float maxSlope = (maxVal - minVal) * 2.0f;
        prevSlope = qBound(-maxSlope, prevSlope, maxSlope);
        nextSlope = qBound(-maxSlope, nextSlope, maxSlope);
    }

    if (idx == 0) {
        k.inTangent = nextSlope;
        k.outTangent = nextSlope;
    } else if (idx == keys.size() - 1) {
        k.inTangent = prevSlope;
        k.outTangent = prevSlope;
    } else {
        k.inTangent = prevSlope;
        k.outTangent = nextSlope;
    }

    // Handle length in frames (proportional to span)
    float handleLen = 20.0f;
    if (idx > 0 && idx < keys.size() - 1) {
        handleLen = (keys[idx+1].frame - keys[idx-1].frame) * 0.25f;
    } else if (idx > 0) {
        handleLen = (k.frame - keys[idx-1].frame) * 0.25f;
    } else if (idx < keys.size() - 1) {
        handleLen = (keys[idx+1].frame - k.frame) * 0.25f;
    }
    handleLen = qMax(handleLen, 5.0f);

    updateHandlesFromTangent(idx);
}

void FCurveChannel::updateHandlesFromTangent(int idx)
{
    if (idx < 0 || idx >= keys.size()) return;
    FCurveKey& k = keys[idx];

    float handleLen = 20.0f;
    if (idx > 0 && idx < keys.size() - 1) {
        handleLen = (keys[idx+1].frame - keys[idx-1].frame) * 0.25f;
    } else if (idx > 0) {
        handleLen = (k.frame - keys[idx-1].frame) * 0.25f;
    } else if (idx < keys.size() - 1) {
        handleLen = (keys[idx+1].frame - k.frame) * 0.25f;
    }
    handleLen = qMax(handleLen, 5.0f);

    k.inHandleFrame = -handleLen;
    k.inHandleValue = -k.inTangent * handleLen;
    k.outHandleFrame = handleLen;
    k.outHandleValue = k.outTangent * handleLen;
}

void FCurveChannel::updateTangentFromHandles(int idx)
{
    if (idx < 0 || idx >= keys.size()) return;
    FCurveKey& k = keys[idx];

    if (qAbs(k.inHandleFrame) > 1e-6f) {
        k.inTangent = k.inHandleValue / k.inHandleFrame;
    }
    if (qAbs(k.outHandleFrame) > 1e-6f) {
        k.outTangent = k.outHandleValue / k.outHandleFrame;
    }

    // If aligned mode, sync the other handle
    if (k.tangentMode == FCurveTangentMode::Aligned) {
        float avgSlope = (k.inTangent + k.outTangent) * 0.5f;
        k.inTangent = avgSlope;
        k.outTangent = avgSlope;
        updateHandlesFromTangent(idx);
    }
}

int FCurveChannel::nearestKey(float frame) const
{
    if (keys.isEmpty()) return -1;
    int best = 0;
    float bestDist = qAbs(keys[0].frame - frame);
    for (int i = 1; i < keys.size(); ++i) {
        float d = qAbs(keys[i].frame - frame);
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

float FCurveChannel::evaluate(float frame) const
{
    if (keys.isEmpty()) return 0.0f;
    if (keys.size() == 1) return keys[0].value;

    // Pre-extrapolation
    if (frame < keys.first().frame) {
        switch (preExtrapolation) {
        case FCurveExtrapolation::Linear: {
            if (keys.size() < 2) return keys[0].value;
            float span = keys[1].frame - keys[0].frame;
            if (span <= 1e-6f) return keys[0].value;
            float slope = (keys[1].value - keys[0].value) / span;
            return keys[0].value + slope * (frame - keys[0].frame);
        }
        case FCurveExtrapolation::Cycle: {
            float range = keys.last().frame - keys[0].frame;
            if (range <= 1e-6f) return keys[0].value;
            float offset = fmodf(keys[0].frame - frame, range);
            float f = keys.last().frame - offset;
            return evaluate(f);
        }
        case FCurveExtrapolation::CycleOffset: {
            float range = keys.last().frame - keys[0].frame;
            if (range <= 1e-6f) return keys[0].value;
            float offset = fmodf(keys[0].frame - frame, range);
            float f = keys.last().frame - offset;
            return evaluate(f) - (keys.last().value - keys[0].value);
        }
        case FCurveExtrapolation::Constant:
        default:
            return keys[0].value;
        }
    }

    // Post-extrapolation
    if (frame > keys.last().frame) {
        switch (postExtrapolation) {
        case FCurveExtrapolation::Linear: {
            if (keys.size() < 2) return keys.last().value;
            float span = keys.last().frame - keys[keys.size()-2].frame;
            if (span <= 1e-6f) return keys.last().value;
            float slope = (keys.last().value - keys[keys.size()-2].value) / span;
            return keys.last().value + slope * (frame - keys.last().frame);
        }
        case FCurveExtrapolation::Cycle: {
            float range = keys.last().frame - keys[0].frame;
            if (range <= 1e-6f) return keys.last().value;
            float offset = fmodf(frame - keys[0].frame, range);
            float f = keys[0].frame + offset;
            return evaluate(f);
        }
        case FCurveExtrapolation::CycleOffset: {
            float range = keys.last().frame - keys[0].frame;
            if (range <= 1e-6f) return keys.last().value;
            float offset = fmodf(frame - keys[0].frame, range);
            float f = keys[0].frame + offset;
            return evaluate(f) + (keys.last().value - keys[0].value);
        }
        case FCurveExtrapolation::Constant:
        default:
            return keys.last().value;
        }
    }

    // Find segment [i, i+1].
    int i = 0;
    for (int k = 0; k < keys.size() - 1; ++k) {
        if (frame >= keys[k].frame && frame < keys[k + 1].frame) { i = k; break; }
    }
    const FCurveKey& a = keys[i];
    const FCurveKey& b = keys[i + 1];
    float span = b.frame - a.frame;
    if (span <= 1e-6f) return a.value;
    float t = (frame - a.frame) / span;
    t = qBound(0.0f, t, 1.0f);

    switch (a.interpolation) {
    case FCurveInterp::Step:
        return a.value;
    case FCurveInterp::Cubic: {
        // Hermite interpolation using tangents (slopes per frame).
        float tIn = a.outTangent * span;
        float tOut = b.inTangent * span;
        float t2 = t * t;
        float t3 = t2 * t;
        float h00 = 2*t3 - 3*t2 + 1;
        float h10 = t3 - 2*t2 + t;
        float h01 = -2*t3 + 3*t2;
        float h11 = t3 - t2;
        return h00*a.value + h10*tIn + h01*b.value + h11*tOut;
    }
    case FCurveInterp::EaseIn:
        return a.value + (b.value - a.value) * (t * t);
    case FCurveInterp::EaseOut:
        return a.value + (b.value - a.value) * (1.0f - (1.0f - t) * (1.0f - t));
    case FCurveInterp::EaseInOut:
        return a.value + (b.value - a.value) * (t < 0.5f ? 2*t*t : 1.0f - 2*(1-t)*(1-t));
    case FCurveInterp::Linear:
    default:
        return a.value + (b.value - a.value) * t;
    }
}

QVariantList FCurveChannel::toVariant() const
{
    QVariantList list;
    for (const auto& k : keys)
        list.append(k.toVariant());
    return list;
}

// ============================================================================
// FCurveData
// ============================================================================

FCurveChannel* FCurveData::channel(const QString& name)
{
    for (auto& c : channels)
        if (c.name == name) return &c;
    return nullptr;
}

const FCurveChannel* FCurveData::channel(const QString& name) const
{
    for (const auto& c : channels)
        if (c.name == name) return &c;
    return nullptr;
}

FCurveChannel& FCurveData::ensureChannel(const QString& name)
{
    if (FCurveChannel* c = channel(name)) return *c;
    FCurveChannel c;
    c.name = name;
    channels.append(c);
    return channels.last();
}

QStringList FCurveData::channelNames() const
{
    QStringList names;
    for (const auto& c : channels)
        names.append(c.name);
    return names;
}

void FCurveData::pushUndoState()
{
    undoStack.append(channels);
    redoStack.clear();
    // Limit undo stack size
    if (undoStack.size() > 50) undoStack.removeFirst();
}

void FCurveData::undo()
{
    if (undoStack.isEmpty()) return;
    redoStack.append(channels);
    channels = undoStack.takeLast();
}

void FCurveData::redo()
{
    if (redoStack.isEmpty()) return;
    undoStack.append(channels);
    channels = redoStack.takeLast();
}

QVariantMap FCurveData::toVariant() const
{
    QVariantMap m;
    m["objectName"] = objectName;
    QVariantMap channelsMap;
    for (const auto& c : channels) {
        QVariantMap cm;
        cm["name"] = c.name;
        cm["keys"] = c.toVariant();
        cm["preExtrapolation"] = fcurveExtrapolationToString(c.preExtrapolation);
        cm["postExtrapolation"] = fcurveExtrapolationToString(c.postExtrapolation);
        channelsMap[c.name] = cm;
    }
    m["channels"] = channelsMap;
    return m;
}

FCurveData FCurveData::fromVariant(const QVariantMap& m)
{
    FCurveData data;
    data.objectName = m.value("objectName").toString();
    QVariantMap channelsMap = m.value("channels").toMap();
    for (auto it = channelsMap.constBegin(); it != channelsMap.constEnd(); ++it) {
        FCurveChannel ch;
        ch.name = it.key();
        QVariantMap cm = it.value().toMap();
        ch.preExtrapolation = fcurveExtrapolationFromString(cm.value("preExtrapolation").toString());
        ch.postExtrapolation = fcurveExtrapolationFromString(cm.value("postExtrapolation").toString());
        QVariantList keys = cm.value("keys").toList();
        for (const auto& kv : keys) {
            QVariantMap km = kv.toMap();
            FCurveKey k;
            k.frame = km.value("frame").toFloat();
            k.value = km.value("value").toFloat();
            k.interpolation = fcurveInterpFromString(km.value("interpolation").toString());
            k.inTangent = km.value("inTangent").toFloat();
            k.outTangent = km.value("outTangent").toFloat();
            k.locked = km.value("locked").toBool();
            k.tangentMode = static_cast<FCurveTangentMode>(km.value("tangentMode", 0).toInt());
            k.inHandleFrame = km.value("inHandleFrame", 0.0f).toFloat();
            k.inHandleValue = km.value("inHandleValue", 0.0f).toFloat();
            k.outHandleFrame = km.value("outHandleFrame", 0.0f).toFloat();
            k.outHandleValue = km.value("outHandleValue", 0.0f).toFloat();
            ch.keys.append(k);
        }
        ch.sort();
        data.channels.append(ch);
    }
    return data;
}

} // namespace ks
