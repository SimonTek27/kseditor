#include "StructuredLightScanner.h"
#include <QDebug>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <QRandomGenerator>
#include <QTimer>

namespace ks {
namespace structured_light {

StructuredLightScanner::StructuredLightScanner(QObject* parent)
    : QObject(parent)
    , m_baseline(5.0)
    , m_projectorDistance(5.0)
    , m_cameraDistance(5.0)
{
    m_result.description = "Structured light scan";
}

StructuredLightScanner::~StructuredLightScanner()
{
}

void StructuredLightScanner::startScan(PatternType patternType, int numFrames, qreal baseline)
{
    m_patternType = patternType;
    m_numFrames = qMax(1, numFrames);
    m_baseline = baseline;
    m_currentFrame = 0;
    m_scanning = true;
    m_frames.clear();
    m_result = ScanResult();
    m_result.description = QString("Structured light scan (%1 patterns)").arg(static_cast<int>(patternType));
    m_result.averageConfidence = 0.0f;
    
    emit scanStarted();
    
    // Capture the first frame immediately
    captureFrame();
}

void StructuredLightScanner::captureFrame()
{
    if (!m_scanning) {
        qWarning() << "Scanner not running";
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    if (m_currentFrame >= m_numFrames) {
        // All frames captured, finish reconstruction
        m_scanning = false;
        reconstructGeometry();
        emit scanCompleted(m_result);
        return;
    }
    
    // Simulate pattern projection and capture
    QImage pattern, captured;
    simulateCapture(m_patternType, pattern, captured);
    
    // Create frame record
    CapturedFrame frame;
    frame.pattern = pattern;
    frame.captured = captured;
    frame.timestamp = QDateTime::currentDateTime();
    
    // Add some synthetic "phase" based data for realism
    // In a real system, this would come from phase-shifting algorithms
    frame.valid = (m_currentFrame > 0);  // First frame is reference only
    
    m_frames.append(frame);
    
    // Capture actual image from viewport if available
    // For now, we use synthetic data
    
    // Emit signal about this frame
    emit frameCaptured(pattern, captured, frame.valid);
    
    // Advance to next frame
    m_currentFrame++;
    
    // Compute progress
    qreal progress = (static_cast<qreal>(m_currentFrame) / m_numFrames) * 100.0f;
    emit scanProgressed(m_result.points, progress);
    
    // Capture next frame after short delay (simulate projector exposure time)
    if (m_scanning && m_currentFrame < m_numFrames) {
        QTimer::singleShot(200, this, [this]() {
            captureFrame();
        });
    } else if (m_scanning) {
        // All frames captured, do final reconstruction
        m_scanning = false;
        reconstructGeometry();
        emit scanCompleted(m_result);
    }
}

void StructuredLightScanner::stopScan()
{
    QMutexLocker locker(&m_mutex);
    m_scanning = false;
    emit errorOccurred("Scan stopped by user");
}

// result() and frames() are defined inline in the header

// ----------------------------------------------------------------------------
// Internal: Simulate pattern projection and capture
// ----------------------------------------------------------------------------

void StructuredLightScanner::simulateCapture(PatternType type, QImage& outPattern, QImage& outCaptured)
{
    // Generate pattern image
    outPattern = generatePattern(type, QSize(640, 480));
    
    // Create a "captured" image by simulating what a camera would see
    // The pattern appears deformed based on object shape
    // For simulation, we'll create a synthetic deformed pattern
    
    outCaptured = outPattern.copy();  // Start with pattern
    
    // Add some synthetic "object deformation" - displace pixels based on fake 3D shape
    // This simulates the pattern being distorted by a real 3D object
    for (int y = 0; y < outCaptured.height(); ++y) {
        uchar* scanLine = outCaptured.scanLine(y);
        for (int x = 0; x < outCaptured.width(); ++x) {
            // Simple radial distortion simulation
            float dx = (x - outCaptured.width() / 2.0f) / outCaptured.width();
            float dy = (y - outCaptured.height() / 2.0f) / outCaptured.height();
            float r = qSqrt(dx * dx + dy * dy);
            
            // Distort based on radius - simulates 3D surface
            float displacement = r * 20.0f;  // Max 20 pixel displacement
            int nx = qBound(0, x + int(displacement * dx * 0.5), outCaptured.width() - 1);
            int ny = qBound(0, y + int(displacement * dy * 0.5), outCaptured.height() - 1);
            
            // Sample from original pattern at displaced position
            uchar original = qGray(outPattern.pixel(nx, ny));
            scanLine[x] = original;
        }
    }
}

// ----------------------------------------------------------------------------
// Internal: Generate pattern images of specified type
// ----------------------------------------------------------------------------

QImage StructuredLightScanner::generatePattern(PatternType type, const QSize& size)
{
    QImage pattern(size, QImage::Format_Grayscale8);
    pattern.fill(0);
    
    int w = size.width();
    int h = size.height();
    int cx = w / 2;
    int cy = h / 2;
    int r = qMin(w, h) / 3;
    
    for (int y = 0; y < h; ++y) {
        uchar* scanLine = pattern.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int val = 0;
            
            switch (type) {
                case PatternType::Grid: {
                    // Horizontal and vertical lines
                    if (x % 20 == 0 || y % 20 == 0) val = 255;
                    break;
                }
                case PatternType::Cross: {
                    // Cross shape
                    if (qAbs(x - cx) < 30 || qAbs(y - cy) < 30) val = 255;
                    break;
                }
                case PatternType::Circle: {
                    // Concentric circles
                    float dist = qSqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
                    if (dist > r && dist < r + 10) val = 255;
                    else if (dist > r + 30 && dist < r + 40) val = 255;
                    break;
                }
                case PatternType::RandomDots: {
                    // Random dot pattern
                    if (QRandomGenerator::global()->bounded(100) < 30) val = 255;
                    break;
                }
                case PatternType::PhaseShift1: {
                    // Single sinusoidal pattern
                    float phase = 2.0f * M_PI * (x / float(w));
                    val = qBound(0, int(128 + 128 * qCos(phase)), 255);
                    break;
                }
                case PatternType::PhaseShift3: {
                    // Three-step phase shifting
                    float phase1 = 2.0f * M_PI * (x / float(w));
                    float phase2 = 2.0f * M_PI * (x / float(w) + 2.0f / 3.0f);
                    float phase3 = 2.0f * M_PI * (x / float(w) + 4.0f / 3.0f);
                    
                    int val1 = qBound(0, int(128 + 128 * qCos(phase1)), 255);
                    int val2 = qBound(0, int(128 + 128 * qCos(phase2)), 255);
                    int val3 = qBound(0, int(128 + 128 * qCos(phase3)), 255);
                    
                    // For phase calculation, use weighted average
                    val = qBound(0, int((val1 + val2 * 2 + val3) / 4.0), 255);
                    break;
                }
            }
            
            scanLine[x] = static_cast<uchar>(val);
        }
    }
    
    return pattern;
}

// ----------------------------------------------------------------------------
// Internal: Triangulate 3D point from two camera views
// ----------------------------------------------------------------------------

bool StructuredLightScanner::triangulate(const QVector3D& ray1Orig, const QVector3D& ray1Dir,
                                          const QVector3D& ray2Orig, const QVector3D& ray2Dir,
                                          QVector3D& intersection, float& confidence)
{
    // Based on: https://en.wikipedia.org/wiki/Triangulation_(computer_vision)
    // Solve for intersection of two rays using least squares
    
    // Ray1: P1 + t1 * D1
    // Ray2: P2 + t2 * D2
    // At intersection: P1 + t1 * D1 = P2 + t2 * D2
    // => t1 * D1 - t2 * D2 = P2 - P1
    
    QVector3D w = ray2Orig - ray1Orig;
    
    // Build the 2x2 system: [D1.d1, -D2.d1; D1.d2, -D2.d2] * [t1; t2] = [w.d1; w.d2]
    // Actually using a more robust approach: minimize |(P1 + t1*D1) - (P2 + t2*D2)|^2
    
    float a = ray1Dir.x() * ray1Dir.x() + ray1Dir.y() * ray1Dir.y() + ray1Dir.z() * ray1Dir.z();  // |D1|^2 = 1 if normalized
    float b = -ray1Dir.x() * ray2Dir.x() - ray1Dir.y() * ray2Dir.y() - ray1Dir.z() * ray2Dir.z();  // -D1.D2
    float c = ray2Dir.x() * ray2Dir.x() + ray2Dir.y() * ray2Dir.y() + ray2Dir.z() * ray2Dir.z();  // |D2|^2 = 1 if normalized
    float d = ray1Dir.x() * w.x() + ray1Dir.y() * w.y() + ray1Dir.z() * w.z();  // D1.w
    float e = -ray2Dir.x() * w.x() - ray2Dir.y() * w.y() - ray2Dir.z() * w.z();  // -D2.w
    
    // Determinant
    float det = a * c - b * b;
    
    if (qFabs(det) < 0.0001f) {
        // Rays are parallel or nearly parallel
        confidence = 0.0f;
        return false;
    }
    
    // Solve for t1 and t2
    float t1 = (d * c - b * e) / det;
    float t2 = (a * e - b * d) / det;
    
    // Compute intersection point (using ray1)
    intersection = ray1Orig + t1 * ray1Dir;
    
    // Compute confidence based on angle between rays
    // Smaller angle = less reliable triangulation
    float cosAngle = QVector3D::dotProduct(ray1Dir, ray2Dir);
    float angle = qAcos(qBound(-1.0f, cosAngle, 1.0f));
    
    // Confidence: higher when rays are more orthogonal (angle near 90°)
    // Lower when rays are parallel or nearly so
    if (angle < 0.1f) {
        // Nearly parallel - poor triangulation
        confidence = 0.1f;
    } else if (angle > M_PI - 0.1f) {
        // Nearly opposite - also poor
        confidence = 0.1f;
    } else {
        // Good triangulation when angle ~ 90°
        confidence = qSqrt(qMax(0.0f, 1.0f - qFabs(cosAngle) / M_PI_2));  // 0 to 1
        // Actually: confidence = sin(angle) normalized
        confidence = qSin(angle);
    }
    
    // Additional confidence based on baseline/distance
    float distance = intersection.length();
    if (distance > 0) {
        // Confidence decreases with distance
        float distFactor = qMax(0.1f, 1.0f / qMax(1.0f, distance / 10.0f));
        confidence *= distFactor;
    }
    
    // Clamp confidence
    confidence = qBound(0.0f, confidence, 1.0f);
    
    return true;
}

// ----------------------------------------------------------------------------
// Internal: Reconstruct geometry from all captured frames
// ----------------------------------------------------------------------------

void StructuredLightScanner::reconstructGeometry()
{
    m_result.points.clear();
    m_result.cameraPositions.clear();
    
    if (m_frames.size() < 2) {
        m_result.notes = "Need at least 2 frames for triangulation";
        m_result.averageConfidence = 0.0f;
        return;
    }
    
    // For structured light, we assume the pattern projects known patterns
    // and the camera observes deformations
    // 
    // Simplified approach: 
    // - Use first frame as reference (no deformation = flat surface at distance)
    // - Subsequent frames have synthetic deformations based on imagined 3D shape
    // - Triangulate points from correspondences
    
    // For this simulation, we'll create a simple cylindrical object
    // and compute points based on the synthetic deformations
    
    qreal totalConfidence = 0.0f;
    int validPoints = 0;
    
    // Process consecutive frame pairs for triangulation
    for (int i = 0; i < m_frames.size() - 1; ++i) {
        const auto& frame1 = m_frames[i];
        const auto& frame2 = m_frames[i + 1];
        
        // Camera positions: simulate camera moving in an arc
        // In real system, these would be actual camera poses
        qreal angle1 = (static_cast<qreal>(i) / m_numFrames) * M_PI;
        qreal angle2 = angle1 + 0.5;  // Some offset
        
        // Projector position (same for all frames in this sim)
        QVector3D projPos = QVector3D(m_projectorDistance, 0, 0);
        
        // Camera positions in a circle around the object
        QVector3D cam1 = QVector3D(m_cameraDistance * qCos(angle1), 0, m_cameraDistance * qSin(angle1));
        QVector3D cam2 = QVector3D(m_cameraDistance * qCos(angle2), 0, m_cameraDistance * qSin(angle2));
        
        m_result.cameraPositions.append(cam1);
        m_result.cameraPositions.append(cam2);
        
        // For each pixel in the pattern, compute ray directions
        // and triangulate
        for (int py = 0; py < frame1.captured.height(); py += 4) {  // Sample every 4 pixels
            for (int px = 0; px < frame1.captured.width(); px += 4) {
                // Get pixel value (brightness = pattern modulation)
                uchar brightness1 = frame1.captured.pixel(px, py) & 0xFF;
                uchar brightness2 = frame2.captured.pixel(px, py) & 0xFF;
                
                // Only process significant pattern modulation
                if (qAbs(brightness1 - brightness2) > 30) {
                    // Compute ray from camera through pixel
                    // Camera model: pinhole, focal length assumed
                    float fx = 1.0f;  // focal length x
                    float fy = 1.0f;  // focal length y
                    float cx = frame1.captured.width() / 2.0f;  // principal point x
                    float cy = frame1.captured.height() / 2.0f;  // principal point y
                    
                    // Ray direction from camera through pixel (normalized)
                    QVector3D ray1Dir = QVector3D(
                        (px - cx) / fx,
                        (py - cy) / fy,
                        1.0f
                    ).normalized();
                    
                    QVector3D ray2Dir = QVector3D(
                        (px - cx) / fx,
                        (py - cy) / fy,
                        1.0f
                    ).normalized();
                    
                    // Triangulate
                    QVector3D intersection;
                    float confidence;
                    if (triangulate(cam1, ray1Dir, cam2, ray2Dir, intersection, confidence)) {
                        // Only add points with reasonable confidence
                        if (confidence > 0.3f) {
                            ReconstructedPoint pt;
                            pt.position = intersection;
                            pt.confidence = confidence;
                            pt.uv = QVector2D(px, py);
                            m_result.points.append(pt);
                            totalConfidence += confidence;
                            validPoints++;
                        }
                    }
                }
            }
        }
    }
    
    // Compute result statistics
    if (validPoints > 0) {
        m_result.averageConfidence = totalConfidence / validPoints;
    } else {
        m_result.averageConfidence = 0.0f;
    }
    
    // Estimate scanned volume (very rough: bounding box volume)
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
        
        m_result.scannedVolume = qMax(1.0f, (maxPt.x() - minPt.x()) * 
                                      (maxPt.y() - minPt.y()) * 
                                      (maxPt.z() - minPt.z()));
        
        m_result.description += QString(" - %1 points, avg confidence: %2%")
            .arg(validPoints)
            .arg(QString::number(m_result.averageConfidence * 100, 'f', 1));
    } else {
        m_result.description += " - no valid points reconstructed";
    }
    
    m_result.notes = "Structured light simulation - " + 
                     QString::number(m_frames.size()) + " frames captured";
}

// ----------------------------------------------------------------------------
// Register in QML - done in main.cpp
// ----------------------------------------------------------------------------

} // namespace structured_light
} // namespace ks