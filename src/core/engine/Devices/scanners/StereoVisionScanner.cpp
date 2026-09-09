#include "StereoVisionScanner.h"
#include <QDebug>
#include <cmath>
#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QTimer>

namespace ks {
namespace stereo_vision {

StereoVisionScanner::StereoVisionScanner(QObject* parent)
    : QObject(parent)
    , m_baseline(5.0)
    , m_focalLength(35.0)
    , m_sensorWidth(36.0)
    , m_imageWidth(1920)
    , m_imageHeight(1080)
{
    m_result.description = "Stereo vision scan";
}

StereoVisionScanner::~StereoVisionScanner()
{
}

void StereoVisionScanner::startScan(qreal baseline, qreal focalLength, qreal sensorWidth,
                                    int imageWidth, int imageHeight)
{
    m_baseline = qMax(0.1f, baseline);
    m_focalLength = qMax(0.1f, focalLength);
    m_sensorWidth = qMax(0.1f, sensorWidth);
    m_imageWidth = qMax(64, imageWidth);
    m_imageHeight = qMax(48, imageHeight);
    
    m_currentPair = 0;
    m_pairs.clear();
    m_result = ScanResult();
    m_result.cameraBaseline = QVector3D(m_baseline, 0, 0);
    m_result.cameraPosition = QVector3D(0, 0, 0);
    m_result.description = QString("Stereo vision scan (baseline: %1mm)").arg(m_baseline, 0, 'f', 1);
    m_result.averageDisparity = 0.0f;
    m_result.scannedVolume = 0.0f;
    m_result.points.clear();
    
    m_scanning = true;
    
    emit scanStarted();
    
    // Capture first pair immediately
    capturePair();
}

void StereoVisionScanner::capturePair()
{
    if (!m_scanning) {
        qWarning() << "Scanner not running";
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    if (m_currentPair >= 10) {  // Limit number of pairs for demo
        m_scanning = false;
        finishReconstruction();
        emit scanCompleted(m_result);
        return;
    }
    
    // Simulate capturing a stereo pair
    CapturedPair pair;
    simulateStereoPair(pair);
    
    // Compute disparity map and reconstruct points
    processPair(pair);
    
    // Store the pair
    m_pairs.append(pair);
    
    // Emit signal
    emit pairCaptured(pair.leftImage, pair.rightImage, pair.disparityRange);
    
    // Update progress
    m_currentPair++;
    qreal progress = (static_cast<qreal>(m_currentPair) / 10.0f) * 100.0f;
    emit scanProgressed(m_result.points, progress);
    
    // Capture next pair after delay
    if (m_scanning && m_currentPair < 10) {
        QTimer::singleShot(300, this, [this]() {
            capturePair();
        });
    } else if (m_scanning) {
        // All pairs captured, finish
        m_scanning = false;
        finishReconstruction();
        emit scanCompleted(m_result);
    }
}

void StereoVisionScanner::simulateStereoPair(CapturedPair& pair)
{
    // Generate left image (checkerboard pattern for feature matching)
    pair.leftImage = QImage(m_imageWidth, m_imageHeight, QImage::Format_Grayscale8);
    pair.leftImage.fill(0);
    
    QRgb* leftScan = nullptr;
    for (int y = 0; y < m_imageHeight; y += 2) {
        leftScan = reinterpret_cast<QRgb*>(pair.leftImage.scanLine(y));
        for (int x = 0; x < m_imageWidth; x += 2) {
            leftScan[x] = 255;  // White pixels
        }
    }
    // Draw checkerboard
    for (int y = 0; y < m_imageHeight; ++y) {
        for (int x = 0; x < m_imageWidth; ++x) {
            bool white = ((x / 20) + (y / 20)) % 2 == 0;
            pair.leftImage.setPixel(x, y, white ? 255 : 0);
        }
    }
    
    // Generate right image with horizontal shift (simulates disparity)
    pair.rightImage = pair.leftImage.copy();
    
    // Simulate disparity: shift depends on "depth" - closer objects have larger shift
    QRgb* rightScan = nullptr;
    for (int y = 0; y < m_imageHeight; ++y) {
        rightScan = reinterpret_cast<QRgb*>(pair.rightImage.scanLine(y));
        for (int x = 0; x < m_imageWidth; ++x) {
            // Compute simulated disparity based on position
            // Center objects (x ~ imageWidth/2) have zero disparity
            // Edge objects have maximum disparity
            int dx = qBound(-64, int((x - m_imageWidth / 2) * 64 / (m_imageWidth / 2)), 64);
            
            // Sample from left image at shifted position
            int sampleX = qBound(0, x + dx, m_imageWidth - 1);
            rightScan[x] = pair.leftImage.pixel(sampleX, y);
        }
    }
    
    // Set disparity range based on maximum shift
    pair.disparityRange = DisparityRange{-64, 64, 1};
}

void StereoVisionScanner::processPair(const CapturedPair& pair)
{
    // Compute disparity map (simplified)
    // In a real system, this would use SAD (Sum of Absolute Differences) or SAD/SGBM algorithms
    
    // For each pixel, compute disparity and triangulate
    const uchar* leftScan = nullptr;
    const uchar* rightScan = nullptr;
    
    int validPoints = 0;
    float totalDisparity = 0.0f;
    
    for (int y = 0; y < m_imageHeight; y += 8) {  // Sample every 8 pixels for demo
        leftScan = pair.leftImage.constScanLine(y);
        rightScan = pair.rightImage.constScanLine(y);
        
        for (int x = 0; x < m_imageWidth; x += 8) {
            // Get pixel values (simplified: just use brightness)
            uchar leftVal = leftScan[x];
            uchar rightVal = rightScan[x];
            
            // Compute disparity using block matching (simplified)
            // Search for matching window in right image
            int bestDisparity = 0;
            int bestScore = 10000;
            
            // Search range
            int searchMin = -64;
            int searchMax = qMin(64, m_imageWidth - x);
            
            for (int d = searchMin; d < searchMax; d += 1) {
                // Compare windows (very simplified: just compare single pixel)
                if (d + x < m_imageWidth) {
                    uchar rightValAtShift = rightScan[qMin(m_imageWidth - 1, x + d)];
                    int score = qAbs(int(leftVal) - int(rightValAtShift));
                    if (score < bestScore) {
                        bestScore = score;
                        bestDisparity = d;
                    }
                }
            }
            
            // Only add point if good match found
            if (bestScore < 50) {  // Threshold for good match
                // Compute 3D position via triangulation
                float disparity = static_cast<float>(bestDisparity);
                
                // Skip zero disparity (infinite depth)
                if (qFabs(disparity) < 0.1f) continue;
                
                // Triangulate 3D point
                QVector3D position;
                float confidence;
                
                // Camera setup: baseline along X axis, camera at Z distance
                float Z = (m_focalLength * m_baseline) / qMax(0.1f, disparity);
                float X = (Z * (x - m_imageWidth / 2.0f)) / m_focalLength;
                float Y = (Z * (y - m_imageHeight / 2.0f)) / m_focalLength;
                
                position = QVector3D(X, Y, Z);
                
                // Confidence based on disparity magnitude and match quality
                confidence = qBound(0.0f, 1.0f - qAbs(disparity) / 64.0f, 1.0f);
                confidence *= qMax(0.1f, 1.0f - bestScore / 100.0f);
                
                // Create reconstructed point
                ReconstructedPoint pt;
                pt.position = position;
                pt.disparity = disparity;
                pt.confidence = confidence;
                pt.leftUv = QVector2D(x, y);
                pt.rightUv = QVector2D(x + bestDisparity, y);
                
                m_result.points.append(pt);
                totalDisparity += qAbs(disparity);
                validPoints++;
            }
        }
    }
    
    // Update result statistics
    if (validPoints > 0) {
        m_result.averageDisparity = totalDisparity / validPoints;
    }
    
    // Estimate scanned volume (rough: bounding box of points)
    if (!m_result.points.isEmpty()) {
        QVector3D minPt = m_result.points[0].position;
        QVector3D maxPt = m_result.points[0].position;
        
        for (const auto& pt : m_result.points) {
            minPt = QVector3D(qMin(minPt.x(), pt.position.x()),
                              qMin(minPt.y(), pt.position.y()),
                              qMin(minPt.z(), pt.position.z()));
            maxPt = QVector3D(qMax(maxPt.x(), pt.position.x()),
                              qMax(maxPt.y(), pt.position.y()),
                              qMax(maxPt.z(), pt.position.z()));
        }
        
        m_result.scannedVolume = qMax(1.0f, 
            (maxPt.x() - minPt.x()) * 
            (maxPt.y() - minPt.y()) * 
            (maxPt.z() - minPt.z()) / 1000.0f);  // Scale factor
        
        m_result.description += QString(" - %1 points, avg disp: %2")
            .arg(validPoints)
            .arg(QString::number(m_result.averageDisparity, 'f', 1));
    }
}

void StereoVisionScanner::finishReconstruction()
{
    // Final volume estimation
    if (!m_result.points.isEmpty()) {
        QVector3D minPt = m_result.points[0].position;
        QVector3D maxPt = m_result.points[0].position;
        
        for (const auto& pt : m_result.points) {
            minPt = QVector3D(qMin(minPt.x(), pt.position.x()),
                              qMin(minPt.y(), pt.position.y()),
                              qMin(minPt.z(), pt.position.z()));
            maxPt = QVector3D(qMax(maxPt.x(), pt.position.x()),
                              qMax(maxPt.y(), pt.position.y()),
                              qMax(maxPt.z(), pt.position.z()));
        }
        
        m_result.scannedVolume = qMax(1.0f,
            (maxPt.x() - minPt.x()) * 
            (maxPt.y() - minPt.y()) * 
            (maxPt.z() - minPt.z()) / 1000.0f);
    }
    
    m_result.description += QString(" - completed with %1 total points")
        .arg(m_result.points.size());
}

// result() and pairs() are defined inline in the header

// ----------------------------------------------------------------------------
// Internal: Compute disparity map (placeholder - real implementation would use
// block matching, SGBM, or other algorithm)
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Internal: Triangulate 3D point from disparity
// ----------------------------------------------------------------------------

// Formula: Z = (f * B) / d
// Where: f = focal length, B = baseline, d = disparity
//        X = (Z * (u - cx)) / f
//        Y = (Z * (v - cy)) / f
// ----------------------------------------------------------------------------

} // namespace stereo_vision
} // namespace ks