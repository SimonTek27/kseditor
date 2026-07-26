#include "KNHFormat.h"

#include <QFile>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDebug>
#include <cmath>
#include <QDir>

namespace ks {

// ============================================================================
// KNHRacingLine implementation
// ============================================================================

void KNHRacingLine::clear() {
    trackName.clear();
    trackConfig.clear();
    header = KNHHeader();
    waypoints.clear();
    sectors.clear();
    cameraSplines.clear();
    cameraConfigs.clear();
}

bool KNHRacingLine::isEmpty() const {
    return waypoints.isEmpty();
}

int KNHRacingLine::totalWaypoints() const {
    return waypoints.size();
}

float KNHRacingLine::totalLength() const {
    float len = 0.0f;
    for (int i = 1; i < waypoints.size(); ++i) {
        len += (waypoints[i].position - waypoints[i-1].position).length();
    }
    return len;
}

float KNHRacingLine::totalTime() const {
    float time = 0.0f;
    for (int i = 1; i < waypoints.size(); ++i) {
        float dist = (waypoints[i].position - waypoints[i-1].position).length();
        float speed = (waypoints[i].targetSpeed + waypoints[i-1].targetSpeed) * 0.5f / 3.6f; // m/s
        if (speed > 0) time += dist / speed;
    }
    return time;
}

QVector3D KNHRacingLine::getPositionAt(float distance) const {
    if (waypoints.size() < 2) return QVector3D();
    float traveled = 0.0f;
    for (int i = 1; i < waypoints.size(); ++i) {
        float segLen = (waypoints[i].position - waypoints[i-1].position).length();
        if (traveled + segLen >= distance) {
            float t = (distance - traveled) / segLen;
            return waypoints[i-1].position + (waypoints[i].position - waypoints[i-1].position) * t;
        }
        traveled += segLen;
    }
    return waypoints.last().position;
}

float KNHRacingLine::getSpeedAt(float distance) const {
    if (waypoints.size() < 2) return 0.0f;
    float traveled = 0.0f;
    for (int i = 1; i < waypoints.size(); ++i) {
        float segLen = (waypoints[i].position - waypoints[i-1].position).length();
        if (traveled + segLen >= distance) {
            float t = (distance - traveled) / segLen;
            return waypoints[i-1].targetSpeed + (waypoints[i].targetSpeed - waypoints[i-1].targetSpeed) * t;
        }
        traveled += segLen;
    }
    return waypoints.last().targetSpeed;
}

int KNHRacingLine::getSectorAt(float distance) const {
    float traveled = 0.0f;
    for (int i = 1; i < waypoints.size(); ++i) {
        float segLen = (waypoints[i].position - waypoints[i-1].position).length();
        traveled += segLen;
        if (traveled >= distance) {
            return waypoints[i].sector;
        }
    }
    return waypoints.isEmpty() ? 0 : waypoints.last().sector;
}

float KNHRacingLine::estimateLapTime() const {
    return totalTime();
}

QVector<int> KNHRacingLine::findBrakingZones(float threshold) const {
    QVector<int> zones;
    for (int i = 1; i < waypoints.size(); ++i) {
        float speedDiff = waypoints[i-1].targetSpeed - waypoints[i].targetSpeed;
        if (speedDiff > threshold * waypoints[i-1].targetSpeed) {
            zones.append(i);
        }
    }
    return zones;
}

QVector<int> KNHRacingLine::findApexes() const {
    QVector<int> apexes;
    for (int i = 1; i < waypoints.size() - 1; ++i) {
        if (waypoints[i].isApexZone || waypoints[i].isCorner) {
            apexes.append(i);
        }
    }
    return apexes;
}

bool KNHRacingLine::validate(QString& error) const {
    if (waypoints.size() < 3) {
        error = "Need at least 3 waypoints";
        return false;
    }
    if (trackName.isEmpty()) {
        error = "Track name is empty";
        return false;
    }
    // Check for duplicate waypoints
    for (int i = 1; i < waypoints.size(); ++i) {
        float dist = (waypoints[i].position - waypoints[i-1].position).length();
        if (dist < 0.1f) {
            error = QString("Waypoints %1 and %2 are too close (%.2fm)").arg(i-1).arg(i).arg(dist);
            return false;
        }
    }
    return true;
}

// ============================================================================
// KNHReader implementation
// ============================================================================

bool KNHReader::read(const QString& path, KNHRacingLine& line) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot open file: %1").arg(path);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    if (!readHeader(stream, line.header)) return false;
    if (!readWaypoints(stream, line)) return false;
    if (!readSectors(stream, line)) return false;
    if (!readCameraSplines(stream, line)) return false;
    if (!readCameraConfigs(stream, line)) return false;

