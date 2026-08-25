#pragma once

#include <QObject>
#include <QImage>
#include <QVector3D>
#include <QList>
#include <QMutex>
#include <QDateTime>

namespace ks {
namespace structured_light {

// Pattern types for structured light scanning
enum class PatternType {
    Grid,           // Grid pattern (horizontal + vertical lines)
    Cross,          // Cross pattern (+ shape)
    Circle,         // Circle pattern
    RandomDots,     // Random dot pattern
    PhaseShift1,    // Single phase-shift pattern
    PhaseShift3     // Three phase-shifted patterns (0, 120, 240 degrees)
};

// A captured frame with its pattern info
struct CapturedFrame {
    QImage pattern;       // Projected pattern image
    QImage captured;      // Camera-observed image
    QDateTime timestamp;
    bool valid = false;   // Whether triangulation succeeded
    
    CapturedFrame() = default;
};

// A reconstructed 3D point with confidence
struct ReconstructedPoint {
    QVector3D position;   // 3D world position
    float confidence;     // 0.0 - 1.0, how reliable this point is
    QVector2D uv;         // Original pixel coordinates (for reference)
    
    ReconstructedPoint() : confidence(0.0f) {}
};

// Scan result containing all reconstructed geometry
struct ScanResult {
    QList<ReconstructedPoint> points;    // All reconstructed 3D points
    QList<CapturedFrame> frames;         // All captured pattern+image pairs
    QList<QVector3D> cameraPositions;    // Camera positions used
    QString description;
    qreal averageConfidence = 0.0f;
    qreal scannedVolume = 0.0f;          // Estimated volume of scanned object
};

class StructuredLightScanner : public QObject {
    Q_OBJECT
public:
    explicit StructuredLightScanner(QObject* parent = nullptr);
    ~StructuredLightScanner();
    
    // Start scanning with specified pattern type
    Q_INVOKABLE void startScan(PatternType patternType = PatternType::Grid,
                                int numFrames = 5,
                                qreal baseline = 5.0);
    
    // Capture one frame (advances to next pattern automatically)
    Q_INK void captureFrame();
    
    // Stop current scan
    Q_INVOKABLE void stopScan();
    
    // Get current scan result
    Q_INK ScanResult result() const { return m_result; }
    
    // Get captured frames list
    Q_INK QList<CapturedFrame> frames() const { return m_frames; }
    
    // Set parameters
    Q_INK void setBaseline(qreal b) { m_baseline = b; }
    Q_INK void setProjectorDistance(qreal d) { m_projectorDistance = d; }
    
signals:
    void scanStarted();
    void frameCaptured(const QImage& pattern, const QImage& captured, bool valid);
    void scanProgressed(const QList<ReconstructedPoint>& newPoints, qreal progress);
    void scanCompleted(const ScanResult& result);
    void errorOccurred(const QString& error);
    
private:
    // Simulate projecting a pattern and "capture" an image
    void simulateCapture(PatternType type, QImage& outPattern, QImage& outCaptured);
    
    // Triangulate a 3D point from two camera views
    bool triangulate(const QVector3D& ray1Orig, const QVector3D& ray1Dir,
                     const QVector3D& ray2Orig, const QVector3D& ray2Dir,
                     QVector3D& intersection, float& confidence);
    
    // Process all captured frames and reconstruct geometry
    void reconstructGeometry();
    
    // Generate a simulated pattern image
    QImage generatePattern(PatternType type, const QSize& size);
    
    // Camera/projector parameters
    PatternType m_patternType;
    int m_numFrames;
    qreal m_baseline;           // Distance between projector and camera
    qreal m_projectorDistance;  // Distance from turntable/center
    qreal m_cameraDistance;
    
    // State
    ScanResult m_result;
    QList<CapturedFrame> m_frames;
    QMutex m_mutex;
    int m_currentFrame = 0;
    bool m_scanning = false;
};

} // namespace structured_light
} // namespace ks