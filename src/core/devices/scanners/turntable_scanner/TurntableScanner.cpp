#include "TurntableScanner.h"
#include "../../../../modules/modellingEditor/VulkanViewportItem.h"
#include <QMatrix4x4>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <QDir>
#include <QTimer>

namespace ks {
namespace turntable_scanner {

TurntableScanner::TurntableScanner(QObject* parent)
    : QObject(parent)
    , m_viewport(nullptr)
{
    m_progress.scanning = false;
}

TurntableScanner::~TurntableScanner()
{
}

void TurntableScanner::startScan(int numPositions, qreal turntableRadius, qreal cameraDistance,
                                  qreal overlap)
{
    if (numPositions < 3) {
        qWarning() << "Need at least 3 positions for a scan";
        return;
    }
    
    m_numPositions = qMax(3, numPositions);
    m_turntableRadius = turntableRadius;
    m_cameraDistance = cameraDistance;
    m_overlap = overlap;
    m_currentIndex = 0;
    
    // Compute all camera poses around the turntable
    computePoses(m_numPositions);
    
    // Reset capture state
    m_poses.clear();
    m_images.clear();
    m_progress = ScanProgress();
    m_progress.scanning = true;
    m_progress.totalImages = m_numPositions;
    m_progress.currentStep = "Position 1/" + QString::number(m_numPositions);
    m_progress.progressPercentage = 0.0f;
    
    emit scanStarted();
    emit scanProgressed(m_progress);
    emit stepChanged(0);
    
    // Capture first image
    captureCurrentImage();
}

void TurntableScanner::stopScan()
{
    m_progress.scanning = false;
    m_progress.currentStep = "Scan stopped";
    m_progress.progressPercentage = 100.0f;
    emit scanProgressed(m_progress);
    emit scanCompleted("Scan cancelled");
}

void TurntableScanner::captureCurrentImage()
{
    if (!m_viewport || m_currentIndex >= m_poses.size()) {
        qWarning() << "No viewport or invalid index";
        return;
    }
    
    // Capture from viewport
    captureFromViewportAtPose(m_poses[m_currentIndex]);
}

void TurntableScanner::advancePosition()
{
    if (!m_progress.scanning || m_currentIndex >= m_poses.size() - 1) {
        return;
    }
    
    m_currentIndex++;
    
    // Update progress
    m_progress.capturedImages = m_currentIndex + 1;
    m_progress.progressPercentage = 
        (static_cast<float>(m_currentIndex + 1) / m_numPositions) * 100.0f;
    
    QString stepName = "Position " + QString::number(m_currentIndex + 1) + "/" + 
                       QString::number(m_numPositions);
    m_progress.currentStep = stepName;
    
    emit stepChanged(m_currentIndex);
    emit scanProgressed(m_progress);
    
    // Capture image at new position
    captureCurrentImage();
}

QString TurntableScanner::exportForPhotogrammetry(const QString& directory)
{
    QString result;
    QDir dir(directory);
    if (!dir.mkpath(".")) {
        return "Failed to create directory: " + directory;
    }
    
    // Create COLMAP-compatible images.txt
    QString imagesFile = dir.filePath("images.txt");
    QFile f_images(imagesFile);
    if (f_images.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f_images);
        out << "# Turntable scanner images file\n";
        out << "# Format: image_name, qw, qx, qy, qz, tx, ty, tz, camera_id, name\n";
        
        for (int i = 0; i < m_images.size(); ++i) {
            if (m_images[i].isNull()) continue;
            
            const auto& pose = m_poses[i];
            out << "image_" << i << ".png, "
                << pose.cameraPosition.x() << " " << pose.cameraPosition.y() << " " 
                << pose.cameraPosition.z() << " , " // simplified: position as euler-like
                << 1 << ", image_" << i << ".png\n";
        }
        f_images.close();
        result += "Wrote " + QString::number(m_images.size()) + " images to images.txt\n";
    }
    
    // Create a simple poses file
    QString posesFile = dir.filePath("turntable_poses.json");
    QFile f_poses(posesFile);
    if (f_poses.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f_poses);
        out << "{\n";
        out << "  \"turntableRadius\": " << m_turntableRadius << ",\n";
        out << "  \"cameraDistance\": " << m_cameraDistance << ",\n";
        out << "  \"cameraHeight\": " << m_cameraHeight << ",\n";
        out << "  \"numPositions\": " << m_poses.size() << ",\n";
        out << "  \"positions\": [\n";
        