    line.trackName = QFileInfo(path).baseName();
    line.trackConfig = QFileInfo(path).completeSuffix(); // or parse from path

    file.close();
    return true;
}

bool KNHReader::readHeader(QDataStream& stream, KNHHeader& header) {
    char magic[4];
    stream.readRawData(magic, 4);

    if (QByteArray(magic, 3) != "KNH") {
        m_error = "Invalid KNH file format";
        return false;
    }

    stream >> header.version;
    stream >> header.flags;
    stream >> header.waypointCount;
    stream >> header.sectorCount;
    stream >> header.cameraSplineCount;
    stream >> header.cameraConfigCount;

    return true;
}

bool KNHReader::readWaypoints(QDataStream& stream, KNHRacingLine& line) {
    quint32 count;
    stream >> count;
    line.waypoints.resize(count);

    for (quint32 i = 0; i < count; ++i) {
        KNHWaypoint& wp = line.waypoints[i];
        stream >> wp.position;
        stream >> wp.tangent;
        stream >> wp.curvature;
        stream >> wp.width;
        stream >> wp.preferredRadius;
        stream >> wp.turnIn;
        stream >> wp.apex;
        stream >> wp.brakePoint;
        stream >> wp.throttlePoint;
        stream >> wp.targetSpeed;
        stream >> wp.gear;
        stream >> wp.sector;
        stream >> wp.isCorner;
        stream >> wp.isStraight;
        stream >> wp.isBrakingZone;
        stream >> wp.isApexZone;
    }
    return true;
}

bool KNHReader::readSectors(QDataStream& stream, KNHRacingLine& line) {
    quint32 count;
    stream >> count;
    line.sectors.resize(count);

    for (quint32 i = 0; i < count; ++i) {
        KNHSector& sec = line.sectors[i];
        stream >> sec.index;
        stream >> sec.startPos;
        stream >> sec.endPos;
        stream >> sec.length;
        stream >> sec.startWaypoint;
        stream >> sec.endWaypoint;
    }
    return true;
}

bool KNHReader::readCameraSplines(QDataStream& stream, KNHRacingLine& line) {
    quint32 count;
    stream >> count;
    line.cameraSplines.resize(count);

    for (quint32 i = 0; i < count; ++i) {
        KNHCameraSpline& spline = line.cameraSplines[i];
        stream >> spline.name;
        stream >> spline.degree;
        stream >> spline.cyclic;

        quint32 ptCount;
        stream >> ptCount;
        spline.controlPoints.resize(ptCount);
        for (quint32 j = 0; j < ptCount; ++j) {
            stream >> spline.controlPoints[j];
        }

        quint32 knotCount;
        stream >> knotCount;
        spline.knots.resize(knotCount);
        for (quint32 j = 0; j < knotCount; ++j) {
            stream >> spline.knots[j];
        }
    }
    return true;
}

bool KNHReader::readCameraConfigs(QDataStream& stream, KNHRacingLine& line) {
    quint32 count;
    stream >> count;
    line.cameraConfigs.resize(count);

    for (quint32 i = 0; i < count; ++i) {
        KNHCameraSplineConfig& cfg = line.cameraConfigs[i];
        stream >> cfg.fov;
        stream >> cfg.nearPlane;
        stream >> cfg.farPlane;
        stream >> cfg.exposure;
        stream >> cfg.minExposure;
        stream >> cfg.maxExposure;
        stream >> cfg.dofFocus;
        stream >> cfg.dofFactor;
        stream >> cfg.dofRange;
        stream >> cfg.dofManual;
        stream >> cfg.shadowSplits[0];
        stream >> cfg.shadowSplits[1];
        stream >> cfg.shadowSplits[2];
        stream >> cfg.inPoint;
        stream >> cfg.outPoint;
        stream >> cfg.isFixed;
        stream >> cfg.splineRotation;
        stream >> cfg.splineAnimationLength;
    }
    return true;
}

// ============================================================================
// KNHWriter implementation
// ============================================================================

bool KNHWriter::write(const QString& path, const KNHRacingLine& line) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot create file: %1").arg(path);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    KNHHeader header = line.header;
    header.waypointCount = line.waypoints.size();
    header.sectorCount = line.sectors.size();
    header.cameraSplineCount = line.cameraSplines.size();
    header.cameraConfigCount = line.cameraConfigs.size();

    writeHeader(stream, header);
    writeWaypoints(stream, line);
    writeSectors(stream, line);
    writeCameraSplines(stream, line);
    writeCameraConfigs(stream, line);

    file.close();
    return true;
}

