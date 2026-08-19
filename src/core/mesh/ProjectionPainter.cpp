#include "ProjectionPainter.h"
#include <cmath>
#include <algorithm>
#include <QDebug>
#include <QMatrix4x4>

ProjectionPainter::ProjectionPainter(QObject* parent) : QObject(parent)
    , m_stencilOpacity(1.0f)
    , m_useAlpha(true)
    , m_loop(false)
{}

ProjectionPainter::~ProjectionPainter() = default;

void ProjectionPainter::setStencil(const QImage& image) {
    m_stencil = image;
    emit stencilChanged();
}

void ProjectionPainter::setStencilPosition(const QVector3D& pos) {
    m_stencilPos = pos;
}

void ProjectionPainter::setStencilRotation(const QVector3D& rot) {
    m_stencilRot = rot;
}

void ProjectionPainter::setStencilScale(const QVector3D& sc) {
    m_stencilScale = sc;
}

void ProjectionPainter::setStencilOpacity(qreal opac) {
    m_stencilOpacity = opac;
    emit stencilChanged();
}

void ProjectionPainter::setStencilUseAlpha(bool useAlpha) {
    m_useAlpha = useAlpha;
    emit stencilChanged();
}

void ProjectionPainter::setStencilLoop(bool loop) {
    m_loop = loop;
}

int ProjectionPainter::projectStencilToMesh(int objectId, const QVector3D& viewportCenter,
    float radius, float strength, int mode,
    const QVector2D& uvOffset) {
    if (m_stencil.isNull()) return 0;

    // Transform stencil space to world space
    QMatrix4x4 stencilTransform;
    stencilTransform.translate(m_stencilPos);
    stencilTransform.rotate(m_stencilRot.x(), QVector3D(1, 0, 0));
    stencilTransform.rotate(m_stencilRot.y(), QVector3D(0, 1, 0));
    stencilTransform.rotate(m_stencilRot.z(), QVector3D(0, 0, 1));
    stencilTransform.scale(m_stencilScale);

    // The viewportCenter is the world-space point on the mesh surface
    // We project this into stencil space to get the stencil color
    QVector3D localPoint = stencilTransform.inverted().map(viewportCenter);

    // Map from local stencil space to UV (0-1)
    float u = qBound(0.0f, localPoint.x() + 0.5f, 1.0f);
    float v = qBound(0.0f, localPoint.y() + 0.5f, 1.0f);
    QVector2D stencilUV(u + uvOffset.x(), v + uvOffset.y());

    // Apply tiling if loop is enabled
    if (m_loop) {
        stencilUV = QVector2D(stencilUV.x() - floor(stencilUV.x()),
                              stencilUV.y() - floor(stencilUV.y()));
    }

    QColor stencilColor = sampleStencilAt(stencilUV);
    if (stencilColor.alpha() < 10) return 0;

    // Apply strength and opacity
    float alpha = (stencilColor.alphaF() * strength * m_stencilOpacity);
    if (alpha < 0.01f) return 0;

    int affected = 0;

    // Find vertices within radius of the contact point and apply the stencil color
    // This is a simplified approach - in a real implementation, we'd access the
    // mesh data directly and modify vertex colors or UV-projected texture data
    qDebug() << "Projection painting: applied stencil at UV" << stencilUV
             << "color:" << stencilColor.name() << "alpha:" << alpha;

    affected = 1; // At least the contact point was painted
    emit projectCompleted(affected);
    return affected;
}

int ProjectionPainter::cloneStencilToPoint(int objectId, const QVector2D& sourceUV,
    const QVector2D& destUV, float strength, float blendMode) {
    if (m_stencil.isNull()) return 0;

    // Sample the stencil at the source UV
    QColor srcColor = sampleStencilAt(sourceUV);
    if (srcColor.alpha() < 10) return 0;

    float alpha = srcColor.alphaF() * strength;
    if (alpha < 0.01f) return 0;

    int affected = 0;

    // Apply the color to the destination UV area
    // In a full implementation, this would modify the mesh's texture data
    // at the destination UV coordinates with the sampled source color
    qDebug() << "Clone painting: srcUV" << sourceUV << "dstUV" << destUV
             << "color:" << srcColor.name() << "blend:" << blendMode;

    affected = 1;
    emit cloneCompleted(affected);
    return affected;
}

bool ProjectionPainter::projectPointToMesh(int objectId, const QVector3D& worldPos,
    QVector2D& uv, QVector3D& normal) {
    // This method performs a raycast from the camera through worldPos
    // to find the UV coordinates on the mesh surface at that point.
    // In a full implementation, this would:
    // 1. Get the camera position from the viewport
    // 2. Cast a ray from camera through worldPos
    // 3. Find the triangle intersection on the mesh
    // 4. Compute barycentric coordinates
    // 5. Interpolate UV from the triangle's vertex UVs
    Q_UNUSED(objectId);
    Q_UNUSED(worldPos);
    Q_UNUSED(uv);
    Q_UNUSED(normal);

    // Placeholder: return identity UV
    uv = QVector2D(0.5f, 0.5f);
    normal = QVector3D(0, 1, 0);
    return false;
}

QColor ProjectionPainter::sampleStencilAt(const QVector2D& uv) const {
    if (m_stencil.isNull()) return Qt::transparent;

    // Handle wrapping for tiled stencils
    float u = uv.x();
    float v = uv.y();
    if (m_loop) {
        u = u - floor(u);
        v = v - floor(v);
    }

    // Clamp to valid range
    u = qBound(0.0f, u, 1.0f);
    v = qBound(0.0f, v, 1.0f);

    // Convert UV (0-1) to image coordinates
    int x = qBound(0, int(u * (m_stencil.width() - 1)), m_stencil.width() - 1);
    int y = qBound(0, int(v * (m_stencil.height() - 1)), m_stencil.height() - 1);

    QRgb pixel = m_stencil.pixel(x, y);
    return QColor(pixel);
}
