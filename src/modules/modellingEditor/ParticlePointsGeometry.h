#pragma once

#include <QQuick3DGeometry>
#include <QVariantList>
#include <QVector3D>
#include <QVector4D>

namespace ks {

// Geometry that renders particles as GL points (PrimitiveType::Points).
class QmlParticlePointsGeometry : public QQuick3DGeometry {
    Q_OBJECT
    Q_PROPERTY(QVariantList particlePositions READ particlePositions WRITE setParticlePositions NOTIFY particleDataChanged)
    Q_PROPERTY(QVariantList particleColors READ particleColors WRITE setParticleColors NOTIFY particleDataChanged)
    Q_PROPERTY(QVariantList particleSizes READ particleSizes WRITE setParticleSizes NOTIFY particleDataChanged)

public:
    explicit QmlParticlePointsGeometry(QQuick3DObject* parent = nullptr);

    QVariantList particlePositions() const { return m_positions; }
    QVariantList particleColors() const { return m_colors; }
    QVariantList particleSizes() const { return m_sizes; }

    void setParticlePositions(const QVariantList& v);
    void setParticleColors(const QVariantList& v);
    void setParticleSizes(const QVariantList& v);

    Q_INVOKABLE void rebuild();

signals:
    void particleDataChanged();

private:
    QVariantList m_positions;
    QVariantList m_colors;
    QVariantList m_sizes;
};

} // namespace ks