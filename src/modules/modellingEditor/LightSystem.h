#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QVariant>

namespace ks {

// Photometric light definition (3ds Max Light Lister style), keyed by the id of
// a SceneObject of type Light. `type` mirrors the light types exposed to QML:
// 0 = Directional, 1 = Point, 2 = Spot, 3 = Area (area uses the directional
// sampling path with a wide soft cone).
struct LightDef {
    int objectId = -1;
    QString name;
    int type = 0;
    QColor color = QColor(255, 244, 224);
    float intensity = 1.0f;
    bool enabled = true;
    float range = 30.0f;             // point/spot decay distance (world units)
    float spotAngleDeg = 45.0f;      // spot outer cone
    float spotPenumbraDeg = 10.0f;   // inner/outer cone falloff band
    QString iesProfile;              // path of the loaded .ies file ("" = none)
    float iesIntensity = 1.0f;       // extra multiplier applied to the IES curve

    // Normalized vertical candela curve: index 0..90 == 0..90 deg from the light
    // forward axis, value 0..1 (peak = 1.0). Empty when no IES profile is loaded.
    QVector<float> iesCurve;

    QVariant toVariant() const;
    void fromVariant(const QVariant& v);

    // Interpolated IES multiplier for a direction that forms `angleDeg` degrees
    // with the light forward axis (1.0 when no profile is loaded).
    float iesMultiplier(float angleDeg) const;
};

class LightSystem
{
public:
    LightSystem() = default;

    bool add(const LightDef& def);
    bool remove(int objectId);
    bool has(int objectId) const { return find(objectId) != nullptr; }
    LightDef* find(int objectId);
    const LightDef* find(int objectId) const;
    QVector<LightDef> lights() const { return m_lights; }
    QVector<int> lightObjectIds() const;
    bool hasAny() const { return !m_lights.isEmpty(); }
    void clearAll() { m_lights.clear(); }

    // Parses an IESNA LM-63 .ies file into a normalized vertical curve.
    // Returns false when the file is not a parseable IES profile (TILT=INCLUDE
    // and empty/invalid files are rejected).
    static bool parseIESFile(const QString& path, QVector<float>& outCurve);

private:
    QVector<LightDef> m_lights; // insertion order (matches scene object creation)
};

} // namespace ks
