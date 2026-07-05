#include "LiveryViewport.h"
#include <QVBoxLayout>
#include <QtMath>
#include <cmath>

namespace ks {

LiveryViewport::LiveryViewport(QWidget* parent)
    : QWidget(parent)
    , m_liveryEditor(LiveryEditor::instance())
{
    setFocusPolicy(Qt::StrongFocus);
    m_vulkanViewport = new VulkanViewportWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_vulkanViewport);
}

LiveryViewport::~LiveryViewport()
{
}

void LiveryViewport::setCarPath(const QString& path)
{
    m_liveryEditor->setCarPath(path);
}

void LiveryViewport::resetCamera()
{
    m_camYaw = 45.0f;
    m_camPitch = 30.0f;
    m_camDist = 10.0f;
    m_camTarget = QVector3D(0, 0, 0);
    updateCamera();
    update();
}

void LiveryViewport::focusOnModel()
{
    m_camTarget = QVector3D(0, 0.5f, 0);
    m_camDist = 5.0f;
    updateCamera();
    update();
}

void LiveryViewport::setViewMode(const QString& mode)
{
    m_viewMode = mode;
    update();
}

void LiveryViewport::mousePressEvent(QMouseEvent* e)
{
    m_lastMouse = e->pos();
    if (e->button() == Qt::LeftButton) m_lmbDown = true;
    if (e->button() == Qt::RightButton) m_rmbDown = true;
}

void LiveryViewport::mouseMoveEvent(QMouseEvent* e)
{
    QPoint delta = e->pos() - m_lastMouse;
    m_lastMouse = e->pos();

    if (m_rmbDown) {
        m_camYaw += delta.x() * 0.5f;
        m_camPitch = qBound(5.0f, m_camPitch - delta.y() * 0.5f, 89.0f);
        updateCamera();
        update();
    }
}

void LiveryViewport::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) m_lmbDown = false;
    if (e->button() == Qt::RightButton) m_rmbDown = false;
}

void LiveryViewport::wheelEvent(QWheelEvent* e)
{
    float delta = e->angleDelta().y();
    m_camDist = qBound(1.0f, m_camDist * (1.0f - delta * 0.001f), 50.0f);
    updateCamera();
    update();
}

void LiveryViewport::updateCamera()
{
    // Compute orbit camera view matrix
    float yawRad = qDegreesToRadians(m_camYaw);
    float pitchRad = qDegreesToRadians(m_camPitch);

    QVector3D eye(
        m_camTarget.x() + m_camDist * cosf(pitchRad) * sinf(yawRad),
        m_camTarget.y() + m_camDist * sinf(pitchRad),
        m_camTarget.z() + m_camDist * cosf(pitchRad) * cosf(yawRad)
    );

    QMatrix4x4 view;
    view.lookAt(eye, m_camTarget, QVector3D(0, 1, 0));

    if (m_vulkanViewport) {
        m_vulkanViewport->setViewMatrix(view);
    }
}

} // namespace ks
