#include "AiSplineEditor.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QDebug>
#include <cmath>
#include <algorithm>

// ============================================================================
// File operations
// ============================================================================

AiSplineEditor::AiTrackData AiSplineEditor::loadTrack(const QString& trackPath) {
    AiTrackData data;
    data.trackName = QFileInfo(trackPath).fileName();

    // Load fast lane
    QString fastLanePath = trackPath + "/ai/fast_lane.ai";
    if (QFile::exists(fastLanePath)) {
        data.fastLane = loadAiFile(fastLanePath);
        data.fastLane.name = "fast_lane";
    }

    // Load pit lane
    QString pitLanePath = trackPath + "/ai/pit_lane.ai";
    if (QFile::exists(pitLanePath)) {
        data.pitLane = loadAiFile(pitLanePath);
        data.pitLane.name = "pit_lane";
    }

    // Load ideal line
    QString idealLinePath = trackPath + "/ai/ideal_line.ai";
    if (QFile::exists(idealLinePath)) {
        data.idealLine = loadAiFile(idealLinePath);
        data.idealLine.name = "ideal_line";
    }

    // Load borders
    QString leftBorderPath = trackPath + "/ai/side_l.csv";
    if (QFile::exists(leftBorderPath)) {
        data.leftBorder = loadCsvBorder(leftBorderPath);
        data.leftBorder.name = "left";
    }

    QString rightBorderPath = trackPath + "/ai/side_r.csv";
    if (QFile::exists(rightBorderPath)) {
        data.rightBorder = loadCsvBorder(rightBorderPath);
        data.rightBorder.name = "right";
    }

    return data;
}

bool AiSplineEditor::saveTrack(const AiTrackData& data, const QString& trackPath) {
    QDir().mkpath(trackPath + "/ai");

    // Save fast lane
    if (data.fastLane.isValid()) {
        if (!saveAiFile(data.fastLane, trackPath + "/ai/fast_lane.ai")) {
            return false;
        }
    }

    // Save pit lane
    if (data.pitLane.isValid()) {
        if (!saveAiFile(data.pitLane, trackPath + "/ai/pit_lane.ai")) {
            return false;
        }
    }

    // Save ideal line
    if (data.idealLine.isValid()) {
        if (!saveAiFile(data.idealLine, trackPath + "/ai/ideal_line.ai")) {
            return false;
        }
    }

    // Save borders
    if (data.leftBorder.isValid()) {
        if (!saveCsvBorder(data.leftBorder, trackPath + "/ai/side_l.csv")) {
            return false;
        }
    }

    if (data.rightBorder.isValid()) {
        if (!saveCsvBorder(data.rightBorder, trackPath + "/ai/side_r.csv")) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// AI spline file operations
// ============================================================================

AiSplineEditor::AiSpline AiSplineEditor::loadAiFile(const QString& filePath) {
    AiSpline spline;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return spline;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!parseAiBinary(stream, spline)) {
        // Try text format
        file.seek(0);
        QTextStream textStream(&file);
        int index = 0;

        while (!textStream.atEnd()) {
            QString line = textStream.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;

            AiSplinePoint point;
            if (parseCsvLine(line, point)) {
                point.distance = (spline.points.isEmpty()) ? 0.0f :
                    spline.points.last().distanceTo(point);
                spline.points.append(point);
                index++;
            }
        }
    }

    file.close();

    // Calculate cumulative distances
    float totalDist = 0.0f;
    for (int i = 0; i < spline.points.size(); ++i) {
        if (i > 0) {
            totalDist += spline.points[i-1].distanceTo(spline.points[i]);
        }
        spline.points[i].distance = totalDist;
    }
    spline.totalDistance = totalDist;

    return spline;
}

bool AiSplineEditor::saveAiFile(const AiSpline& spline, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    bool success = writeAiBinary(stream, spline);

    file.close();
    return success;
}

// ============================================================================
// CSV border operations
// ============================================================================

AiSplineEditor::AiBorder AiSplineEditor::loadCsvBorder(const QString& filePath) {
    AiBorder border;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return border;
    }

    QTextStream stream(&file);
    bool headerSkipped = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (!headerSkipped) {
            headerSkipped = true;
            if (line.startsWith('#') || line.contains(',')) {
                continue; // Skip header
            }
        }

        if (line.isEmpty() || line.startsWith('#')) continue;

        AiSplinePoint point;
        if (parseCsvLine(line, point)) {
            border.points.append(point);
        }
    }

    file.close();
    return border;
}

