#include "VulkanViewportItem.h"
#include <QtMath>
#include <algorithm>
#include <limits>

namespace ks {

VulkanViewportItem::VulkanViewportItem(QQuickItem* parent)
    : QQuickFramebufferObject(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setMirrorVertically(true);
}

VulkanViewportItem::~VulkanViewportItem() = default;

QQuickFramebufferObject::Renderer* VulkanViewportItem::createRenderer() const {
    m_rendererWrapper = new VulkanViewportRendererWrapper(nullptr, m_sceneGraph);
    return reinterpret_cast<QQuickFramebufferObject::Renderer*>(m_rendererWrapper);
}

void VulkanViewportItem::setCamYaw(qreal yaw) {
    if (qFuzzyCompare(m_camYaw, yaw)) return;
    m_camYaw = yaw;
    updateCamera();
    emit camYawChanged();
}

void VulkanViewportItem::setCamPitch(qreal pitch) {
    pitch = qBound(-89.0, pitch, 89.0);
    if (qFuzzyCompare(m_camPitch, pitch)) return;
    m_camPitch = pitch;
    updateCamera();
    emit camPitchChanged();
}

void VulkanViewportItem::setCamDistance(qreal dist) {
    dist = qMax(0.5, dist);
    if (qFuzzyCompare(m_camDistance, dist)) return;
    m_camDistance = dist;
    updateCamera();
    emit camDistanceChanged();
}

void VulkanViewportItem::setCamTarget(const QVector3D& target) {
    if (m_camTarget == target) return;
    m_camTarget = target;
    updateCamera();
    emit camTargetChanged();
}

void VulkanViewportItem::setGridVisible(bool visible) {
    if (m_gridVisible == visible) return;
    m_gridVisible = visible;
    emit gridVisibleChanged();
    update();
}

void VulkanViewportItem::setViewMode(int mode) {
    if (m_viewMode == mode) return;
    m_viewMode = mode;
    emit viewModeChanged();
    update();
}

void VulkanViewportItem::setGizmoVisible(bool visible) {
    if (m_gizmoVisible == visible) return;
    m_gizmoVisible = visible;
    emit gizmoVisibleChanged();
    update();
}

void VulkanViewportItem::setGizmoMode(int mode) {
    if (m_gizmoMode == mode) return;
    m_gizmoMode = mode;
    emit gizmoModeChanged();
    update();
}

void VulkanViewportItem::orbit(qreal dx, qreal dy) {
    m_camYaw += dx;
    m_camPitch = qBound(-89.0, m_camPitch + dy, 89.0);
    updateCamera();
    emit camYawChanged();
    emit camPitchChanged();
}

void VulkanViewportItem::pan(qreal dx, qreal dy) {
    QMatrix4x4 view;
    double yawRad = qDegreesToRadians(m_camYaw);
    double pitchRad = qDegreesToRadians(m_camPitch);
    QVector3D pos(
        m_camTarget.x() + m_camDistance * std::cos(pitchRad) * std::sin(yawRad),
        m_camTarget.y() + m_camDistance * std::sin(pitchRad),
        m_camTarget.z() + m_camDistance * std::cos(pitchRad) * std::cos(yawRad)
    );
    view.lookAt(pos, m_camTarget, QVector3D(0, 1, 0));
    QMatrix4x4 inv = view.inverted();
    QVector3D right = inv.mapVector(QVector3D(1, 0, 0)) * dx * m_camDistance * 0.002;
    QVector3D up = inv.mapVector(QVector3D(0, 1, 0)) * dy * m_camDistance * 0.002;
    m_camTarget += right + up;
    updateCamera();
    emit camTargetChanged();
}

void VulkanViewportItem::zoom(qreal delta) {
    m_camDistance *= (1.0 - delta * 0.001);
    m_camDistance = qMax(0.5, m_camDistance);
    updateCamera();
    emit camDistanceChanged();
}

void VulkanViewportItem::resetCamera() {
    m_camTarget = QVector3D(0, 0, 0);
    m_camYaw = 30.0;
    m_camPitch = 45.0;
    m_camDistance = 50.0;
    updateCamera();
    emit camYawChanged();
    emit camPitchChanged();
    emit camDistanceChanged();
    emit camTargetChanged();
}

void VulkanViewportItem::focusOnSelection() {
    if (!m_sceneGraph) return;
    auto all = m_sceneGraph->allObjects();
    QVector3D center;
    int count = 0;
    for (auto* obj : all) {
        if (obj && obj->isSelected()) {
            center += obj->translation();
            ++count;
        }
    }
    if (count > 0) {
        center /= count;
        m_camTarget = center;
        m_camDistance = 10.0;
        updateCamera();
        emit camTargetChanged();
        emit camDistanceChanged();
    }
}

void VulkanViewportItem::setSceneGraph(SceneGraph* scene) {
    m_sceneGraph = scene;
    if (m_rendererWrapper)
        m_rendererWrapper->setScene(scene);
    update();
}

void VulkanViewportItem::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->pos();
    if (event->button() == Qt::LeftButton) m_lmbDown = true;
    if (event->button() == Qt::MiddleButton) m_mmbDown = true;
    if (event->button() == Qt::RightButton) m_rmbDown = true;
    event->accept();
}

