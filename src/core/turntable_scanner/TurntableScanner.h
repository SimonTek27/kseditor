#pragma once

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QQuaternion>
#include <QList>
#include <QMap>
#include <QDateTime>

namespace ks {
namespace turntable_scanner {

struct TurntablePose {
    QVector3D cameraPosition;   // Position on turntable circle
    QVector3D cameraTarget;     // Looking at center of turntable
    QVector3D objectPosition;   // Center of turntable
    qreal cameraDistance;       // Distance from turntable center
    qreal turntableAngle;       // Current rotation angle
    QDateTime timestamp;
    
    TurntablePose() : cameraDistance(5.0), turntableAngle(0.0) {
        timestamp = QDateTime::currentDateTime();
        // Default: camera at distance 5, angle 0 (view from front)
        objectPosition = QVector3D(0, 0, 0);
        cameraTarget = QVector3D(0, 0, 0);
        cameraPosition = QVector3D(0, 0, 5); // Default Z-up position
    }
};

struct ScanProgress {
    int totalImages = 0;
    int capturedImages = 0;
    bool scanning = false;
    QString currentStep;
    float progressPercentage = 0.0f;
};

class TurntableScanner : public QObject {
    Q_OBJECT
public:
    explicit TurntableScanner(QObject* parent = nullptr);
    ~TurntableScanner();
    
    // Start a scan with specified parameters
    Q_INVOKABLE void startScan(int numPositions, qreal turntableRadius = 5.0,
                              qreal cameraDistance = 5.0, qreal overlap = 0.5);
    
    // Stop current scan
    Q_INVOKABLE void stopScan();
    
    // Capture one image at current position
    Q_INK void captureCurrentImage();
    
    // Advance to next position
    Q_INVOKABLE void advancePosition();
    
    // Get progress status
    Q_INK ScanProgress progress() const { return m_progress; }
    
    // Get all captured poses
    Q_INK QList<TurntablePose> capturedPoses() const { return m_poses; }
    
    // Get captured images
    Q_INK QList<QImage> capturedImages() const { return m_images; }
    
    // Export in photogrammetry-friendly format
    Q_INK QString exportForPhotogrammetry(const QString& directory) const;
    
    // Set turntable radius
    Q_INK void setTurntableRadius(qreal radius) { m_turntableRadius = radius; }
    
    // Set camera height above turntable
    Q_INK void setCameraHeight(qreal height) { m_cameraHeight = height; }
    
signals:
    void scanStarted();
    void scanProgressed(const ScanProgress& progress);
    void scanCompleted(const QString& result);
    void imageCaptured(const QImage& img, const TurntablePose& pose);
    void stepChanged(int stepIndex);
    
private:
    void computePoses(int numPositions);
    TurntablePose computePoseAtIndex(int index);
    bool saveImage(const QImage& img, int index);
    QString exportImagesTxt() const;
    QString exportPosesTxt() const;
    
    // Scan parameters
    int m_numPositions = 36;      // Number of positions around turntable
    qreal m_turntableRadius = 5.0; // Radius of turntable movement
    qreal m_cameraDistance = 5.0; // Distance from turntable center
    qreal m_cameraHeight = 0.0;   // Height above turntable (0 = same level)
    qreal m_overlap = 0.5;        // Overlap between consecutive images (0-1)
    
    // State
    ScanProgress m_progress;
    QList<TurntablePose> m_poses;    // Computed positions
    QList<QImage> m_images;          // Captured images
    int m_currentIndex = 0;          // Current position index
    
    // Reference to viewport
    class VulkanViewportItem* m_viewport = nullptr;
};

} // namespace turntable_scanner
} // namespace ks