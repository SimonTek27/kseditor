#pragma once

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QQuaternion>
#include <QList>
#include <QDateTime>

namespace ks {
namespace photogrammetry {

struct CameraPose {
    QVector3D position;
    QQuaternion rotation;
    qreal focalLength = 35.0;
    qreal sensorWidth = 36.0;
    qreal imageWidth = 1920.0;
    qreal imageHeight = 1080.0;
    QDateTime timestamp;
    
    CameraPose() : timestamp(QDateTime::currentDateTime()) {}
};

struct CapturedImage {
    QImage image;
    CameraPose pose;
    bool valid = false;
    
    CapturedImage() : valid(false) {}
};

class PhotogrammetryCapture : public QObject {
    Q_OBJECT
public:
    explicit PhotogrammetryCapture(QObject* parent = nullptr);
    ~PhotogrammetryCapture();
    
    // Capture an image from the current viewport camera
    Q_INVOKABLE void captureImage();
    
    // Move camera to a new position/orientation for next capture
    Q_INVOKABLE void setCameraPosition(const QVector3D& pos, const QQuaternion& rot);
    
    // Get captured images
    Q_INVOKABLE QList<CapturedImage> capturedImages() const { return m_capturedImages; }
    
    // Clear all captures
    Q_INK void clearCaptures();
    
    // Export data in COLMAP format (text files for external processing)
    Q_INVOKABLE QString exportColmapFormat(const QString& directory) const;
    
    // Set camera parameters
    Q_INK void setFocalLength(qreal fl) { m_focalLength = fl; }
    Q_INK void setSensorWidth(qreal sw) { m_sensorWidth = sw; }
    Q_INK void setImageSize(qreal w, qreal h) { m_imageWidth = w; m_imageHeight = h; }
    
signals:
    void imageCaptured(const QImage& img, const CameraPose& pose);
    void capturesChanged();
    void exportFinished(const QString& successMessage);
    
private:
    void captureFromViewport();
    CameraPose computeCameraPose() const;
    bool saveImage(const QImage& img, int index);
    QString writeColmapImagesFile() const;
    QString writeColmapCamerasFile() const;
    QString writeColmapImagesText() const;
    
    QList<CapturedImage> m_capturedImages;
    qreal m_focalLength = 35.0;
    qreal m_sensorWidth = 36.0;
    qreal m_imageWidth = 1920.0;
    qreal m_imageHeight = 1080.0;
    
    // Reference to the viewport for capturing
    // Will be set by the caller
    class VulkanViewportItem* m_viewport = nullptr;
};

} // namespace photogrammetry
} // namespace ks