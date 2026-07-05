#include "CharacterViewport.h"
#include <QVBoxLayout>
#include <QtMath>
#include <QKeyEvent>

namespace ks {

CharacterViewport::CharacterViewport(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    m_vulkanViewport = new VulkanViewportWidget(this);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_vulkanViewport);
}

CharacterViewport::~CharacterViewport() {
}

void CharacterViewport::setCharacterEditor(CharacterEditor* editor) {
    m_characterEditor = editor;
}

void CharacterViewport::resetCamera() {
    m_camYaw = 45.0f;
    m_camPitch = 30.0f;
    m_camDist = 10.0f;
    m_camTarget = QVector3D(0, 0.9f, 0);
    update();
}

void CharacterViewport::focusOnCharacter() {
    m_camTarget = QVector3D(0, 0.9f, 0);
    m_camDist = 5.0f;
    update();
}

void CharacterViewport::setViewMode(const QString& mode) {
    m_displayMode = mode;
    update();
}

void CharacterViewport::setDisplayMode(const QString& mode) {
    m_displayMode = mode;
    update();
}

void CharacterViewport::mousePressEvent(QMouseEvent* e) {
    m_lastMouse = e->pos();
    if (e->button() == Qt::LeftButton) m_lmbDown = true;
    if (e->button() == Qt::RightButton) m_rmbDown = true;
    if (e->button() == Qt::MiddleButton) m_mmbDown = true;
}

void CharacterViewport::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastMouse;
    m_lastMouse = e->pos();

    if (m_rmbDown) {
        m_camYaw += delta.x() * 0.5f;
        m_camPitch = qBound(5.0f, m_camPitch - delta.y() * 0.5f, 89.0f);
        update();
    }
}

void CharacterViewport::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) m_lmbDown = false;
    if (e->button() == Qt::RightButton) m_rmbDown = false;
    if (e->button() == Qt::MiddleButton) m_mmbDown = false;
}

void CharacterViewport::wheelEvent(QWheelEvent* e) {
    float delta = e->angleDelta().y();
    m_camDist = qBound(1.0f, m_camDist * (1.0f - delta * 0.001f), 50.0f);
    update();
}

void CharacterViewport::keyPressEvent(QKeyEvent* e) {
    float yawRad = qDegreesToRadians(m_camYaw);
    QVector3D fwd(-std::sin(yawRad), 0, -std::cos(yawRad));
    QVector3D right(std::cos(yawRad), 0, -std::sin(yawRad));
    float spd = m_camDist * 0.05f;
    if (e->key() == Qt::Key_W) m_camTarget += fwd * spd;
    if (e->key() == Qt::Key_S) m_camTarget -= fwd * spd;
    if (e->key() == Qt::Key_A) m_camTarget -= right * spd;
    if (e->key() == Qt::Key_D) m_camTarget += right * spd;
    if (e->key() == Qt::Key_Home) resetCamera();
    update();
}

void CharacterViewport::updateCamera() {
    // Camera is managed by VulkanViewportWidget
}

}