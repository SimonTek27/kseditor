#include "ParticlePointsGeometry.h"
#include <cstring>

namespace ks {

QmlParticlePointsGeometry::QmlParticlePointsGeometry(QQuick3DObject* parent)
    : QQuick3DGeometry(parent)
{
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Points);
}

void QmlParticlePointsGeometry::setParticlePositions(const QVariantList& v)
{
    if (m_positions == v) return;
    m_positions = v;
    emit particleDataChanged();
    rebuild();
}

void QmlParticlePointsGeometry::setParticleColors(const QVariantList& v)
{
    if (m_colors == v) return;
    m_colors = v;
    emit particleDataChanged();
    rebuild();
}

void QmlParticlePointsGeometry::setParticleSizes(const QVariantList& v)
{
    if (m_sizes == v) return;
    m_sizes = v;
    emit particleDataChanged();
    rebuild();
}

void QmlParticlePointsGeometry::rebuild()
{
    clear();
    const int count = m_positions.size();
    if (count == 0) return;

    // Stride: position(3) + color(4) = 7 floats = 28 bytes
    const int stride = 28;
    QByteArray vertexData;
    vertexData.resize(count * stride);
    float* vptr = reinterpret_cast<float*>(vertexData.data());

    for (int i = 0; i < count; ++i) {
        QVariantList p = m_positions[i].toList();
        float x = p.size() > 0 ? p[0].toFloat() : 0;
        float y = p.size() > 1 ? p[1].toFloat() : 0;
        float z = p.size() > 2 ? p[2].toFloat() : 0;
        vptr[0] = x;
        vptr[1] = y;
        vptr[2] = z;

        QVariantList c = m_colors.size() > i ? m_colors[i].toList() : QVariantList();
        float cr = c.size() > 0 ? c[0].toFloat() : 1.0f;
        float cg = c.size() > 1 ? c[1].toFloat() : 1.0f;
        float cb = c.size() > 2 ? c[2].toFloat() : 1.0f;
        float ca = c.size() > 3 ? c[3].toFloat() : 1.0f;
        vptr[3] = cr;
        vptr[4] = cg;
        vptr[5] = cb;
        vptr[6] = ca;

        vptr += 7;
    }

    setVertexData(vertexData);
    setStride(stride);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::ColorSemantic, 12, QQuick3DGeometry::Attribute::F32Type);

    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Points);
}

} // namespace ks