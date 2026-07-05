#pragma once

#define _USE_MATH_DEFINES
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <vector>
#include <cmath>
#include <cstdint>
#include <QString>
#include <QImage>
#include <QVector>
#include <QVector3D>
#include <QObject>
#include <QMap>
#include <QVariant>
#include "../../core/Math/MathCore.h"

namespace ks {
namespace modeler {

// Parameter types for parametric modeling (FreeCAD-inspired)
enum ParamType {
    ParamFloat,
    ParamInt,
    ParamBool,
    ParamEnum,
    ParamVector3D,
    ParamColor
};

struct ParamValue {
    ParamType type;
    QVariant value;
    QVariant minValue;
    QVariant maxValue;
    QStringList enumValues; // For enum type
    QString description;

    float toFloat() const { return value.toFloat(); }
    int toInt() const { return value.toInt(); }
    bool toBool() const { return value.toBool(); }
    QString toString() const { return value.toString(); }
};

// Parametric object base class
class ParametricObject : public QObject {
    Q_OBJECT

public:
    explicit ParametricObject(const QString& name, QObject* parent = nullptr);
    virtual ~ParametricObject() = default;

    QString name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }

    // Parameters
    void addParameter(const QString& key, ParamType type, const QVariant& defaultValue,
                    const QString& description = QString());
    void setParameter(const QString& key, const QVariant& value);
    ParamValue getParameter(const QString& key) const;
    QMap<QString, ParamValue> parameters() const { return m_params; }

    // Rebuild the object based on parameters
    virtual void rebuild() = 0;

    // Get mesh data (to be implemented by subclasses)
    virtual QByteArray getMeshData() const = 0;

 signals:
    void parameterChanged(const QString& key);
    void meshRegenerated();

protected:
    QString m_name;
    QMap<QString, ParamValue> m_params;
};

// Wheel/Tire generator (AC-specific, FreeCAD-inspired)
class ParametricWheel : public ParametricObject {
    Q_OBJECT

public:
    explicit ParametricWheel(QObject* parent = nullptr);

    void rebuild() override;
    QByteArray getMeshData() const override;

    // Wheel-specific parameters
    float rimDiameter() const { return m_params["rimDiameter"].toFloat(); }
    float tireWidth() const { return m_params["tireWidth"].toFloat(); }
    int numBolts() const { return m_params["numBolts"].toInt(); }

private:
    void generateWheelMesh();
    QByteArray m_meshData;
};

// Brake disc generator
class ParametricBrakeDisc : public ParametricObject {
    Q_OBJECT

public:
    explicit ParametricBrakeDisc(QObject* parent = nullptr);

    void rebuild() override;
    QByteArray getMeshData() const override;

private:
    void generateBrakeDiscMesh();
    QByteArray m_meshData;
};

// Suspension component (A-arm, etc.)
class ParametricSuspensionArm : public ParametricObject {
    Q_OBJECT

public:
    explicit ParametricSuspensionArm(QObject* parent = nullptr);

    void rebuild() override;
    QByteArray getMeshData() const override;

private:
    void generateArmMesh();
    QByteArray m_meshData;
};

// Parametric primitive factory
class ParametricFactory : public QObject {
    Q_OBJECT

public:
    static ParametricFactory* instance();

    ParametricObject* createWheel(const QString& name);
    ParametricObject* createBrakeDisc(const QString& name);
    ParametricObject* createSuspensionArm(const QString& name);

    QVector<QString> availableTypes() const;

private:
    ParametricFactory(QObject* parent = nullptr);
    ~ParametricFactory();

    static ParametricFactory* s_instance;
};

} // namespace modeler

// ============================================================================
// Mesh Types (used by QmlBridge and other modules)
// ============================================================================

struct VertexUV {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
};

struct Mesh {
    std::vector<VertexUV> vertices;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> materialIds;
};

inline float noise2D(float x, float y, int seed)
{
    int n = static_cast<int>(x + y * 57.0f + seed);
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

} // namespace ks