        for (int i = 0; i < m_poses.size(); ++i) {
            const auto& pose = m_poses[i];
            out << "    {\"angle\": " << pose.turntableAngle 
                << ", \"cameraPos\": [" << pose.cameraPosition.x() << ", "
                << pose.cameraPosition.y() << ", " << pose.cameraPosition.z() << "]";
            if (i < m_poses.size() - 1) out << "}";
            else out << "}";
            if (i < m_poses.size() - 1) out << ",";
            out << "\n";
        }
        out << "  ]\n";
        out << "}\n";
        f_poses.close();
        result += "Wrote turntable_poses.json\n";
    }
    
    emit scanCompleted(result);
    return result;
}

void TurntableScanner::computePoses(int numPositions)
{
    m_poses.clear();
    m_poses.reserve(numPositions);
    
    for (int i = 0; i < numPositions; ++i) {
        TurntablePose pose;
        pose.turntableAngle = (360.0 * i) / numPositions;
        pose.turntableRadius = m_turntableRadius;
        pose.cameraDistance = m_cameraDistance;
        pose.cameraHeight = m_cameraHeight;
        
        // Calculate camera position on circle around turntable center
        // Assuming Y-up turntable, camera orbits in XZ plane
        float angleRad = pose.turntableAngle * M_PI / 180.0f;
        
        // Camera position: radius distance from center, at angle
        pose.cameraPosition = QVector3D(
            m_turntableRadius * qSin(angleRad),
            m_cameraHeight,
            m_turntableRadius * qCos(angleRad)
        );
        
        // Camera looks at turntable center
        pose.cameraTarget = QVector3D(0, 0, 0);
        pose.objectPosition = QVector3D(0, 0, 0);
        
        m_poses.append(pose);
    }
}

TurntablePose TurntableScanner::computePoseAtIndex(int index) const
{
    if (index < 0 || index >= m_poses.size()) {
        return TurntablePose();
    }
    return m_poses[index];
}

void TurntableScanner::captureFromViewportAtPose(const TurntablePose& pose)
{
    if (!m_viewport) {
        qWarning() << "No viewport set";
        return;
    }
    
    // Trigger viewport update to capture the current frame
    m_viewport->update();
    QTimer::singleShot(100, this, [this, pose]() {
        // Create a blank image placeholder for now
        // TODO: implement proper framebuffer readback via VulkanViewportRenderer
        QImage img(640, 480, QImage::Format_RGB32);
        img.fill(Qt::darkGray);
        
        // Save the image
        QString filename = "scan_image_" + QString::number(m_currentIndex) + ".png";
        bool saved = img.save(filename);
        Q_UNUSED(saved);
        
        // Update progress
        m_images.append(img);
        m_progress.capturedImages = m_currentIndex + 1;
        m_progress.progressPercentage = 
            (static_cast<float>(m_currentIndex + 1) / m_numPositions) * 100.0f;
        
        QString stepName = "Position " + QString::number(m_currentIndex + 1) + "/" + 
                           QString::number(m_numPositions);
        m_progress.currentStep = stepName;
        
        emit imageCaptured(img, pose);
        emit scanProgressed(m_progress);
        emit stepChanged(m_currentIndex);
        
        // Auto-advance if not last
        if (m_currentIndex < m_numPositions - 1) {
            QTimer::singleShot(200, this, [this]() {
                advancePosition();
            });
        } else {
            // Scan complete
            emit scanCompleted("Scan complete - " + 
                QString::number(m_images.size()) + " images captured");
            m_progress.scanning = false;
            emit scanProgressed(m_progress);
        }
    });
}

QString TurntableScanner::exportImagesTxt() const
{
    QString result;
    // Write COLMAP-style images file
    // ... implementation similar to photogrammetry export
    return result;
}

QString TurntableScanner::exportPosesTxt() const
{
    QString result;
    // Write pose information for external software
    return result;
}

} // namespace turntable_scanner
} // namespace ks