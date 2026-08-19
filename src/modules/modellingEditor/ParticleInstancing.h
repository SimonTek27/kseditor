#pragma once

#include <QQuick3DInstancing>
#include <QVariantList>
#include <QVector3D>

namespace ks {

// Instance table that renders particles as instanced spheres (or any source
// model). One instance per particle: position + uniform scale (size) + color.
class QmlParticleInstancing : public QQuick3DInstancing {
    Q_OBJECT
    Q_PROPERTY(QVariantList particlePositions READ particlePositions WRITE setParticlePositions NOTIFY particleDataChanged)
    Q_PROPERTY(QVariantList particleColors READ particleColors WRITE setParticleColors NOTIFY particleDataChanged)
    Q_PROPERTY(QVariantList particleSizes READ particleSizes WRITE setParticleSizes NOTIFY particleDataChanged)

public:
    explicit QmlParticleInstancing(QQuick3DObject* parent = nullptr);

    QVariantList particlePositions() const { return m_positions; }
    QVariantList particleColors() const { return m_colors; }
    QVariantList particleSizes() const { return m_sizes; }

    void setParticlePositions(const QVariantList& v);
    void setParticleColors(const QVariantList& v);
    void setParticleSizes(const QVariantList& v);

    Q_INVOKABLE void rebuild();

signals:
    void particleDataChanged();

protected:
    QByteArray getInstanceBuffer(int* instanceCount) override;

private:
    void updateEntries();
    QVariantList m_positions;
    QVariantList m_colors;
    QVariantList m_sizes;
    QVector<QQuick3DInstancing::InstanceTableEntry> m_entries;
};

} // namespace ks
