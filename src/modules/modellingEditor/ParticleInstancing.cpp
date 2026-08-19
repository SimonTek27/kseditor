#include "ParticleInstancing.h"

#include <cstring>

namespace ks {

QmlParticleInstancing::QmlParticleInstancing(QQuick3DObject* parent)
    : QQuick3DInstancing(parent)
{
}

void QmlParticleInstancing::setParticlePositions(const QVariantList& v)
{
    if (m_positions == v) return;
    m_positions = v;
    updateEntries();
    emit particleDataChanged();
}

void QmlParticleInstancing::setParticleColors(const QVariantList& v)
{
    if (m_colors == v) return;
    m_colors = v;
    updateEntries();
    emit particleDataChanged();
}

void QmlParticleInstancing::setParticleSizes(const QVariantList& v)
{
    if (m_sizes == v) return;
    m_sizes = v;
    updateEntries();
    emit particleDataChanged();
}

void QmlParticleInstancing::rebuild()
{
    updateEntries();
}

void QmlParticleInstancing::updateEntries()
{
    const int count = m_positions.size();
    m_entries.clear();
    m_entries.reserve(count);
    for (int i = 0; i < count; ++i) {
        QVariantList p = m_positions[i].toList();
        float x = p.size() > 0 ? p[0].toFloat() : 0;
        float y = p.size() > 1 ? p[1].toFloat() : 0;
        float z = p.size() > 2 ? p[2].toFloat() : 0;

        QVariantList c = m_colors.size() > i ? m_colors[i].toList() : QVariantList();
        QColor col(255, 255, 255, 255);
        if (c.size() >= 4)
            col = QColor(qRound(c[0].toFloat() * 255), qRound(c[1].toFloat() * 255),
                         qRound(c[2].toFloat() * 255), qRound(c[3].toFloat() * 255));

        float size = m_sizes.size() > i ? m_sizes[i].toFloat() : 0.05f;
        if (size <= 0.0f) size = 0.05f;

        m_entries.append(calculateTableEntry(QVector3D(x, y, z),
                                             QVector3D(size, size, size),
                                             QVector3D(),
                                             col));
    }
    markDirty();
}

QByteArray QmlParticleInstancing::getInstanceBuffer(int* instanceCount)
{
    *instanceCount = m_entries.size();
    QByteArray buffer;
    buffer.resize(int(m_entries.size()) * int(sizeof(QQuick3DInstancing::InstanceTableEntry)));
    if (!m_entries.isEmpty())
        memcpy(buffer.data(), m_entries.constData(),
               m_entries.size() * int(sizeof(QQuick3DInstancing::InstanceTableEntry)));
    return buffer;
}

} // namespace ks
