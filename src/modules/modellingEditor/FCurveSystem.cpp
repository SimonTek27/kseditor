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
            return;
        }
    }
    FCurveKey k;
    k.frame = frame;
    k.value = value;
    k.interpolation = interp;
    keys.append(k);
    sort();
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
    return true;
}

bool FCurveChannel::setValue(int idx, float value)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].value = value;
    return true;
}

bool FCurveChannel::setInterpolation(int idx, FCurveInterp interp)
{
    if (idx < 0 || idx >= keys.size()) return false;
    keys[idx].interpolation = interp;
    return true;
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

    // Extrapolation: hold first/last value.
    if (frame <= keys.first().frame) return keys.first().value;
    if (frame >= keys.last().frame) return keys.last().value;

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

QVariantMap FCurveData::toVariant() const
{
    QVariantMap m;
    m["objectName"] = objectName;
    QVariantMap channelsMap;
    for (const auto& c : channels) {
        QVariantMap cm;
        cm["name"] = c.name;
        cm["keys"] = c.toVariant();
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
            ch.keys.append(k);
        }
        ch.sort();
        data.channels.append(ch);
    }
    return data;
}

} // namespace ks
