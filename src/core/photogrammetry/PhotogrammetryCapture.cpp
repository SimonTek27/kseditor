#include "PhotogrammetryCapture.h"
#include <QVulkanWindow>
#include <QQuickFramebufferObject>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace ks {
namespace photogrammetry {

PhotogrammetryCapture::PhotogrammetryCapture(QObject* parent)
    : QObject(parent)
    , m_viewport(nullptr)
{
}

PhotogrammetryCapture::~PhotogrammetryCapture()
{
    clearCaptures();
}

void PhotogrammetryCapture::captureImage()
{
    if (!m_viewport) {
        qWarning() << "No viewport set for capture";
        return;
    }
    
    captureFromViewport();
}

void PhotogrammetryCapture::setCameraPosition(const QVector3D& pos, const QQuaternion& rot)
{
    // This will be called by the QML bridge to move the camera
    // The actual camera movement happens in the viewport
    Q_UNUSED(pos);
    Q_UNUSED(rot);
    // Emit that a capture should happen at the current pose
    emit imageCaptured(QImage(), computeCameraPose());
}

QList<CapturedImage> PhotogrammetryCapture::capturedImages() const
{
    return m_capturedImages;
}

void PhotogrammetryCapture::clearCaptures()
{
    m_capturedImages.clear();
    emit capturesChanged();
}

QString PhotogrammetryCapture::exportColmapFormat(const QString& directory) const
{
    QString result;
    
    // Create COLMAP format files
    // 1. images.txt
    // 2. cameras.txt
    // 3. points3D.txt (empty for now, user will reconstruct)
    
    QDir dir(directory);
    if (!dir.mkpath(".")) {
        return "Failed to create directory";
    }
    
    // Write images.txt
    QString imagesFile = dir.filePath("images.txt");
    QFile f_images(imagesFile);
    if (f_images.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f_images);
        out << "# Camera images file for COLMAP\n";
        out << "# Format: image_name, qw, qx, qy, qz, tx, ty, tz, camera_id, name\n";
        
        for (int i = 0; i < m_capturedImages.size(); ++i) {
            const auto& img = m_capturedImages[i];
            if (!img.valid) continue;
            
            const auto& pose = img.pose;
            // COLMAP uses camera_id reference; we'll write 1 for now
            out << "image_" << i << ".png, "
                << pose.rotation.qw() << " " << pose.rotation.qx() << " "
                << pose.rotation.qy() << " " << pose.rotation.qz() << " , "
                << pose.position.x() << " " << pose.position.y() << " " << pose.position.z() << " , "
                << 1 << ", " << img.image.fileName().isEmpty() ? QString("image_%1.png").arg(i) : img.image.fileName()
                << "\n";
        }
        f_images.close();
        result += "Wrote images.txt with " + QString::number(m_capturedImages.size()) + " images\n";
    }
    
    // Write cameras.txt
    QString camerasFile = dir.filePath("cameras.txt");
    QFile f_cameras(camerasFile);
    if (f_cameras.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f_cameras);
        out << "# Camera file for COLMAP\n";
        out << "# Format: camera_id, model, width, height, params\n";
        out << "1, PINHOLE, " << m_imageWidth << ", " << m_imageHeight << ", "
            << m_focalLength << ", " << m_sensorWidth << "\n";
        f_cameras.close();
        result += "Wrote cameras.txt\n";
    }
    
    emit exportFinished(result);
    return result;
}

void PhotogrammetryCapture::captureFromViewport()
{
    if (!m_viewport) {
        qWarning() << "No viewport set";
        return;
    }
    
    // Render the current frame and grab the image
    // The viewport should have rendered already; we read the framebuffer
    QQuickFramebufferObject* QO = qobject_cast<QQuickFramebufferObject*>(m_viewport->parent());
    if (QO) {
        // Trigger render and read back
        QO->reset();
        QTimer::singleShot(50, this, [this]() {
            // Read pixels from the framebuffer
            // This is simplified - real implementation would use Vulkan image memory read
            QImage img = m_viewport->renderer()->toImage();
            
            if (!img.isNull()) {
                CameraPose pose = computeCameraPose();
                CapturedImage ci;
                ci.image = img;
                ci.pose = pose;
                ci.valid = true;
                
                m_capturedImages.append(ci);
                emit capturesChanged();
                emit imageCaptured(img, pose);
            }
        });
    } else {
        qWarning() << "Viewport is not a QQuickFramebufferObject";
    }
}

CameraPose PhotogrammetryCapture::computeCameraPose() const
{
    CameraPose pose;
    
    // Get camera from the viewport's scene graph
    // The camera position/target are stored in the VulkanViewportItem
    // For now, we'll use placeholder values based on the viewport's known camera state
    
    // In a full implementation, we'd query the VulkanViewportItem for its current camera transform
    // pose.position = m_viewport->cameraPosition();  // would need getter
    // pose.rotation = m_viewport->cameraRotation();  // would need getter
    
    // Temporary: use identity for now - user should move camera manually
    pose.position = QVector3D(0, 0, 50);
    pose.rotation = QQuaternion::fromEulerAngles(0, 0, 0);
    pose.focalLength = m_focalLength;
    pose.sensorWidth = m_sensorWidth;
    pose.imageWidth = m_imageWidth;
    pose.imageHeight = m_imageHeight;
    
    return pose;
}

} // namespace photogrammetry
} // namespace ks