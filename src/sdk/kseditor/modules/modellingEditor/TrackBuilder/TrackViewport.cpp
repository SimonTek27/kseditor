#include "TrackViewport.h"
#include <QVBoxLayout>
#include <QtMath>
#include <QKeyEvent>

namespace ks { namespace track {

TrackViewport::TrackViewport(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_viewportWidget = new TrackViewportWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_viewportWidget);
}

TrackViewport::~TrackViewport() {
}

void TrackViewport::setModule(TrackBuilderModule* module) {
    m_module = module;
    if (m_viewportWidget)
        m_viewportWidget->setModule(module);
}

void TrackViewport::resetCamera() {
    m_camYaw = 30.0f;
    m_camPitch = 45.0f;
    m_camDist = 500.0f;
    m_camTarget = QVector3D(0, 0, 0);
    if (m_viewportWidget)
        m_viewportWidget->setOrbit(m_camYaw, m_camPitch, m_camDist);
}

void TrackViewport::focusOnTerrain() {
    m_camTarget = QVector3D(0, 0, 0);
    m_camDist = 200.0f;
    if (m_viewportWidget)
        m_viewportWidget->setOrbit(m_camYaw, m_camPitch, m_camDist);
}

void TrackViewport::onTerrainModified() {
    m_terrainDirty = true;
    if (m_viewportWidget) {
        m_viewportWidget->rebuildTerrainMesh();
        m_viewportWidget->rebuildRoadMeshes();
        m_viewportWidget->update();
    }
}

void TrackViewport::onProjectChanged() {
    m_terrainDirty = true;
    if (m_viewportWidget) {
        m_viewportWidget->rebuildTerrainMesh();
        m_viewportWidget->rebuildRoadMeshes();
        m_viewportWidget->update();
    }
}

void TrackViewport::mousePressEvent(QMouseEvent* e) {
    m_lastMousePos = e->pos();
    if (e->button() == Qt::LeftButton) m_lmbDown = true;
    if (e->button() == Qt::RightButton) m_rmbDown = true;
    if (e->button() == Qt::MiddleButton) m_mmbDown = true;
}

void TrackViewport::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastMousePos;
    m_lastMousePos = e->pos();

    if (m_rmbDown) {
        m_camYaw += delta.x() * 0.5f;
        m_camPitch = qBound(5.0f, m_camPitch - delta.y() * 0.5f, 89.0f);
        if (m_viewportWidget)
            m_viewportWidget->setOrbit(m_camYaw, m_camPitch, m_camDist);
        update();
    }
    if (m_mmbDown) {
        float panSpeed = m_camDist * 0.002f;
        QVector3D fwd = QVector3D(
            qCos(qDegreesToRadians(m_camPitch)) * qSin(qDegreesToRadians(m_camYaw)),
            0,
            qCos(qDegreesToRadians(m_camPitch)) * qCos(qDegreesToRadians(m_camYaw))
        ).normalized();
        QVector3D right = QVector3D::crossProduct(fwd, {0, 1, 0}).normalized();
        m_camTarget -= right * delta.x() * panSpeed;
        m_camTarget += QVector3D(0, 1, 0) * delta.y() * panSpeed;
        if (m_viewportWidget)
            m_viewportWidget->pan(delta.x(), delta.y());
        update();
    }
}

void TrackViewport::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) m_lmbDown = false;
    if (e->button() == Qt::RightButton) m_rmbDown = false;
    if (e->button() == Qt::MiddleButton) m_mmbDown = false;
}

void TrackViewport::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y();
    m_camDist = qBound(10.0f, m_camDist * (1.0f - delta * 0.001f), 2000.0f);
    if (m_viewportWidget)
        m_viewportWidget->zoom(delta);
    update();
}

void TrackViewport::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Home) resetCamera();
    if (e->key() == Qt::Key_F) focusOnTerrain();
}

}} // namespace ks::track