bool AiSplineEditor::saveCsvBorder(const AiBorder& border, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "# AI Border Line\n";
    stream << "# Generated by ksEditor\n";
    stream << "# X, Y, Z\n";

    for (const AiSplinePoint& point : border.points) {
        stream << formatCsvLine(point) << "\n";
    }

    file.close();
    return true;
}

// ============================================================================
// AI hints operations
// ============================================================================

QJsonObject AiSplineEditor::loadAiHints(const QString& trackPath) {
    QString hintsPath = trackPath + "/ai/ai_hints.ini";
    QFile file(hintsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QJsonObject();
    }

    QTextStream stream(&file);
    QJsonObject hints;
    QString currentSection;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();

            if (!currentSection.isEmpty()) {
                QJsonObject section = hints[currentSection].toObject();
                section[key] = value;
                hints[currentSection] = section;
            }
        }
    }

    file.close();
    return hints;
}

bool AiSplineEditor::saveAiHints(const QJsonObject& hints, const QString& trackPath) {
    QString hintsPath = trackPath + "/ai/ai_hints.ini";
    QFile file(hintsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "; AI Hints Configuration\n";
    stream << "; Generated by ksEditor\n\n";

    for (auto it = hints.begin(); it != hints.end(); ++it) {
        stream << "[" << it.key() << "]\n";
        QJsonObject section = it.value().toObject();
        for (auto sit = section.begin(); sit != section.end(); ++sit) {
            stream << sit.key() << "=" << sit.value().toString() << "\n";
        }
        stream << "\n";
    }

    file.close();
    return true;
}

// ============================================================================
// Spline manipulation
// ============================================================================

AiSplineEditor::AiSpline AiSplineEditor::smoothSpline(const AiSpline& input, int iterations) {
    AiSpline result = input;

    for (int iter = 0; iter < iterations; ++iter) {
        AiSpline smoothed;
        smoothed.name = result.name;
        smoothed.isClosed = result.isClosed;

        if (result.points.size() < 3) {
            return result;
        }

        smoothed.points.append(result.points.first());

        for (int i = 1; i < result.points.size() - 1; ++i) {
            const AiSplinePoint& prev = result.points[i-1];
            const AiSplinePoint& curr = result.points[i];
            const AiSplinePoint& next = result.points[i+1];

            AiSplinePoint smoothedPoint;
            smoothedPoint.x = (prev.x + curr.x * 2 + next.x) / 4.0f;
            smoothedPoint.y = (prev.y + curr.y * 2 + next.y) / 4.0f;
            smoothedPoint.z = (prev.z + curr.z * 2 + next.z) / 4.0f;
            smoothedPoint.curvature = curr.curvature;
            smoothedPoint.speed = curr.speed;

            smoothed.points.append(smoothedPoint);
        }

        smoothed.points.append(result.points.last());
        result = smoothed;
    }

    // Recalculate distances
    float totalDist = 0.0f;
    for (int i = 0; i < result.points.size(); ++i) {
        if (i > 0) {
            totalDist += result.points[i-1].distanceTo(result.points[i]);
        }
        result.points[i].distance = totalDist;
    }
    result.totalDistance = totalDist;

    return result;
}

AiSplineEditor::AiSpline AiSplineEditor::resampleSpline(const AiSpline& input, int targetPoints) {
    AiSpline result;
    result.name = input.name;
    result.isClosed = input.isClosed;

    if (input.points.size() < 2 || targetPoints < 2) {
        return input;
    }

    float totalLength = calculateTotalLength(input);
    float step = totalLength / (targetPoints - 1);

    for (int i = 0; i < targetPoints; ++i) {
        float distance = i * step;
        result.points.append(input.getPointAtDistance(distance));
    }

    // Recalculate distances
    float totalDist = 0.0f;
    for (int i = 0; i < result.points.size(); ++i) {
        if (i > 0) {
            totalDist += result.points[i-1].distanceTo(result.points[i]);
        }
        result.points[i].distance = totalDist;
    }
    result.totalDistance = totalDist;

    return result;
}

AiSplineEditor::AiSpline AiSplineEditor::subdivideSpline(const AiSpline& input, int subdivisions) {
    AiSpline result;
    result.name = input.name;
    result.isClosed = input.isClosed;

    if (input.points.size() < 2) {
        return input;
    }

    for (int i = 0; i < input.points.size() - 1; ++i) {
        const AiSplinePoint& start = input.points[i];
        const AiSplinePoint& end = input.points[i + 1];

        result.points.append(start);

        for (int j = 1; j < subdivisions; ++j) {
            float t = static_cast<float>(j) / subdivisions;
            result.points.append(interpolate(start, end, t));
        }
    }

    result.points.append(input.points.last());

    // Recalculate distances
    float totalDist = 0.0f;
    for (int i = 0; i < result.points.size(); ++i) {
        if (i > 0) {
            totalDist += result.points[i-1].distanceTo(result.points[i]);
        }
        result.points[i].distance = totalDist;
    }
    result.totalDistance = totalDist;

    return result;
}

AiSplineEditor::AiSpline AiSplineEditor::optimizeSpline(const AiSpline& input, float minDistance) {
    AiSpline result;
    result.name = input.name;
    result.isClosed = input.isClosed;

    if (input.points.isEmpty()) {
        return result;
    }

    result.points.append(input.points.first());

    for (int i = 1; i < input.points.size(); ++i) {
        float dist = result.points.last().distanceTo(input.points[i]);
        if (dist >= minDistance) {
            result.points.append(input.points[i]);
        }
    }

    // Recalculate distances
    float totalDist = 0.0f;
    for (int i = 0; i < result.points.size(); ++i) {
        if (i > 0) {
            totalDist += result.points[i-1].distanceTo(result.points[i]);
        }
        result.points[i].distance = totalDist;
    }
    result.totalDistance = totalDist;

    return result;
}

// ============================================================================
// Analysis
// ============================================================================

float AiSplineEditor::calculateTotalLength(const AiSpline& spline) {
    float length = 0.0f;
    for (int i = 1; i < spline.points.size(); ++i) {
        length += spline.points[i-1].distanceTo(spline.points[i]);
    }
    return length;
}

QVector<float> AiSplineEditor::calculateCurvatures(const AiSpline& spline) {
    QVector<float> curvatures;
    curvatures.reserve(spline.points.size());

    for (int i = 0; i < spline.points.size(); ++i) {
        curvatures.append(spline.getCurvatureAt(i));
    }

    return curvatures;
}

QVector<float> AiSplineEditor::calculateSpeeds(const AiSpline& spline, float maxSpeed) {
    QVector<float> speeds;
    speeds.reserve(spline.points.size());

    for (int i = 0; i < spline.points.size(); ++i) {
        float curvature = std::abs(spline.getCurvatureAt(i));
        float speed = maxSpeed / (1.0f + curvature * 0.1f);
        speeds.append(std::min(speed, maxSpeed));
    }

    return speeds;
}

QVector<float> AiSplineEditor::calculateDistances(const AiSpline& spline) {
    QVector<float> distances;
    distances.reserve(spline.points.size());

    float totalDist = 0.0f;
    for (int i = 0; i < spline.points.size(); ++i) {
        if (i > 0) {
            totalDist += spline.points[i-1].distanceTo(spline.points[i]);
        }
        distances.append(totalDist);
    }

    return distances;
}

float AiSplineEditor::getMaxCurvature(const AiSpline& spline) {
    float maxCurvature = 0.0f;
    for (const AiSplinePoint& point : spline.points) {
        maxCurvature = std::max(maxCurvature, std::abs(point.curvature));
    }
    return maxCurvature;
}

float AiSplineEditor::getMaxSpeed(const AiSpline& spline) {
    float maxSpeed = 0.0f;
    for (const AiSplinePoint& point : spline.points) {
        maxSpeed = std::max(maxSpeed, point.speed);
    }
    return maxSpeed;
}

// ============================================================================
// Validation
// ============================================================================

bool AiSplineEditor::validateSpline(const AiSpline& spline, QString* error) {
    if (spline.points.size() < 2) {
        if (error) *error = "Spline has fewer than 2 points";
        return false;
    }

    // Check for duplicate consecutive points
    for (int i = 1; i < spline.points.size(); ++i) {
        if (spline.points[i-1].distanceTo(spline.points[i]) < 0.001f) {
            if (error) *error = "Duplicate consecutive points at index " + QString::number(i);
            return false;
        }
    }

    return true;
}

bool AiSplineEditor::validateBorders(const AiBorder& left, const AiBorder& right, QString* error) {
    if (!left.isValid()) {
        if (error) *error = "Left border is empty";
        return false;
    }

    if (!right.isValid()) {
        if (error) *error = "Right border is empty";
        return false;
    }

    if (left.points.size() != right.points.size()) {
        if (error) *error = "Border point count mismatch (left: " +
            QString::number(left.points.size()) + ", right: " +
            QString::number(right.points.size()) + ")";
        return false;
    }

    return true;
}

bool AiSplineEditor::validateTrackData(const AiTrackData& data, QString* error) {
    if (!data.hasFastLane()) {
        if (error) *error = "No fast lane data";
        return false;
    }

    if (!validateSpline(data.fastLane, error)) {
        return false;
    }

    return true;
}

// ============================================================================
// Utility
// ============================================================================

AiSplineEditor::AiSplinePoint AiSplineEditor::interpolate(const AiSplinePoint& a, const AiSplinePoint& b, float t) {
    AiSplinePoint result;
    result.x = a.x + (b.x - a.x) * t;
    result.y = a.y + (b.y - a.y) * t;
    result.z = a.z + (b.z - a.z) * t;
    result.curvature = a.curvature + (b.curvature - a.curvature) * t;
    result.speed = a.speed + (b.speed - a.speed) * t;
    return result;
}

AiSplineEditor::AiSplinePoint AiSplineEditor::lerp(const AiSplinePoint& a, const AiSplinePoint& b, float t) {
    return interpolate(a, b, t);
}

float AiSplineEditor::cross2D(const AiSplinePoint& a, const AiSplinePoint& b, const AiSplinePoint& c) {
    return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
}

// ============================================================================
// Private helpers
// ============================================================================

bool AiSplineEditor::parseAiBinary(QDataStream& stream, AiSpline& spline) {
    // Check for AC AI spline binary format
    quint32 magic = 0;
    stream >> magic;

    if (magic != 0x00414900) { // "\0AI"
        stream.device()->seek(0);
        return false;
    }

    quint32 version = 0;
    stream >> version;

    quint32 pointCount = 0;
    stream >> pointCount;

    if (pointCount > 100000) { // Sanity check
        stream.device()->seek(0);
        return false;
    }

    spline.points.reserve(pointCount);

    for (quint32 i = 0; i < pointCount; ++i) {
        AiSplinePoint point;
        stream >> point.x >> point.y >> point.z;
        stream >> point.curvature >> point.speed;
        spline.points.append(point);
    }

    return true;
}

bool AiSplineEditor::writeAiBinary(QDataStream& stream, const AiSpline& spline) {
    stream << quint32(0x00414900); // "\0AI"
    stream << quint32(1); // version
    stream << quint32(spline.points.size());

    for (const AiSplinePoint& point : spline.points) {
        stream << point.x << point.y << point.z;
        stream << point.curvature << point.speed;
    }

    return true;
}

bool AiSplineEditor::parseCsvLine(const QString& line, AiSplinePoint& point) {
    QStringList parts = line.split(',');
    if (parts.size() < 3) return false;

    bool ok;
    point.x = parts[0].trimmed().toFloat(&ok);
    if (!ok) return false;

    point.y = parts[1].trimmed().toFloat(&ok);
    if (!ok) return false;

    point.z = parts[2].trimmed().toFloat(&ok);
    if (!ok) return false;

    if (parts.size() >= 4) {
        point.curvature = parts[3].trimmed().toFloat(&ok);
    }
    if (parts.size() >= 5) {
        point.speed = parts[4].trimmed().toFloat(&ok);
    }

    return true;
}

QString AiSplineEditor::formatCsvLine(const AiSplinePoint& point) {
    return QString("%1,%2,%3,%4,%5")
        .arg(point.x, 0, 'f', 6)
        .arg(point.y, 0, 'f', 6)
        .arg(point.z, 0, 'f', 6)
        .arg(point.curvature, 0, 'f', 4)
        .arg(point.speed, 0, 'f', 2);
}

// ============================================================================
// AiSpline class methods
// ============================================================================

float AiSplineEditor::AiSpline::getLength() const {
    return calculateTotalLength(*this);
}

AiSplineEditor::AiSplinePoint AiSplineEditor::AiSpline::getPointAtDistance(float distance) const {
    if (points.isEmpty()) return AiSplinePoint();
    if (points.size() == 1) return points.first();

    float accumulated = 0.0f;
    for (int i = 1; i < points.size(); ++i) {
        float segLen = points[i-1].distanceTo(points[i]);
        if (accumulated + segLen >= distance) {
            float t = (distance - accumulated) / segLen;
            return interpolate(points[i-1], points[i], t);
        }
        accumulated += segLen;
    }

    return points.last();
}

float AiSplineEditor::AiSpline::getCurvatureAt(int index) const {
    if (index < 0 || index >= points.size()) return 0.0f;
    return points[index].curvature;
}

// ============================================================================
// AiSplineManager implementation
// ============================================================================

AiSplineManager::AiSplineManager(const QString& trackPath)
    : m_trackPath(trackPath) {
}

bool AiSplineManager::load() {
    m_data = AiSplineEditor::loadTrack(m_trackPath);
    return m_data.hasFastLane();
}

bool AiSplineManager::save() {
    return AiSplineEditor::saveTrack(m_data, m_trackPath);
}

float AiSplineManager::getFastLaneLength() const {
    return m_data.fastLane.getLength();
}

bool AiSplineManager::smoothFastLane(int iterations) {
    if (!m_data.hasFastLane()) return false;
    m_data.fastLane = AiSplineEditor::smoothSpline(m_data.fastLane, iterations);
    return true;
}

bool AiSplineManager::resampleFastLane(int targetPoints) {
    if (!m_data.hasFastLane()) return false;
    m_data.fastLane = AiSplineEditor::resampleSpline(m_data.fastLane, targetPoints);
    return true;
}

bool AiSplineManager::generateBorders(float width) {
    if (!m_data.hasFastLane()) return false;

    using AiSplinePoint = AiSplineEditor::AiSplinePoint;

    m_data.leftBorder.name = "left";
    m_data.rightBorder.name = "right";
    m_data.leftBorder.points.clear();
    m_data.rightBorder.points.clear();

    for (int i = 0; i < m_data.fastLane.points.size(); ++i) {
        const AiSplinePoint& center = m_data.fastLane.points[i];

        // Calculate perpendicular direction
        AiSplinePoint prev, next;
        if (i > 0) prev = m_data.fastLane.points[i-1];
        else prev = center;
        if (i < m_data.fastLane.points.size() - 1) next = m_data.fastLane.points[i+1];
        else next = center;

        float dx = next.x - prev.x;
        float dz = next.z - prev.z;
        float len = std::sqrt(dx*dx + dz*dz);

        if (len > 0.001f) {
            // Perpendicular vector (rotate 90 degrees)
            float px = -dz / len * width * 0.5f;
            float pz = dx / len * width * 0.5f;

            AiSplinePoint leftPoint = center;
            leftPoint.x += px;
            leftPoint.z += pz;
            m_data.leftBorder.points.append(leftPoint);

            AiSplinePoint rightPoint = center;
            rightPoint.x -= px;
            rightPoint.z -= pz;
            m_data.rightBorder.points.append(rightPoint);
        }
    }

    return true;
}

bool AiSplineManager::validate(QString* error) const {
    return AiSplineEditor::validateTrackData(m_data, error);
}