bool KNHWriter::writeBinary(const QString& path, const KNHRacingLine& line) {
    return write(path, line);
}

bool KNHWriter::writeJson(const QString& path, const KNHRacingLine& line) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot create file: %1").arg(path);
        return false;
    }

    QJsonObject root;
    root["trackName"] = line.trackName;
    root["trackConfig"] = line.trackConfig;
    root["waypointCount"] = line.waypoints.size();
    root["sectorCount"] = line.sectors.size();

    QJsonArray wpArray;
    for (const auto& wp : line.waypoints) {
        QJsonObject o;
        o["pos"] = QJsonArray{QJsonValue(wp.position.x()), QJsonValue(wp.position.y()), QJsonValue(wp.position.z())};
        o["tangent"] = QJsonArray{QJsonValue(wp.tangent.x()), QJsonValue(wp.tangent.y()), QJsonValue(wp.tangent.z())};
        o["speed"] = wp.targetSpeed;
        o["gear"] = wp.gear;
        o["sector"] = wp.sector;
        o["isCorner"] = wp.isCorner;
        o["isBraking"] = wp.isBrakingZone;
        o["isApex"] = wp.isApexZone;
        wpArray.append(o);
    }
    root["waypoints"] = wpArray;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void KNHWriter::writeHeader(QDataStream& stream, const KNHHeader& header) {
    stream.writeRawData("KNH", 4);
    stream << header.version;
    stream << header.flags;
    stream << header.waypointCount;
    stream << header.sectorCount;
    stream << header.cameraSplineCount;
    stream << header.cameraConfigCount;
}

void KNHWriter::writeWaypoints(QDataStream& stream, const KNHRacingLine& line) {
    stream << quint32(line.waypoints.size());
    for (const auto& wp : line.waypoints) {
        stream << wp.position;
        stream << wp.tangent;
        stream << wp.curvature;
        stream << wp.width;
        stream << wp.preferredRadius;
        stream << wp.turnIn;
        stream << wp.apex;
        stream << wp.brakePoint;
        stream << wp.throttlePoint;
        stream << wp.targetSpeed;
        stream << wp.gear;
        stream << wp.sector;
        stream << wp.isCorner;
        stream << wp.isStraight;
        stream << wp.isBrakingZone;
        stream << wp.isApexZone;
    }
}

void KNHWriter::writeSectors(QDataStream& stream, const KNHRacingLine& line) {
    stream << quint32(line.sectors.size());
    for (const auto& sec : line.sectors) {
        stream << sec.index;
        stream << sec.startPos;
        stream << sec.endPos;
        stream << sec.length;
        stream << sec.startWaypoint;
        stream << sec.endWaypoint;
    }
}

void KNHWriter::writeCameraSplines(QDataStream& stream, const KNHRacingLine& line) {
    stream << quint32(line.cameraSplines.size());
    for (const auto& spline : line.cameraSplines) {
        stream << spline.name;
        stream << spline.degree;
        stream << spline.cyclic;
        stream << quint32(spline.controlPoints.size());
        for (const auto& pt : spline.controlPoints) stream << pt;
        stream << quint32(spline.knots.size());
        for (float k : spline.knots) stream << k;
    }
}

void KNHWriter::writeCameraConfigs(QDataStream& stream, const KNHRacingLine& line) {
    stream << quint32(line.cameraConfigs.size());
    for (const auto& cfg : line.cameraConfigs) {
        stream << cfg.fov;
        stream << cfg.nearPlane;
        stream << cfg.farPlane;
        stream << cfg.exposure;
        stream << cfg.minExposure;
        stream << cfg.maxExposure;
        stream << cfg.dofFocus;
        stream << cfg.dofFactor;
        stream << cfg.dofRange;
        stream << cfg.dofManual;
        stream << cfg.shadowSplits[0];
        stream << cfg.shadowSplits[1];
        stream << cfg.shadowSplits[2];
        stream << cfg.inPoint;
        stream << cfg.outPoint;
        stream << cfg.isFixed;
        stream << cfg.splineRotation;
        stream << cfg.splineAnimationLength;
    }
}

// ============================================================================
// KNHConverter implementation
// ============================================================================

