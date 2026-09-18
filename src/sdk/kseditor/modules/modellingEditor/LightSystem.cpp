#include "LightSystem.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

namespace ks {

namespace {
// Barycentric/linear interpolation helper for the IES curve (angle in degrees).
float lerp3(const QVector<float>& v, float x)
{
    if (v.isEmpty())
        return 1.0f;
    if (x <= 0.0f) return v.first();
    if (x >= 90.0f) return v.last();
    const float f = x * (v.size() - 1) / 90.0f;
    const int i = std::min(int(f), int(v.size()) - 2);
    const float t = f - i;
    return v[i] * (1.0f - t) + v[i + 1] * t;
}
} // namespace

QVariant LightDef::toVariant() const
{
    QVariantMap m;
    m["name"] = name;
    m["type"] = type;
    m["color"] = color.rgb();
    m["intensity"] = double(intensity);
    m["enabled"] = enabled;
    m["range"] = double(range);
    m["spotAngleDeg"] = double(spotAngleDeg);
    m["spotPenumbraDeg"] = double(spotPenumbraDeg);
    m["iesProfile"] = iesProfile;
    m["iesIntensity"] = double(iesIntensity);
    // The curve is regenerated from the profile path on load; not persisted.
    return m;
}

void LightDef::fromVariant(const QVariant& v)
{
    const QVariantMap m = v.toMap();
    name = m.value("name").toString();
    type = m.value("type", 0).toInt();
    color = QColor::fromRgb(m.value("color", QColor(255, 244, 224).rgb()).toUInt());
    intensity = float(m.value("intensity", 1.0).toDouble());
    enabled = m.value("enabled", true).toBool();
    range = float(m.value("range", 30.0).toDouble());
    spotAngleDeg = float(m.value("spotAngleDeg", 45.0).toDouble());
    spotPenumbraDeg = float(m.value("spotPenumbraDeg", 10.0).toDouble());
    iesProfile = m.value("iesProfile").toString();
    iesIntensity = float(m.value("iesIntensity", 1.0).toDouble());
}

float LightDef::iesMultiplier(float angleDeg) const
{
    return lerp3(iesCurve, qBound(0.0f, angleDeg, 90.0f)) * iesIntensity;
}

bool LightSystem::add(const LightDef& def)
{
    if (find(def.objectId))
        return false;
    m_lights.append(def);
    return true;
}

bool LightSystem::remove(int objectId)
{
    for (int i = 0; i < m_lights.size(); ++i) {
        if (m_lights[i].objectId == objectId) {
            m_lights.remove(i);
            return true;
        }
    }
    return false;
}

LightDef* LightSystem::find(int objectId)
{
    for (LightDef& l : m_lights)
        if (l.objectId == objectId) return &l;
    return nullptr;
}

const LightDef* LightSystem::find(int objectId) const
{
    for (const LightDef& l : m_lights)
        if (l.objectId == objectId) return &l;
    return nullptr;
}

QVector<int> LightSystem::lightObjectIds() const
{
    QVector<int> ids;
    ids.reserve(m_lights.size());
    for (const LightDef& l : m_lights)
        ids.append(l.objectId);
    return ids;
}

bool LightSystem::parseIESFile(const QString& path, QVector<float>& outCurve)
{
    outCurve.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    // Collect the numeric data section: every line whose first token is a number.
    // Header lines (IESNA:..., [TEST], TILT=...) are skipped.
    QVector<double> nums;
    bool tiltInclude = false;
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty()) continue;
        if (line.startsWith(QStringLiteral("TILT"))) {
            if (line.contains(QStringLiteral("INCLUDE"), Qt::CaseInsensitive))
                tiltInclude = true;
            continue;
        }
        bool allNumeric = true;
        const QStringList toks = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        for (const QString& t : toks) {
            bool ok = false;
            t.toDouble(&ok);
            if (!ok) { allNumeric = false; break; }
        }
        if (!allNumeric || toks.isEmpty()) continue;
        for (const QString& t : toks)
            nums.append(t.toDouble());
    }
    if (nums.size() < 5)
        return false;

    int pos = 0;
    const int nLamps = int(nums[pos++]);
    // lumens = nums[pos++]; candelaMult = nums[pos++];
    pos += 2;
    const int nv = int(nums[pos++]);
    const int nh = int(nums[pos++]);
    if (nv < 1 || nh < 1 || nums.size() < pos + nv + nh + nv * nh)
        return false;

    QVector<double> vert(nv);
    for (int i = 0; i < nv; ++i) vert[i] = nums[pos++];
    QVector<double> horiz(nh);
    for (int i = 0; i < nh; ++i) horiz[i] = nums[pos++];

    // Candela block: nh groups of nv values (one group per horizontal angle).
    QVector<double> candela(nv * nh);
    for (int i = 0; i < nv * nh; ++i)
        candela[i] = nums[pos++];

    // If TILT=INCLUDE, read tilt adjustment table and apply it to candela values.
    // Tilt table: nh groups of nv values (adjustments added to candela).
    if (tiltInclude && pos + nv * nh <= nums.size()) {
        for (int i = 0; i < nv * nh; ++i) {
            candela[i] += nums[pos++];
        }
    }

    // Rebuild a normalized vertical curve: for each 0..90 deg step, average the
    // candela across all horizontal angles at the nearest vertical angle.
    double maxV = 1e-9;
    QVector<float> raw(91, 0.0f);
    for (int d = 0; d <= 90; ++d) {
        double sum = 0.0;
        for (int h = 0; h < nh; ++h) {
            const int idx = h * nv;
            // Pick the vertical sample nearest `d` degrees.
            int best = 0;
            double bestDist = 1e18;
            for (int vi = 0; vi < nv; ++vi) {
                double a = vert[vi];
                if (a > 90.0) a -= 360.0;
                if (a < 0.0) a = -a;
                const double dist = std::abs(a - d);
                if (dist < bestDist) { bestDist = dist; best = vi; }
            }
            sum += candela[idx + best];
        }
        raw[d] = float(sum / nh);
        maxV = std::max(maxV, double(raw[d]));
    }
    if (maxV <= 1e-9)
        return false;
    outCurve.reserve(91);
    for (int d = 0; d <= 90; ++d)
        outCurve.append(raw[d] / float(maxV));
    return true;
}

} // namespace ks
