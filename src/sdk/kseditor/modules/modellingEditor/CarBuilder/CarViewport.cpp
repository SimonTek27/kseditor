#include "CarViewport.h"
#include <QVBoxLayout>
#include <QtMath>

namespace ks {

CarViewport::CarViewport(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_vulkanViewport = new VulkanViewportWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_vulkanViewport);
}

CarViewport::~CarViewport() {
}

void CarViewport::setCarEditor(CarEditor* editor) {
    m_carEditor = editor;
}

void CarViewport::resetCamera() {
    m_camYaw = 45.0f;
    m_camPitch = 30.0f;
    m_camDist = 10.0f;
    m_camTarget = QVector3D(0, 0, 0);
    update();
}

void CarViewport::focusOnCar() {
    m_camTarget = QVector3D(0, 0.5f, 0);
    m_camDist = 5.0f;
    update();
}

void CarViewport::setViewMode(const QString& mode) {
    m_viewMode = mode;
    update();
}

void CarViewport::mousePressEvent(QMouseEvent* e) {
    m_lastMouse = e->pos();
    if (e->button() == Qt::LeftButton) m_lmbDown = true;
    if (e->button() == Qt::RightButton) m_rmbDown = true;
}

void CarViewport::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastMouse;
    m_lastMouse = e->pos();

    if (m_rmbDown) {
        m_camYaw += delta.x() * 0.5f;
        m_camPitch = qBound(5.0f, m_camPitch - delta.y() * 0.5f, 89.0f);
        update();
    }
}

void CarViewport::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) m_lmbDown = false;
    if (e->button() == Qt::RightButton) m_rmbDown = false;
}

void CarViewport::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y();
    m_camDist = qBound(1.0f, m_camDist * (1.0f - delta * 0.001f), 50.0f);
    update();
}

void CarViewport::updateCamera() {
    // Camera is managed by VulkanViewportWidget
}

}