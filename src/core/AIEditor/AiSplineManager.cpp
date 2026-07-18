#include "AiSplineEditor.h"
#include <QDir>
#include <QFileInfo>
#include <cmath>

// ============================================================================
// AiSplineManager
// ============================================================================

AiSplineManager::AiSplineManager(QObject* parent)
    : QObject(parent)
{
}

bool AiSplineManager::loadSpline(const QString& filePath)
{
    m_currentSplinePath = filePath;
    QFileInfo fi(filePath);
    m_trackPath = fi.absolutePath();
    
    m_data = AiSplineEditor::loadTrack(m_trackPath);

    if (!m_data.hasFastLane()) {
        return false;
    }

    if (m_data.fastLane.totalDistance == 0.0f) {
        m_data.fastLane.totalDistance = AiSplineEditor::calculateTotalLength(m_data.fastLane);
    }

    return true;
}

bool AiSplineManager::saveSpline(const QString& filePath)
{
    if (filePath.isEmpty()) return false;
    m_currentSplinePath = filePath;
    QFileInfo fi(filePath);
    m_trackPath = fi.absolutePath();
    return AiSplineEditor::saveTrack(m_data, m_trackPath);
}

float AiSplineManager::getFastLaneLength() const
{
    return m_data.fastLane.totalDistance;
}

QString AiSplineManager::getSplineInfo() const
{
    if (!hasSpline()) return "No spline loaded";
    
    QString info;
    info += QString("Track: %1\n").arg(m_data.trackName);
    info += QString("Fast lane: %1 points, %.1f m\n").arg(m_data.fastLane.points.size()).arg(m_data.fastLane.totalDistance);
    info += QString("Pit lane: %1 points\n").arg(m_data.pitLane.points.size());
    info += QString("Ideal line: %1 points\n").arg(m_data.idealLine.points.size());
    info += QString("Left border: %1 points\n").arg(m_data.leftBorder.points.size());
    info += QString("Right border: %1 points\n").arg(m_data.rightBorder.points.size());
    return info;
}

bool AiSplineManager::smoothSpline(int iterations, int targetPoints)
{
    if (!m_data.hasFastLane()) return false;
    
    m_data.fastLane = AiSplineEditor::smoothSpline(m_data.fastLane, iterations);
    if (targetPoints > 0) {
        m_data.fastLane = AiSplineEditor::resampleSpline(m_data.fastLane, targetPoints);
    }
    m_data.fastLane.totalDistance = AiSplineEditor::calculateTotalLength(m_data.fastLane);
    return true;
}

bool AiSplineManager::resampleSpline(int targetPoints)
{
    if (!m_data.hasFastLane()) return false;
    m_data.fastLane = AiSplineEditor::resampleSpline(m_data.fastLane, targetPoints);
    m_data.fastLane.totalDistance = AiSplineEditor::calculateTotalLength(m_data.fastLane);
    return true;
}

bool AiSplineManager::smoothFastLane(int iterations)
{
    return smoothSpline(iterations, -1);
}

bool AiSplineManager::resampleFastLane(int targetPoints)
{
    return resampleSpline(targetPoints);
}

bool AiSplineManager::generateBorders(float width)
{
    if (!m_data.hasFastLane()) return false;

    const auto& pts = m_data.fastLane.points;
    if (pts.size() < 2) return false;

    AiSplineEditor::AiBorder left, right;
    left.name = "left";
    right.name = "right";

    for (int i = 0; i < pts.size(); ++i) {
        int prev = (i > 0) ? i - 1 : (m_data.fastLane.isClosed ? pts.size() - 1 : 0);
        int next = (i < pts.size() - 1) ? i + 1 : (m_data.fastLane.isClosed ? 0 : pts.size() - 1);

        float dx = pts[next].x - pts[prev].x;
        float dz = pts[next].z - pts[prev].z;
        float len = std::sqrt(dx * dx + dz * dz);
        if (len < 0.0001f) continue;

        float nx = -dz / len;
        float nz = dx / len;

        AiSplineEditor::AiSplinePoint lpt, rpt;
        lpt.x = pts[i].x + nx * width;
        lpt.z = pts[i].z + nz * width;
        lpt.y = pts[i].y;
        left.points.append(lpt);

        rpt.x = pts[i].x - nx * width;
        rpt.z = pts[i].z - nz * width;
        rpt.y = pts[i].y;
        right.points.append(rpt);
    }

    m_data.leftBorder = left;
    m_data.rightBorder = right;
    return true;
}

bool AiSplineManager::validate(QString* error) const
{
    return AiSplineEditor::validateTrackData(m_data, error);
}