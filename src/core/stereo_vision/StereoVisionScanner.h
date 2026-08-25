#pragma once

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QQuaternion>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QPair>

namespace ks {
namespace stereo_vision {

// Disparity range for depth computation
struct DisparityRange {
    int min = 0;
    int max = 256;
    int step = 1;
};

// A reconstructed 3D point with confidence and disparity
struct ReconstructedPoint {
    QVector3D position;      // 3D world position
    float disparity;         // Disparity value (pixels)
    float confidence;      // 0.0 - 1.0, reliability of this point
    QVector2D leftUv;        // Pixel coordinate in left camera
    QVector2D rightUv;       // Pixel coordinate in right camera
    
    ReconstructedPoint() : disparity(0.0f), confidence(0.0f) {}
};

// A captured stereo pair
struct CapturedPair {
    QImage leftImage;   // Left camera image
    QImage rightImage;  // Right camera image
    QDateTime timestamp;
    DisparityRange disparityRange;
    
    CapturedPair() : disparityRange{0, 256, 1} {}
};

// Scan result containing reconstructed 3D geometry
struct ScanResult {
    QList<ReconstructedPoint> points;  // All reconstructed 3D points
    QList<CapturedPair> pairs;         // All captured stereo pairs
    QVector3D cameraBaseline;          // Distance between left and right cameras
    QVector3D cameraPosition;          // Camera position (center)
    QString description;
    qreal averageDisparity = 0.0f;
    qreal scannedVolume = 0.0f;
};

class StereoVisionScanner : public QObject {
    Q_OBJECT
public:
    explicit StereoVisionScanner(QObject* parent = nullptr);
    ~StereoVisionScanner();
    
    // Start a stereo scan with specified baseline and camera parameters
    Q_INVOKABLE void startScan(qreal baseline = 5.0,
                                qreal focalLength = 35.0,
                                qreal sensorWidth = 36.0,
                                int imageWidth = 1920,
                                int imageHeight = 1080);
    
    // Capture one stereo pair (left/right images)
    Q_INK void capturePair();
    
    // Get current scan result
    Q_INK ScanResult result() const { return m_result; }
    
    // Get captured pairs
    Q_INK QList<CapturedPair> pairs() const { return m_pairs; }
    
    // Set parameters
    Q_INK void setBaseline(qreal b) { m_baseline = b; }
    Q_INK void setFocalLength(qreal f) { m_focalLength = f; }
    
signals:
    void scanStarted();
    void pairCaptured(const QImage& left, const QImage& right, const DisparityRange& range);
    void scanProgressed(const QList<ReconstructedPoint>& newPoints, qreal progress);
    void scanCompleted(const ScanResult& result);
    void errorOccurred(const QString& error);
    
private:
    // Compute disparity map from left/right images (simulated)
    QImage computeDisparityMap(const QImage& left, const QImage& right, const DisparityRange& range);
    
    // Triangulate a 3D point from corresponding pixels in left/right images
    bool triangulatePoint(const QVector2D& leftUv, const QVector2D& rightUv,
                          float disparity,
                          const QVector3D& cameraPos,
                          const QVector3D& cameraBaseline,
                          QVector3D& position, float& confidence);
    
    // Process a captured pair and add reconstructed points
    void processPair(const CapturedPair& pair);
    
    // Generate synthetic stereo pair for demonstration
    void simulateStereoPair(CapturedPair& pair);
    
    // Camera parameters
    qreal m_baseline;             // Distance between left and right cameras (mm)
    qreal m_focalLength;          // Focal length (mm)
    qreal m_sensorWidth;          // Sensor width (mm)
    int m_imageWidth;
    int m_imageHeight;
    
    // State
    ScanResult m_result;
    QList<CapturedPair> m_pairs;
    QMutex m_mutex;
    int m_currentPair = 0;
    bool m_scanning = false;
};

} // namespace stereo_vision
} // namespace ks