void VulkanViewportItem::mouseMoveEvent(QMouseEvent* event) {
    QPointF delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (m_lmbDown) {
        orbit(delta.x(), delta.y());
    } else if (m_rmbDown) {
        pan(delta.x(), delta.y());
    } else if (m_mmbDown) {
        zoom(delta.y());
    }
    event->accept();
}

void VulkanViewportItem::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) m_lmbDown = false;
    if (event->button() == Qt::MiddleButton) m_mmbDown = false;
    if (event->button() == Qt::RightButton) m_rmbDown = false;
    event->accept();
}

void VulkanViewportItem::wheelEvent(QWheelEvent* event) {
    zoom(event->angleDelta().y());
    event->accept();
}

void VulkanViewportItem::tabletEvent(QTabletEvent* event)
{
    float pressure = qBound(0.05f, float(event->pressure()), 1.0f);
    float tiltX = event->xTilt();
    float tiltY = event->yTilt();
    QPointF pos = event->position();
    switch (event->type()) {
    case QEvent::TabletPress:
        m_lastMousePos = pos;
        if (event->button() == Qt::LeftButton || event->buttons() & Qt::LeftButton) m_lmbDown = true;
        event->accept();
        break;
    case QEvent::TabletMove: {
        QPointF delta = pos - m_lastMousePos;
        m_lastMousePos = pos;
        float scaledX = float(delta.x()) * (0.5f + 0.5f * pressure);
        float scaledY = float(delta.y()) * (0.5f + 0.5f * pressure);
        // Tilt modulates camera roll: tiltX controls roll (rotation around camera Z axis)
        float rollAngle = tiltX * 0.5f; // tiltX -> roll, scaled modestly
        if (m_lmbDown) orbit(scaledX, scaledY);
        else if (m_rmbDown) pan(scaledX, scaledY);
        else if (m_mmbDown) zoom(scaledY);
        // Apply roll when left button is down for natural camera orientation
        if (m_lmbDown && std::abs(rollAngle) > 0.01f) {
            // Roll is applied as a small additional yaw/pitch adjustment
            float rollFactor = std::abs(rollAngle) * 0.1f;
            if (rollAngle > 0) orbit(-rollFactor, 0);
            else orbit(rollFactor, 0);
        }
        event->accept();
        break;
    }
    case QEvent::TabletRelease:
        if (event->button() == Qt::LeftButton) m_lmbDown = false;
        m_lmbDown = false;
        m_mmbDown = false;
        m_rmbDown = false;
        event->accept();
        break;
    default:
        break;
    }
    update();
}

void VulkanViewportItem::hoverMoveEvent(QHoverEvent* event) {
    Q_UNUSED(event);
    setCursor(Qt::CrossCursor);
}

void VulkanViewportItem::hoverLeaveEvent(QHoverEvent* event) {
    Q_UNUSED(event);
    unsetCursor();
}

void VulkanViewportItem::updateCamera() {
    if (!m_rendererWrapper) return;
    double yawRad = qDegreesToRadians(m_camYaw);
    double pitchRad = qDegreesToRadians(m_camPitch);
    QVector3D eye(
        m_camTarget.x() + m_camDistance * std::cos(pitchRad) * std::sin(yawRad),
        m_camTarget.y() + m_camDistance * std::sin(pitchRad),
        m_camTarget.z() + m_camDistance * std::cos(pitchRad) * std::cos(yawRad)
    );
    QMatrix4x4 view;
    view.lookAt(eye, m_camTarget, QVector3D(0, 1, 0));
    QMatrix4x4 proj;
    float aspect = width() / qMax(1.0f, height());
    proj.perspective(50.0, aspect, 0.1, 10000.0);
    m_rendererWrapper->renderer().updateViewportSize(static_cast<int>(width()), static_cast<int>(height()));
    update();
}

void VulkanViewportItem::updateGizmo() {
    if (!m_gizmoVisible || !m_sceneGraph) return;
    update();
}

} // namespace ks