bool KNHConverter::convertFromFastLane(const QString& fastLanePath, const QString& knhPath) {
    QFile file(fastLanePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    KNHRacingLine line;
    line.trackName = QFileInfo(fastLanePath).baseName();
    line.trackConfig = "default";

    QTextStream in(&file);
    QString headerLine = in.readLine(); // Skip header

    while (!in.atEnd()) {
        QString ln = in.readLine().trimmed();
        if (ln.isEmpty() || ln.startsWith(';') || ln.startsWith('#')) continue;

        QStringList parts = ln.split(',', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            KNHWaypoint wp;
            wp.position = QVector3D(parts[0].toFloat(), parts[1].toFloat(), parts[2].toFloat());
            if (parts.size() >= 6) {
                wp.tangent = QVector3D(parts[3].toFloat(), parts[4].toFloat(), parts[5].toFloat());
            }
            if (parts.size() >= 7) wp.targetSpeed = parts[6].toFloat() * 3.6f; // m/s -> km/h
            if (parts.size() >= 8) wp.gear = parts[7].toFloat();
            wp.isCorner = (parts.size() > 9 && parts[9].toInt() == 1);
            wp.isStraight = !wp.isCorner;
            line.waypoints.append(wp);
        }
    }

    file.close();

    // Calculate sectors
    float totalLen = line.totalLength();
    float sectorLen = totalLen / 3.0f;
    for (int i = 0; i < line.waypoints.size(); ++i) {
        line.waypoints[i].sector = qMin(2, int(line.totalLength() / sectorLen));
    }

    KNHWriter writer;
    return writer.write(knhPath, line);
}

bool KNHConverter::convertFromIdealLine(const QString& idealLinePath, const QString& knhPath) {
    // Similar to fast_lane but with ideal racing line data
    return convertFromFastLane(idealLinePath, knhPath);
}

bool KNHConverter::convertFromCameraSpline(const QString& splinePath, const QString& knhPath) {
    QFile file(splinePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    KNHRacingLine line;
    line.trackName = QFileInfo(splinePath).baseName();

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 splineCount;
    stream >> splineCount;
    line.cameraSplines.resize(splineCount);

    for (quint32 i = 0; i < splineCount; ++i) {
        KNHCameraSpline& spline = line.cameraSplines[i];
        stream >> spline.name;
        stream >> spline.degree;
        stream >> spline.cyclic;

        quint32 ptCount;
        stream >> ptCount;
        spline.controlPoints.resize(ptCount);
        for (quint32 j = 0; j < ptCount; ++j) {
            stream >> spline.controlPoints[j];
        }

        quint32 knotCount;
        stream >> knotCount;
        spline.knots.resize(knotCount);
        for (quint32 j = 0; j < knotCount; ++j) {
            stream >> spline.knots[j];
        }
    }

    file.close();

    KNHWriter writer;
    return writer.write(knhPath, line);
}

bool KNHConverter::generateFromTrackGeometry(const QString& trackPath, const QString& knhPath, float targetSpeed) {
    // TODO: Implement track geometry analysis to generate racing line
    // Would need track mesh, surface data, etc.
    return false;
}

bool KNHConverter::exportFastLane(const QString& knhPath, const QString& outputPath) {
    KNHRacingLine line;
    KNHReader reader;
    if (!reader.read(knhPath, line)) return false;

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "POSITION_X,POSITION_Y,POSITION_Z,TANGENT_X,TANGENT_Y,TANGENT_Z,SPEED,GEAR,SECTOR,IS_CORNER\n";

    for (const auto& wp : line.waypoints) {
        out << wp.position.x() << ","
            << wp.position.y() << ","
            << wp.position.z() << ","
            << wp.tangent.x() << ","
            << wp.tangent.y() << ","
            << wp.tangent.z() << ","
            << (wp.targetSpeed / 3.6f) << ","  // km/h -> m/s
            << wp.gear << ","
            << wp.sector << ","
            << (wp.isCorner ? 1 : 0) << "\n";
    }

    file.close();
    return true;
}

bool KNHConverter::exportIdealLine(const QString& knhPath, const QString& outputPath) {
    // Same as fast_lane but with ideal line data
    return exportFastLane(knhPath, outputPath);
}

bool KNHConverter::exportCameraSplines(const QString& knhPath, const QString& outputDir) {
    KNHRacingLine line;
    KNHReader reader;
    if (!reader.read(knhPath, line)) return false;

    QDir dir(outputDir);
    if (!dir.exists()) dir.mkpath(".");

    for (const auto& spline : line.cameraSplines) {
        QString path = outputDir + "/" + spline.name + ".spline";
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) continue;

        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << quint32(1); // spline count
        stream << spline.name;
        stream << spline.degree;
        stream << spline.cyclic;
        stream << quint32(spline.controlPoints.size());
        for (const auto& pt : spline.controlPoints) stream << pt;
        stream << quint32(spline.knots.size());
        for (float k : spline.knots) stream << k;
        file.close();
    }

    return true;
}

} // namespace ks