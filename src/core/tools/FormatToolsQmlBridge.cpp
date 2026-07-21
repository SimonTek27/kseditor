#include "FormatToolsQmlBridge.h"
#include "../sys/LogManager.h"
#include <QGroupBox>
#include <QFormLayout>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <cmath>
#include <algorithm>

namespace ks {

FormatToolsQmlBridge* FormatToolsQmlBridge::s_instance = nullptr;

FormatToolsQmlBridge* FormatToolsQmlBridge::instance() {
    if (!s_instance) {
        s_instance = new FormatToolsQmlBridge();
    }
    return s_instance;
}

FormatToolsQmlBridge::FormatToolsQmlBridge(QObject* parent)
    : QObject(parent)
{
}

// ============================================================================
// AI Line Operations
// ============================================================================

float FormatToolsQmlBridge::calculateDistance(const AiLineDataPoint& a, const AiLineDataPoint& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool FormatToolsQmlBridge::parseAiBinaryLine(QDataStream& stream, AiLineFileData& data) {
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> data.header >> data.pointCount >> data.unknown1 >> data.unknown2;

    if (data.pointCount <= 0 || data.pointCount > 1000000) {
        return false;
    }

    data.idealLine.resize(data.pointCount);
    for (int i = 0; i < data.pointCount; ++i) {
        float x, z, y, dist;
        int id;
        stream >> x >> z >> y >> dist >> id;
        data.idealLine[i].x = x;
        data.idealLine[i].y = -y;
        data.idealLine[i].z = z;
        data.idealLine[i].distance = dist;
        data.idealLine[i].id = id;
    }

    data.detailData.resize(data.pointCount);
    for (int i = 0; i < data.pointCount; ++i) {
        float vals[18];
        for (int j = 0; j < 18; ++j) {
            stream >> vals[j];
        }
        data.detailData[i].unknown0 = vals[0];
        data.detailData[i].speed = vals[1];
        data.detailData[i].gas = vals[2];
        data.detailData[i].brake = vals[3];
        data.detailData[i].obsoleteLatG = vals[4];
        data.detailData[i].radius = vals[5];
        data.detailData[i].wallLeft = vals[6];
        data.detailData[i].wallRight = vals[7];
        data.detailData[i].camber = vals[8];
        data.detailData[i].direction = vals[9];
        data.detailData[i].normalX = vals[10];
        data.detailData[i].normalY = vals[11];
        data.detailData[i].normalZ = vals[12];
        data.detailData[i].length = vals[13];
        data.detailData[i].forwardVectorX = vals[14];
        data.detailData[i].forwardVectorY = vals[15];
        data.detailData[i].forwardVectorZ = vals[16];
        data.detailData[i].tag = vals[17];
    }

    if (!stream.atEnd()) {
        data.restData = stream.device()->readAll();
    }

    return true;
}

QVariantMap FormatToolsQmlBridge::importAiLine(const QString& filePath, float scaling, bool importExtraData) {
    QVariantMap result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot open AI line file: " + filePath);
        return result;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    AiLineFileData data;
    if (!parseAiBinaryLine(stream, data)) {
        emit errorMessage("Failed to parse AI line file: " + filePath);
        return result;
    }
    file.close();

    QVariantList idealLineVertices;
    for (const auto& pt : data.idealLine) {
        QVariantMap v;
        v["x"] = pt.x * scaling;
        v["y"] = pt.y * scaling;
        v["z"] = pt.z * scaling;
        v["distance"] = pt.distance;
        v["id"] = pt.id;
        idealLineVertices.append(v);
    }
    result["idealLine"] = idealLineVertices;

    QVariantList leftBorder;
    QVariantList rightBorder;
    float prevX = 0, prevY = 0;
    for (int i = 0; i < data.idealLine.size() && i < data.detailData.size(); ++i) {
        const auto& pt = data.idealLine[i];
        const auto& detail = data.detailData[i];

        float dx = pt.x - prevX;
        float dy = pt.y - prevY;
        float len = std::sqrt(dx*dx + dy*dy);
        float dir = 0;
        if (len > 0.001f) {
            dir = -std::atan2(dy, dx) * 180.0f / 3.14159265f;
        }

        float wallL = detail.wallLeft;
        float wallR = detail.wallRight;

        float leftX = pt.x + std::cos((dir + 90) * 3.14159265f / 180.0f) * wallL;
        float leftY = pt.y - std::sin((dir + 90) * 3.14159265f / 180.0f) * wallL;
        QVariantMap lb;
        lb["x"] = leftX * scaling;
        lb["y"] = leftY * scaling;
        lb["z"] = pt.z * scaling;
        leftBorder.append(lb);

        float rightX = pt.x + std::cos((dir - 90) * 3.14159265f / 180.0f) * wallR;
        float rightY = pt.y - std::sin((dir - 90) * 3.14159265f / 180.0f) * wallR;
        QVariantMap rb;
        rb["x"] = rightX * scaling;
        rb["y"] = rightY * scaling;
        rb["z"] = pt.z * scaling;
        rightBorder.append(rb);

        prevX = pt.x;
        prevY = pt.y;
    }
    result["leftBorder"] = leftBorder;
    result["rightBorder"] = rightBorder;

    if (importExtraData) {
        QVariantList speedData, gasData, brakeData, camberData, radiusData;
        for (const auto& detail : data.detailData) {
            QVariantMap spd;
            spd["speed"] = detail.speed;
            speedData.append(spd);

            QVariantMap gas;
            gas["gas"] = detail.gas;
            gasData.append(gas);

            QVariantMap brk;
            brk["brake"] = detail.brake;
            brakeData.append(brk);

            QVariantMap cmb;
            cmb["camber"] = detail.camber;
            camberData.append(cmb);

            QVariantMap rad;
            rad["radius"] = detail.radius;
            radiusData.append(rad);
        }
        result["speedData"] = speedData;
        result["gasData"] = gasData;
        result["brakeData"] = brakeData;
        result["camberData"] = camberData;
        result["radiusData"] = radiusData;
    }

    result["pointCount"] = data.pointCount;
    result["filePath"] = filePath;

    emit importComplete(filePath, data.pointCount);
    emit statusMessage(QString("Imported AI line: %1 points").arg(data.pointCount));
    return result;
}

QVariantMap FormatToolsQmlBridge::importAiLineBorders(const QString& filePath, float scaling) {
    QVariantMap result;
    QVariantMap fullData = importAiLine(filePath, scaling, false);
    result["leftBorder"] = fullData["leftBorder"];
    result["rightBorder"] = fullData["rightBorder"];
    return result;
}

bool FormatToolsQmlBridge::exportAiLine(const QString& filePath, const QVariantList& vertices,
                                             float scaling, int shiftCount, bool reverse,
                                             bool fixedBorders, float fixedLeft, float fixedRight) {
    if (vertices.size() < 2) {
        emit errorMessage("Not enough vertices to export AI line");
        return false;
    }

    AiLineFileData newData;
    newData.header = 7;
    newData.pointCount = vertices.size();
    newData.unknown1 = 0;
    newData.unknown2 = 0;

    AiLineFileData existingData;
    bool hasExisting = false;
    QFile existingFile(filePath);
    if (existingFile.open(QIODevice::ReadOnly)) {
        QDataStream existingStream(&existingFile);
        if (parseAiBinaryLine(existingStream, existingData)) {
            hasExisting = true;
        }
        existingFile.close();
    }

    newData.idealLine.resize(vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        QVariantMap v = vertices[i].toMap();
        int idx = i;
        if (reverse) {
            idx = vertices.size() - 1 - i;
        }
        idx = (idx + shiftCount) % vertices.size();
        if (idx < 0) idx += vertices.size();

        newData.idealLine[i].x = v["x"].toFloat() * scaling;
        newData.idealLine[i].y = v["y"].toFloat() * scaling;
        newData.idealLine[i].z = v["z"].toFloat() * scaling;

        if (i > 0) {
            newData.idealLine[i].distance = newData.idealLine[i-1].distance +
                calculateDistance(newData.idealLine[i-1], newData.idealLine[i]);
        } else {
            newData.idealLine[i].distance = 0;
        }
        newData.idealLine[i].id = idx;
    }

    newData.detailData.resize(vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        int idx = i;
        if (reverse) {
            idx = vertices.size() - 1 - i;
        }
        idx = (idx + shiftCount) % vertices.size();
        if (idx < 0) idx += vertices.size();

        if (hasExisting && idx < existingData.detailData.size()) {
            newData.detailData[i] = existingData.detailData[idx];
        } else {
            newData.detailData[i] = AiLineDetailData();
        }

        if (fixedBorders) {
            newData.detailData[i].wallLeft = fixedLeft;
            newData.detailData[i].wallRight = fixedRight;
        }
    }

    if (hasExisting && !existingData.restData.isEmpty()) {
        newData.restData = existingData.restData;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorMessage("Cannot write AI line file: " + filePath);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    writeAiBinaryLine(stream, newData);
    file.close();

    emit exportComplete(filePath);
    emit statusMessage(QString("Exported AI line: %1 points").arg(vertices.size()));
    return true;
}

bool FormatToolsQmlBridge::writeAiBinaryLine(QDataStream& stream, const AiLineFileData& data) {
    stream << data.header << data.pointCount << data.unknown1 << data.unknown2;

    for (const auto& pt : data.idealLine) {
        float x = pt.x;
        float z = pt.z;
        float y = -pt.y;
        stream << x << z << y << pt.distance << pt.id;
    }

    for (const auto& detail : data.detailData) {
        stream << detail.unknown0 << detail.speed << detail.gas << detail.brake
               << detail.obsoleteLatG << detail.radius << detail.wallLeft << detail.wallRight
               << detail.camber << detail.direction << detail.normalX << detail.normalY
               << detail.normalZ << detail.length << detail.forwardVectorX << detail.forwardVectorY
               << detail.forwardVectorZ << detail.tag;
    }

    if (!data.restData.isEmpty()) {
        stream.writeRawData(data.restData.constData(), data.restData.size());
    }

    return true;
}

// ============================================================================
// CSV Border Operations
// ============================================================================

QVariantList FormatToolsQmlBridge::importCsv(const QString& filePath, float scaling) {
    QVariantList vertices;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Cannot open CSV file: " + filePath);
        return vertices;
    }

    QTextStream stream(&file);
    bool headerSkipped = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (!headerSkipped) {
            headerSkipped = true;
            if (line.contains(',')) {
                bool allNumeric = true;
                for (const QString& part : line.split(',')) {
                    bool ok;
                    part.trimmed().toFloat(&ok);
                    if (!ok) { allNumeric = false; break; }
                }
                if (!allNumeric) continue;
            }
        }

        QStringList parts = line.split(',');
        if (parts.size() >= 3) {
            bool ok;
            float x = parts[0].trimmed().toFloat(&ok);
            if (!ok) continue;
            float z = parts[1].trimmed().toFloat(&ok);
            if (!ok) continue;
            float y = parts[2].trimmed().toFloat(&ok);
            if (!ok) continue;

            QVariantMap v;
            v["x"] = x * scaling;
            v["y"] = -y * scaling;
            v["z"] = z * scaling;

            if (parts.size() >= 4) {
                v["pointOfTrack"] = parts[3].trimmed().toFloat(&ok);
            }
            vertices.append(v);
        }
    }
    file.close();

    emit importComplete(filePath, vertices.size());
    return vertices;
}

bool FormatToolsQmlBridge::exportCsv(const QString& filePath, const QVariantList& vertices,
                                          float scaling, int shiftCount, bool reverse, bool skipPoT) {
    if (vertices.isEmpty()) {
        emit errorMessage("No vertices to export");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Cannot write CSV file: " + filePath);
        return false;
    }

    QTextStream stream(&file);

    float totalDist = 0.0f;
    for (int i = 1; i < vertices.size(); ++i) {
        QVariantMap a = vertices[i-1].toMap();
        QVariantMap b = vertices[i].toMap();
        float dx = b["x"].toFloat() - a["x"].toFloat();
        float dy = b["y"].toFloat() - a["y"].toFloat();
        float dz = b["z"].toFloat() - a["z"].toFloat();
        totalDist += std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    float dist = 0.0f;
    for (int i = 0; i < vertices.size(); ++i) {
        int idx = reverse ? (vertices.size() - 1 - i) : i;
        idx = (idx + shiftCount) % vertices.size();
        if (idx < 0) idx += vertices.size();

        QVariantMap v = vertices[idx].toMap();
        float x = v["x"].toFloat() / scaling;
        float y = v["y"].toFloat() / scaling;
        float z = v["z"].toFloat() / scaling;

        if (i > 0) {
            QVariantMap prev = vertices[reverse ? (vertices.size() - i) : (i-1)].toMap();
            int prevIdx = reverse ? (vertices.size() - i) : (i-1);
            prevIdx = (prevIdx + shiftCount) % vertices.size();
            if (prevIdx < 0) prevIdx += vertices.size();
            prev = vertices[prevIdx].toMap();

            float dx = (v["x"].toFloat() / scaling) - prev["x"].toFloat();
            float dy = (v["y"].toFloat() / scaling) - prev["y"].toFloat();
            float dz = (v["z"].toFloat() / scaling) - prev["z"].toFloat();
            dist += std::sqrt(dx*dx + dy*dy + dz*dz);
        }

        if (skipPoT) {
            stream << QString("%1,%2,%3\n")
                      .arg(x, 0, 'f', 4)
                      .arg(z, 0, 'f', 4)
                      .arg(y, 0, 'f', 4);
        } else {
            float pot = (totalDist > 0) ? dist / totalDist : 0.0f;
            stream << QString("%1,%2,%3,%4\n")
                      .arg(x, 0, 'f', 4)
                      .arg(z, 0, 'f', 4)
                      .arg(y, 0, 'f', 4)
                      .arg(pot, 0, 'f', 6);
        }
    }

    file.close();
    emit exportComplete(filePath);
    return true;
}

// ============================================================================
// Camera.ini Operations
// ============================================================================

QVariantList FormatToolsQmlBridge::importCameraIni(const QString& filePath) {
    QVariantList cameras;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Cannot open camera.ini: " + filePath);
        return cameras;
    }

    QTextStream stream(&file);
    CameraData currentCamera;
    bool inCamera = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            if (inCamera) {
                QVariantMap cam;
                cam["name"] = currentCamera.name;
                cam["positionX"] = currentCamera.positionX;
                cam["positionY"] = currentCamera.positionY;
                cam["positionZ"] = currentCamera.positionZ;
                cam["targetX"] = currentCamera.targetX;
                cam["targetY"] = currentCamera.targetY;
                cam["targetZ"] = currentCamera.targetZ;
                cam["fov"] = currentCamera.fov;
                cam["nearPlane"] = currentCamera.nearPlane;
                cam["farPlane"] = currentCamera.farPlane;
                cam["tilt"] = currentCamera.tilt;
                cameras.append(cam);
            }
            currentCamera = CameraData();
            currentCamera.name = line.mid(1, line.length() - 2);
            inCamera = true;
        } else if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toLower();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "position") {
                QStringList vals = value.split(',');
                if (vals.size() >= 3) {
                    currentCamera.positionX = vals[0].trimmed().toFloat();
                    currentCamera.positionY = vals[1].trimmed().toFloat();
                    currentCamera.positionZ = vals[2].trimmed().toFloat();
                }
            } else if (key == "target") {
                QStringList vals = value.split(',');
                if (vals.size() >= 3) {
                    currentCamera.targetX = vals[0].trimmed().toFloat();
                    currentCamera.targetY = vals[1].trimmed().toFloat();
                    currentCamera.targetZ = vals[2].trimmed().toFloat();
                }
            } else if (key == "fov") {
                currentCamera.fov = value.toFloat();
            } else if (key == "near") {
                currentCamera.nearPlane = value.toFloat();
            } else if (key == "far") {
                currentCamera.farPlane = value.toFloat();
            } else if (key == "tilt") {
                currentCamera.tilt = value.toFloat();
            }
        }
    }

    if (inCamera) {
        QVariantMap cam;
        cam["name"] = currentCamera.name;
        cam["positionX"] = currentCamera.positionX;
        cam["positionY"] = currentCamera.positionY;
        cam["positionZ"] = currentCamera.positionZ;
        cam["targetX"] = currentCamera.targetX;
        cam["targetY"] = currentCamera.targetY;
        cam["targetZ"] = currentCamera.targetZ;
        cam["fov"] = currentCamera.fov;
        cam["nearPlane"] = currentCamera.nearPlane;
        cam["farPlane"] = currentCamera.farPlane;
        cam["tilt"] = currentCamera.tilt;
        cameras.append(cam);
    }

    file.close();
    return cameras;
}

bool FormatToolsQmlBridge::exportCameraIni(const QString& filePath, const QVariantList& cameras) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Cannot write camera.ini: " + filePath);
        return false;
    }

    QTextStream stream(&file);
    stream << "; Camera configuration\n";
    stream << "; Generated by ksEditor AC Blender Tools\n\n";

    for (int i = 0; i < cameras.size(); ++i) {
        QVariantMap cam = cameras[i].toMap();
        QString name = cam["name"].toString();
        if (name.isEmpty()) name = QString("CAMERA_%1").arg(i);

        stream << "[" << name << "]\n";
        stream << "POSITION=" << cam["positionX"].toFloat() << ", "
               << cam["positionY"].toFloat() << ", "
               << cam["positionZ"].toFloat() << "\n";
        stream << "TARGET=" << cam["targetX"].toFloat() << ", "
               << cam["targetY"].toFloat() << ", "
               << cam["targetZ"].toFloat() << "\n";
        stream << "FOV=" << cam["fov"].toFloat() << "\n";
        stream << "NEAR=" << cam["nearPlane"].toFloat() << "\n";
        stream << "FAR=" << cam["farPlane"].toFloat() << "\n";
        stream << "TILT=" << cam["tilt"].toFloat() << "\n\n";
    }

    file.close();
    emit exportComplete(filePath);
    return true;
}

// ============================================================================
// Overlay.ini Operations
// ============================================================================

QVariantList FormatToolsQmlBridge::importOverlayIni(const QString& filePath) {
    QVariantList overlays;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Cannot open overlay.ini: " + filePath);
        return overlays;
    }

    QTextStream stream(&file);
    OverlayData currentOverlay;
    bool inOverlay = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            if (inOverlay) {
                QVariantMap ov;
                ov["name"] = currentOverlay.name;
                ov["type"] = currentOverlay.type;
                ov["posX"] = currentOverlay.posX;
                ov["posY"] = currentOverlay.posY;
                ov["sizeX"] = currentOverlay.sizeX;
                ov["sizeY"] = currentOverlay.sizeY;
                ov["visible"] = currentOverlay.visible;
                ov["texture"] = currentOverlay.texture;
                overlays.append(ov);
            }
            currentOverlay = OverlayData();
            currentOverlay.name = line.mid(1, line.length() - 2);
            inOverlay = true;
        } else if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString key = line.left(eqPos).trimmed().toLower();
            QString value = line.mid(eqPos + 1).trimmed();

            if (key == "type") {
                currentOverlay.type = value.toInt();
            } else if (key == "position") {
                QStringList vals = value.split(',');
                if (vals.size() >= 2) {
                    currentOverlay.posX = vals[0].trimmed().toFloat();
                    currentOverlay.posY = vals[1].trimmed().toFloat();
                }
            } else if (key == "size") {
                QStringList vals = value.split(',');
                if (vals.size() >= 2) {
                    currentOverlay.sizeX = vals[0].trimmed().toFloat();
                    currentOverlay.sizeY = vals[1].trimmed().toFloat();
                }
            } else if (key == "visible") {
                currentOverlay.visible = (value.toLower() == "1" || value.toLower() == "true");
            } else if (key == "texture") {
                currentOverlay.texture = value;
            }
        }
    }

    if (inOverlay) {
        QVariantMap ov;
        ov["name"] = currentOverlay.name;
        ov["type"] = currentOverlay.type;
        ov["posX"] = currentOverlay.posX;
        ov["posY"] = currentOverlay.posY;
        ov["sizeX"] = currentOverlay.sizeX;
        ov["sizeY"] = currentOverlay.sizeY;
        ov["visible"] = currentOverlay.visible;
        ov["texture"] = currentOverlay.texture;
        overlays.append(ov);
    }

    file.close();
    return overlays;
}

bool FormatToolsQmlBridge::exportOverlayIni(const QString& filePath, const QVariantList& overlays) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Cannot write overlay.ini: " + filePath);
        return false;
    }

    QTextStream stream(&file);
    stream << "; Overlay configuration\n";
    stream << "; Generated by ksEditor AC Blender Tools\n\n";

    for (int i = 0; i < overlays.size(); ++i) {
        QVariantMap ov = overlays[i].toMap();
        QString name = ov["name"].toString();
        if (name.isEmpty()) name = QString("OVERLAY_%1").arg(i);

        stream << "[" << name << "]\n";
        stream << "TYPE=" << ov["type"].toInt() << "\n";
        stream << "POSITION=" << ov["posX"].toFloat() << ", " << ov["posY"].toFloat() << "\n";
        stream << "SIZE=" << ov["sizeX"].toFloat() << ", " << ov["sizeY"].toFloat() << "\n";
        stream << "VISIBLE=" << (ov["visible"].toBool() ? "1" : "0") << "\n";
        stream << "TEXTURE=" << ov["texture"].toString() << "\n\n";
    }

    file.close();
    emit exportComplete(filePath);
    return true;
}

// ============================================================================
// Material Fix Tools
// ============================================================================

QVariantMap FormatToolsQmlBridge::fixAlphaBlendToOpaque(const QVariantList& materials) {
    QVariantMap result;
    int fixedCount = 0;
    QVariantList fixedMaterials;

    for (const auto& mat : materials) {
        QVariantMap m = mat.toMap();
        if (m["blendMode"].toString() == "AlphaBlend" || m["blendMode"].toInt() == 1) {
            m["blendMode"] = "Opaque";
            m["blendModeInt"] = 0;
            fixedCount++;
        }
        fixedMaterials.append(m);
    }

    result["fixedCount"] = fixedCount;
    result["materials"] = fixedMaterials;
    return result;
}

QVariantMap FormatToolsQmlBridge::resetSpecularMetallic(const QVariantList& materials) {
    QVariantMap result;
    int resetCount = 0;
    QVariantList fixedMaterials;

    for (const auto& mat : materials) {
        QVariantMap m = mat.toMap();
        bool changed = false;
        if (m["specular"].toFloat() != 0.0f) {
            m["specular"] = 0.0f;
            changed = true;
        }
        if (m["metallic"].toFloat() != 0.0f) {
            m["metallic"] = 0.0f;
            changed = true;
        }
        if (changed) resetCount++;
        fixedMaterials.append(m);
    }

    result["resetCount"] = resetCount;
    result["materials"] = fixedMaterials;
    return result;
}

// ============================================================================
// Mesh Cleanup Tools
// ============================================================================

QVariantMap FormatToolsQmlBridge::mergeByDistance(const QVariantList& vertices, float threshold) {
    QVariantMap result;
    QVector<QPair<int,int>> duplicateMap;

    QVector<AiLineVertex> uniqueVerts;
    for (int i = 0; i < vertices.size(); ++i) {
        QVariantMap v = vertices[i].toMap();
        AiLineVertex vert;
        vert.x = v["x"].toFloat();
        vert.y = v["y"].toFloat();
        vert.z = v["z"].toFloat();

        int mergedTo = -1;
        for (int j = 0; j < uniqueVerts.size(); ++j) {
            float dx = vert.x - uniqueVerts[j].x;
            float dy = vert.y - uniqueVerts[j].y;
            float dz = vert.z - uniqueVerts[j].z;
            float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
            if (dist < threshold) {
                mergedTo = j;
                break;
            }
        }

        if (mergedTo >= 0) {
            duplicateMap.append(QPair<int,int>(i, mergedTo));
        } else {
            uniqueVerts.append(vert);
        }
    }

    QVariantList mergedVertices;
    for (const auto& v : uniqueVerts) {
        QVariantMap mv;
        mv["x"] = v.x;
        mv["y"] = v.y;
        mv["z"] = v.z;
        mergedVertices.append(mv);
    }

    result["originalCount"] = vertices.size();
    result["mergedCount"] = uniqueVerts.size();
    result["removedCount"] = vertices.size() - uniqueVerts.size();
    result["vertices"] = mergedVertices;
    QVariantList dupList;
    for (const auto& p : duplicateMap) {
        QVariantMap dm;
        dm["original"] = p.first;
        dm["mergedTo"] = p.second;
        dupList.append(dm);
    }
    result["duplicateMap"] = dupList;

    return result;
}

// ============================================================================
// Name Cleanup
// ============================================================================

QStringList FormatToolsQmlBridge::cleanNames(const QStringList& names) {
    QStringList cleaned;
    QRegularExpression suffixRegex("\\.\\d{3}$");

    for (const QString& name : names) {
        QString cleanName = name;
        cleanName.remove(suffixRegex);

        if (cleanName != name) {
            LOG_INFO("ACBlenderTools", QString("Cleaned name: %1 -> %2").arg(name, cleanName));
        }
        cleaned.append(cleanName);
    }

    return cleaned;
}

// ============================================================================
// AC Object Creation Helpers
// ============================================================================

QVariantMap FormatToolsQmlBridge::createAcObject(const QString& objectType, const QVariantMap& params) {
    QVariantMap result;
    result["type"] = objectType;

    if (objectType == "start_position") {
        result = createStartPosition(
            params["x"].toFloat(),
            params["z"].toFloat(),
            params["rotation"].toFloat(),
            params["isRightSide"].toBool()
        );
    } else if (objectType == "timing_left" || objectType == "timing_right") {
        result = createTimingPosition(
            params["x"].toFloat(),
            params["z"].toFloat(),
            objectType == "timing_right"
        );
    } else if (objectType == "grid_position") {
        int row = params["row"].toInt();
        int col = params["col"].toInt();
        float spacing = params["spacing"].toFloat();
        if (!params.contains("spacing")) spacing = 5.0f;
        float startX = params["startX"].toFloat();
        float startZ = params["startZ"].toFloat();

        result["x"] = startX + col * spacing;
        result["z"] = startZ + row * spacing * 0.3f;
        result["y"] = 0.0f;
        result["rotation"] = params["baseRotation"].toFloat();
    } else if (objectType == "pit_position") {
        int pitIndex = params["pitIndex"].toInt();
        float pitLaneX = params["pitLaneX"].toFloat();
        float pitLaneZ = params["pitLaneZ"].toFloat();
        float pitSpacing = params["pitSpacing"].toFloat();
        if (!params.contains("pitSpacing")) pitSpacing = 6.0f;

        result["x"] = pitLaneX;
        result["z"] = pitLaneZ + pitIndex * pitSpacing;
        result["y"] = 0.0f;
        result["rotation"] = params["baseRotation"].toFloat();
    }

    return result;
}

QVariantMap FormatToolsQmlBridge::createStartPosition(float x, float z, float rotation, bool isRightSide) {
    QVariantMap result;
    result["type"] = "start_position";
    result["x"] = x;
    result["y"] = 0.0f;
    result["z"] = z;
    result["rotation"] = rotation;
    result["side"] = isRightSide ? "right" : "left";
    result["meshPrefix"] = "1ROAD";
    result["soundType"] = "asphalt";
    return result;
}

QVariantMap FormatToolsQmlBridge::createTimingPosition(float x, float z, bool isRightSide) {
    QVariantMap result;
    result["type"] = isRightSide ? "timing_right" : "timing_left";
    result["x"] = x;
    result["y"] = 0.0f;
    result["z"] = z;
    result["rotation"] = 0.0f;
    result["side"] = isRightSide ? "right" : "left";
    return result;
}

// ============================================================================
// Batch Operations
// ============================================================================

QVariantMap FormatToolsQmlBridge::batchImportAiLines(const QStringList& filePaths, float scaling) {
    QVariantMap result;
    QVariantList imported;
    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < filePaths.size(); ++i) {
        emit batchProgress(i + 1, filePaths.size());
        QVariantMap data = importAiLine(filePaths[i], scaling, false);
        if (!data.isEmpty() && data["pointCount"].toInt() > 0) {
            data["sourceFile"] = filePaths[i];
            imported.append(data);
            successCount++;
        } else {
            failCount++;
        }
    }

    result["successCount"] = successCount;
    result["failCount"] = failCount;
    result["totalFiles"] = filePaths.size();
    result["importedLines"] = imported;

    emit statusMessage(QString("Batch import: %1/%2 files successful")
                       .arg(successCount).arg(filePaths.size()));
    return result;
}

QVariantMap FormatToolsQmlBridge::batchExportAiLines(const QString& directory,
                                                         const QVariantList& lineData,
                                                         float scaling) {
    QVariantMap result;
    int successCount = 0;
    int failCount = 0;

    QDir().mkpath(directory);

    for (int i = 0; i < lineData.size(); ++i) {
        emit batchProgress(i + 1, lineData.size());
        QVariantMap line = lineData[i].toMap();
        QString fileName = line["fileName"].toString();
        if (fileName.isEmpty()) {
            fileName = QString("line_%1.ai").arg(i);
        }
        QString filePath = directory + "/" + fileName;

        QVariantList vertices = line["vertices"].toList();
        if (exportAiLine(filePath, vertices, scaling)) {
            successCount++;
        } else {
            failCount++;
        }
    }

    result["successCount"] = successCount;
    result["failCount"] = failCount;
    result["totalLines"] = lineData.size();

    emit statusMessage(QString("Batch export: %1/%2 lines successful")
                       .arg(successCount).arg(lineData.size()));
    return result;
}

// ============================================================================
// 3D Replay Visualization
// ============================================================================

QVariantList FormatToolsQmlBridge::importReplayPath(const QString& replayPath, int carId) {
    QVariantList path;

    QFile file(replayPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot open replay file: " + replayPath);
        return path;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    stream >> magic;

    if (magic != 0x41435250 && magic != 0x504C4152) {
        emit errorMessage("Invalid replay file format");
        file.close();
        return path;
    }

    quint32 version = 0;
    stream >> version;

    quint32 carCount = 0;
    stream >> carCount;

    if (carId < 0 || carId >= static_cast<int>(carCount)) {
        carId = 0;
    }

    struct ReplayHeader {
        quint32 magic;
        quint32 version;
        quint32 carCount;
        quint32 frameCount;
        float trackLength;
        quint32 recordingDate;
    };

    ReplayHeader header;
    header.magic = magic;
    header.version = version;
    header.carCount = carCount;
    stream >> header.frameCount >> header.trackLength >> header.recordingDate;

    for (quint32 frame = 0; frame < header.frameCount; ++frame) {
        quint16 timestamp;
        quint16 flags;
        stream >> timestamp >> flags;

        for (quint32 c = 0; c < header.carCount; ++c) {
            float posX, posY, posZ;
            float speed;
            quint16 gear;
            float steer;
            float throttle;
            float brake;

            stream >> posX >> posY >> posZ;
            stream >> speed >> gear >> steer >> throttle >> brake;

            if (static_cast<int>(c) == carId) {
                QVariantMap point;
                point["x"] = posX;
                point["y"] = posY;
                point["z"] = posZ;
                point["speed"] = speed;
                point["gear"] = gear;
                point["throttle"] = throttle;
                point["brake"] = brake;
                point["timestamp"] = timestamp;
                path.append(point);
            } else {
                stream.skipRawData(20);
            }
        }
    }

    file.close();

    emit statusMessage(QString("Imported replay path: %1 points for car %2")
                       .arg(path.size()).arg(carId));
    return path;
}

QVariantMap FormatToolsQmlBridge::getReplayInfo(const QString& replayPath) {
    QVariantMap info;

    QFile file(replayPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return info;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    stream >> magic;

    if (magic != 0x41435250 && magic != 0x504C4152) {
        file.close();
        return info;
    }

    quint32 version, carCount, frameCount;
    float trackLength;
    quint32 recordingDate;

    stream >> version >> carCount >> frameCount >> trackLength >> recordingDate;

    info["version"] = version;
    info["carCount"] = carCount;
    info["frameCount"] = frameCount;
    info["trackLength"] = trackLength;
    info["recordingDate"] = recordingDate;
    info["filePath"] = replayPath;
    info["fileSize"] = file.size();

    file.close();
    return info;
}

// ============================================================================
// Naming Convention (modder_year_manufacturer_carname)
// ============================================================================

QString FormatToolsQmlBridge::generateCarName(const QString& modder, int year, const QString& manufacturer, const QString& carName) {
    QString cleanModder = modder.toLower().replace(" ", "_").replace("-", "_");
    QString cleanMfr = manufacturer.toLower().replace(" ", "_").replace("-", "_");
    QString cleanCar = carName.toLower().replace(" ", "_").replace("-", "_");
    return QString("%1_%2_%3_%4").arg(cleanModder).arg(year).arg(cleanMfr).arg(cleanCar);
}

QString FormatToolsQmlBridge::generatePrefix(const QString& manufacturer) {
    QString mfr = manufacturer.toLower().trimmed();
    static QMap<QString, QString> knownPrefixes = {
        {"benetton", "ben"}, {"ferrari", "fer"}, {"mclaren", "mcl"},
        {"williams", "wil"}, {"mercedes", "mer"}, {"red_bull", "reb"},
        {"aston_martin", "ast"}, {"alpine", "alp"}, {"haas", "has"},
        {"alfa", "alf"}, {"alphatauri", "ata"}, {"racing_point", "rac"},
        {"force_india", "frc"}, {"toro_rosso", "tor"}, {"lotus", "lot"},
        {"caterham", "cat"}, {"marussia", "mar"}, {"hrt", "hrt"},
        {"sauber", "sau"}, {"prost", "pro"}, {"jordan", "jor"},
        {"minardi", "min"}, {"tyrell", "tyr"}, {"brabham", "bra"},
        {"lotus_f1", "lot"}, {"renault", "ren"}, {"porsche", "por"},
        {"bmw", "bmw"}, {"audi", "aud"}, {"toyota", "toy"},
        {"honda", "hon"}, {"nissan", "nis"}, {"mazda", "maz"},
        {"subaru", "sub"}, {"mitsubishi", "mit"}, {"ford", "for"},
        {"chevrolet", "che"}, {"dodge", "Dod"}, {"chrysler", "chr"},
        {"pontiac", "pon"}, {"buick", "bui"}, {"cadillac", "cad"},
        {"lincoln", "lin"}, {"acura", "acr"}, {"infiniti", "inf"},
        {"lexus", "lex"}, {"genesis", "gen"}, {"hyundai", "hyu"},
        {"kia", "kia"}, {"volvo", "vol"}, {"jaguar", "jag"},
        {"land_rover", "lan"}, {"aston", "ast"}, {"maserati", "mas"},
        {"lamborghini", "lam"}, {"bugatti", "bug"}, {"ferrari", "fer"},
        {"mclaren", "mcl"}, {"pagani", "pag"}, {"koenigsegg", "koen"},
        {"noble", "nob"}, {"aria", "ari"}, {"wiesmann", "wie"},
        {"TVR", "tvr"}, {"lotus", "lot"}, {"caterham", "cat"},
        {"aria", "ari"}, {"radical", "rad"}, {"ultima", "ult"}
    };

    if (knownPrefixes.contains(mfr)) {
        return knownPrefixes[mfr];
    }

    // Auto-generate: take first 3 consonants
    QString consonants;
    for (const QChar& c : mfr) {
        if (c.isLetter() && !QString("aeiou").contains(c)) {
            consonants += c;
            if (consonants.length() == 3) break;
        }
    }

    if (consonants.length() < 3) {
        consonants = mfr.left(3);
    }

    return consonants;
}

QVariantList FormatToolsQmlBridge::getComponentList() {
    QVariantList components;

    auto addComponent = [&](const QString& name, const QString& category, bool required = true) {
        QVariantMap c;
        c["name"] = name;
        c["category"] = category;
        c["required"] = required;
        components.append(c);
    };

    // Body
    addComponent("body", "Body", true);
    addComponent("body_low", "Body", false);
    addComponent("underbody", "Body", false);
    addComponent("nosecone", "Body", true);
    addComponent("collision", "Body", false);
    addComponent("barrier", "Body", false);
    addComponent("shadow", "Body", false);
    addComponent("skidblock", "Body", false);

    // Aero
    addComponent("fwing", "Aero", true);
    addComponent("fwing_flap_l", "Aero", false);
    addComponent("fwing_flap_r", "Aero", false);
    addComponent("rwing", "Aero", true);
    addComponent("rwing_flap_l", "Aero", false);
    addComponent("rwing_flap_r", "Aero", false);
    addComponent("diffuser", "Aero", false);
    addComponent("bargeboard_l", "Aero", false);
    addComponent("bargeboard_r", "Aero", false);
    addComponent("airscope", "Aero", true);
    addComponent("turning_vane_l", "Aero", false);
    addComponent("turning_vane_r", "Aero", false);
    addComponent("floor_edge_l", "Aero", false);
    addComponent("floor_edge_r", "Aero", false);

    // Cockpit
    addComponent("cockpit", "Cockpit", true);
    addComponent("cockpit_parts", "Cockpit", false);
    addComponent("intcockpit", "Cockpit", false);
    addComponent("interior", "Cockpit", false);
    addComponent("dashboard", "Cockpit", true);
    addComponent("steering", "Cockpit", false);
    addComponent("seat", "Cockpit", true);
    addComponent("display", "Cockpit", false);
    addComponent("motec_glass", "Cockpit", false);
    addComponent("mirrors", "Cockpit", false);
    addComponent("mirror_l", "Cockpit", false);
    addComponent("mirror_r", "Cockpit", false);
    addComponent("lmirror_in", "Cockpit", false);
    addComponent("rmirror_in", "Cockpit", false);
    addComponent("mirror_dust", "Cockpit", false);
    addComponent("wind_deflector", "Cockpit", false);
    addComponent("visor", "Cockpit", false);

    // Glass
    addComponent("cwind", "Glass", true);
    addComponent("sidepod_l", "Glass", false);
    addComponent("sidepod_r", "Glass", false);

    // Driver
    addComponent("driver", "Driver", true);
    addComponent("driver_belt", "Driver", false);
    addComponent("belt", "Driver", false);
    addComponent("helmet", "Driver", false);
    addComponent("suit", "Driver", false);
    addComponent("gloves", "Driver", false);
    addComponent("shoes", "Driver", false);
    addComponent("hands", "Driver", false);

    // Engine
    addComponent("engine", "Engine", true);
    addComponent("engine_low", "Engine", false);
    addComponent("coverengine", "Engine", false);
    addComponent("coverengine_int", "Engine", false);
    addComponent("intake", "Engine", false);
    addComponent("exhaust", "Engine", true);
    addComponent("fuel_cell", "Engine", false);
    addComponent("battery", "Engine", false);
    addComponent("ecu", "Engine", false);

    // Drivetrain
    addComponent("gearbox", "Drivetrain", true);
    addComponent("gearbox_parts", "Drivetrain", false);

    // Suspension (system nodes - uppercase)
    addComponent("SUSPRR", "Suspension", false);
    addComponent("SUSPLR", "Suspension", false);
    addComponent("SUSP_RR", "Suspension", false);
    addComponent("SUSP_LR", "Suspension", false);
    addComponent("HUB_RF", "Suspension", false);
    addComponent("SUSP_RF", "Suspension", false);
    addComponent("HUB_LF", "Suspension", false);
    addComponent("SUSP_LF", "Suspension", false);
    addComponent("SUSPLFSUP", "Suspension", false);
    addComponent("SUSPRFSUP", "Suspension", false);

    // Suspension (mesh nodes - lowercase)
    addComponent("rr_susp", "Suspension", false);
    addComponent("lr_susp", "Suspension", false);
    addComponent("rf_susp", "Suspension", false);
    addComponent("lf_susp", "Suspension", false);
    addComponent("rf_bcool", "Brakes", false);
    addComponent("lf_bcool", "Brakes", false);

    // Brakes
    addComponent("9X_RR_CAL_B", "Brakes", false);
    addComponent("9X_LR_CAL_B", "Brakes", false);
    addComponent("9X_RF_CAL_B", "Brakes", false);
    addComponent("9X_LF_CAL_B", "Brakes", false);
    addComponent("9x_rr_bd", "Brakes", false);
    addComponent("9x_lr_bd", "Brakes", false);
    addComponent("9x_rf_bd", "Brakes", false);
    addComponent("9x_lf_bd", "Brakes", false);

    // Wheels (system nodes)
    addComponent("WHEEL_RR", "Wheels", false);
    addComponent("WHEEL_LR", "Wheels", false);
    addComponent("WHEEL_RF", "Wheels", false);
    addComponent("WHEEL_LF", "Wheels", false);
    addComponent("DIR_SUSPRR", "Wheels", false);
    addComponent("DIR_SUSPLR", "Wheels", false);
    addComponent("DIR_SUPRF", "Wheels", false);
    addComponent("DIR_SUPLF", "Wheels", false);

    // Wheels (mesh nodes)
    addComponent("RIM_RR", "Wheels", true);
    addComponent("RIM_LR", "Wheels", true);
    addComponent("RIM_RF", "Wheels", true);
    addComponent("RIM_LF", "Wheels", true);
    addComponent("RIM_BLUR_RR", "Wheels", false);
    addComponent("RIM_BLUR_LR", "Wheels", false);
    addComponent("RIM_BLUR_RF", "Wheels", false);
    addComponent("RIM_BLUR_LF", "Wheels", false);
    addComponent("RIMBLR_RR", "Wheels", false);
    addComponent("RIMBLR_LR", "Wheels", false);
    addComponent("RIMBLR_RF", "Wheels", false);
    addComponent("RIMBLR_LF", "Wheels", false);

    // Tires
    addComponent("RR_TIRE", "Tires", true);
    addComponent("LR_TIRE", "Tires", true);
    addComponent("RF_TIRE", "Tires", true);
    addComponent("LF_TIRE", "Tires", true);
    addComponent("P9X_RR_TIRE", "Tires", false);
    addComponent("P9X_LR_TIRE", "Tires", false);
    addComponent("P9X_RF_TIRE", "Tires", false);
    addComponent("P9X_LF_TIRE", "Tires", false);

    // Brake discs
    addComponent("DISC_RR", "Brakes", false);
    addComponent("DISC_LR", "Brakes", false);
    addComponent("DISC_RF", "Brakes", false);
    addComponent("DISC_LF", "Brakes", false);

    // Steering
    addComponent("STEER_HR", "Steering", false);
    addComponent("STEER", "Steering", false);

    // Radiators
    addComponent("radiators", "Cooling", false);
    addComponent("radiator_l", "Cooling", false);
    addComponent("radiator_r", "Cooling", false);

    // Camera
    addComponent("tcam", "Camera", true);
    addComponent("tcam_low", "Camera", false);

    // Dashboard lights
    addComponent("COVER", "Dashboard", false);
    addComponent("COVERLED", "Dashboard", false);
    addComponent("RPM", "Dashboard", false);

    // External
    addComponent("numberplate", "External", false);
    addComponent("livery", "External", false);
    addComponent("pit", "External", false);
    addComponent("hlglo_ds", "External", false);

    // Miscellaneous
    addComponent("wcextra", "Misc", false);
    addComponent("LEDPSY", "Misc", false);
    addComponent("LED_REV", "Misc", false);
    addComponent("LEDLF", "Misc", false);

    return components;
}

QVariantMap FormatToolsQmlBridge::getComponentTree() {
    QVariantMap tree;

    // Build the tree structure matching real AC car hierarchy
    tree["SUSPRR"] = QVariantMap{{"ben_rr_susp", QVariantMap{{"ben_rr_susp01", QVariantMap()}}}};
    tree["SUSPLR"] = QVariantMap{{"ben_lr_susp", QVariantMap{{"ben_lr_susp01", QVariantMap()}}}};
    tree["SUPRF"] = QVariantMap{{"ben_rf_susp", QVariantMap{{"ben_rf_susp02", QVariantMap()}}}};
    tree["SUPLF"] = QVariantMap{{"ben_lf_susp", QVariantMap{{"ben_lf_susp02", QVariantMap()}}}};
    tree["SUSP_RR"] = QVariantMap{
        {"HUB_RR", QVariantMap{{"9X_RR_CAL_B", QVariantMap()}}},
        {"WHEEL_RR", QVariantMap{
            {"DISC_RR", QVariantMap{{"9x_rr_bd", QVariantMap{{"9x_rr_bd_SUB0", QVariantMap()}, {"9x_rr_bd_SUB1", QVariantMap()}}}}},
            {"RIM_RR", QVariantMap{{"BEN_RIM_RR", QVariantMap()}}},
            {"RR_TIRE", QVariantMap{{"P9X_RR_TIRE", QVariantMap{{"BEN_RR_TIRE", QVariantMap()}}}}},
            {"RIM_BLUR_RR", QVariantMap{{"BEN_RIM_BLUR_RR", QVariantMap()}, {"BEN_RIMBLR_RR", QVariantMap()}}}
        }},
        {"DIR_SUSPRR", QVariantMap()}
    };
    tree["SUSP_LR"] = QVariantMap{
        {"DIR_SUSPLR", QVariantMap()},
        {"HUB_LR", QVariantMap{{"9X_LR_CAL_B", QVariantMap()}}},
        {"WHEEL_LR", QVariantMap{
            {"DISC_LR", QVariantMap{{"9x_lr_bd", QVariantMap{{"9x_lr_bd_SUB0", QVariantMap()}, {"9x_lr_bd_SUB1", QVariantMap()}}}}},
            {"RIM_BLUR_LR", QVariantMap{{"BEN_RIM_BLUR_LR", QVariantMap()}, {"BEN_RIMBLR_LR", QVariantMap()}}},
            {"RIM_LR", QVariantMap{{"BEN_RIM_LR", QVariantMap()}}},
            {"LR_TIRE", QVariantMap{{"P9X_LR_TIRE", QVariantMap{{"BEN_LR_TIRE", QVariantMap()}}}}}
        }}
    };
    tree["HUB_RF"] = QVariantMap{
        {"SUSP_RF", QVariantMap{
            {"DIR_SUPRF", QVariantMap()},
            {"ben_rf_bcool", QVariantMap{{"ben_rf_bcool01", QVariantMap()}}},
            {"9X_RF_CAL_B", QVariantMap()}
        }},
        {"WHEEL_RF", QVariantMap{
            {"RF_TIRE", QVariantMap{{"P9X_RF_TIRE", QVariantMap{{"BEN_RF_TIRE", QVariantMap()}}}}},
            {"RIM_RF", QVariantMap{{"BEN_RIM_RF", QVariantMap()}}},
            {"RIM_BLUR_RF", QVariantMap{{"BEN_RIM_BLUR_RF", QVariantMap()}, {"BEN_RIMBLR_RF", QVariantMap()}}},
            {"DISC_RF", QVariantMap{{"9x_rf_bd", QVariantMap{{"9x_rf_bd_SUB0", QVariantMap()}, {"9x_rf_bd_SUB1", QVariantMap()}}}}}
        }}
    };
    tree["HUB_LF"] = QVariantMap{
        {"SUSP_LF", QVariantMap{
            {"DIR_SUPLF", QVariantMap()},
            {"ben_lf_bcool", QVariantMap{{"ben_lf_bcool01", QVariantMap()}}},
            {"9X_LF_CAL_B", QVariantMap()}
        }},
        {"WHEEL_LF", QVariantMap{
            {"RIM_BLUR_LF", QVariantMap{{"BEN_RIM_BLUR_LF", QVariantMap()}, {"BEN_RIMBLR_LF", QVariantMap()}}},
            {"LF_TIRE", QVariantMap{{"P9X_LF_TIRE", QVariantMap{{"BEN_LF_TIRE", QVariantMap()}}}}},
            {"RIM_LF", QVariantMap{{"BEN_RIM_LF", QVariantMap()}}},
            {"DISC_LF", QVariantMap{{"9x_lf_bd", QVariantMap{{"9x_lf_bd_SUB0", QVariantMap()}, {"9x_lf_bd_SUB1", QVariantMap()}}}}}
        }}
    };
    tree["STEER_HR"] = QVariantMap{{"STEER", QVariantMap{{"BEN_STEER", QVariantMap()}}}};
    tree["SUSPLFSUP"] = QVariantMap();
    tree["SUSPRFSUP"] = QVariantMap();
    tree["CINTURE_ON"] = QVariantMap{{"ben_driver_belt", QVariantMap()}};
    tree["CINTURE_OFF"] = QVariantMap{{"ben_belt", QVariantMap()}};
    tree["ben_seat"] = QVariantMap();
    tree["ben_airscope"] = QVariantMap();
    tree["ben_cwind"] = QVariantMap();
    tree["ben_dashboard"] = QVariantMap();
    tree["ben_engine"] = QVariantMap();
    tree["ben_engine_low"] = QVariantMap();
    tree["ben_exhaust"] = QVariantMap();
    tree["ben_fwing"] = QVariantMap();
    tree["ben_nosecone"] = QVariantMap();
    tree["ben_gearbox"] = QVariantMap();
    tree["hlglo_ds"] = QVariantMap();
    tree["ben_gearbox_parts"] = QVariantMap();
    tree["ben_motec_glass"] = QVariantMap();
    tree["ben_radiators"] = QVariantMap();
    tree["ben_rwing"] = QVariantMap();
    tree["ben_tcam"] = QVariantMap();
    tree["ben_display"] = QVariantMap();
    tree["COVER"] = QVariantMap{{"COVERLED", QVariantMap()}};
    tree["RPM"] = QVariantMap();
    tree["wcextra"] = QVariantMap();
    tree["LEDPSY"] = QVariantMap();
    tree["LED_REV"] = QVariantMap();
    tree["LEDLF"] = QVariantMap();
    tree["ben_coverengine_int"] = QVariantMap();
    tree["ben_coverengine"] = QVariantMap();
    tree["ben_cockpit_parts"] = QVariantMap();
    tree["ben_intcockpit"] = QVariantMap();
    tree["ben_body"] = QVariantMap();
    tree["ben_mirror_dust"] = QVariantMap();
    tree["ben_lmirror_in"] = QVariantMap();
    tree["ben_rmirror_in"] = QVariantMap();

    // Add RPM lights
    QVariantMap rpmTree;
    for (int i = 0; i <= 70; i++) {
        rpmTree[QString("RPM_LIGHT_%1").arg(i)] = QVariantMap();
    }
    tree["RPM"] = rpmTree;

    return tree;
}

QVariantMap FormatToolsQmlBridge::parseNodeHierarchy(const QString& nodeList) {
    QVariantMap tree;
    QStringList lines = nodeList.split("\n");
    QVector<QPair<int, QString>> stack; // indent level, name

    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;

        int indent = 0;
        QString trimmed = line;
        while (trimmed.startsWith("____")) {
            indent++;
            trimmed = trimmed.mid(4);
        }

        QString name = trimmed.split("=").first().trimmed();
        if (name.isEmpty()) continue;

        // Pop stack to current indent level
        while (!stack.isEmpty() && stack.last().first >= indent) {
            stack.pop_back();
        }

        // Add to tree
        if (stack.isEmpty()) {
            tree[name] = QVariantMap();
        } else {
            // Navigate to parent through stack and add child
            QVariantMap current = tree;
            for (int i = 0; i < stack.size() - 1; i++) {
                current = current[stack[i].second].toMap();
            }
            current[name] = QVariantMap();
            // Now rebuild the tree from the modified current
            QVariantMap parent = tree;
            for (int i = 0; i < stack.size() - 1; i++) {
                parent = parent[stack[i].second].toMap();
            }
            parent[stack.last().second] = current;
            tree = parent;
        }

        stack.append(QPair<int, QString>(indent, name));
    }

    return tree;
}

QString FormatToolsQmlBridge::buildNodeHierarchy(const QVariantMap& tree, int indent) {
    QString result;
    QString prefix(indent * 4, ' ');

    for (auto it = tree.begin(); it != tree.end(); ++it) {
        result += prefix + it.key() + "=0\n";
        if (it.value().isNull()) continue;
        QVariantMap children = it.value().toMap();
        if (!children.isEmpty()) {
            result += buildNodeHierarchy(children, indent + 1);
        }
    }

    return result;
}

QVariantList FormatToolsQmlBridge::generateComponentNames(const QString& carName, const QString& manufacturer) {
    QVariantList result;
    QString prefix = generatePrefix(manufacturer);
    QVariantList components = getComponentList();

    for (const auto& comp : components) {
        QVariantMap c = comp.toMap();
        QVariantMap entry;
        entry["name"] = QString("%1_%2").arg(prefix, c["name"].toString());
        entry["original"] = c["name"];
        entry["category"] = c["category"];
        entry["required"] = c["required"];
        result.append(entry);
    }

    return result;
}

QVariantMap FormatToolsQmlBridge::validateCarNaming(const QString& carName, const QVariantMap& componentTree) {
    QVariantMap result;
    QStringList errors;
    QStringList warnings;

    // Validate car name format: modder_year_manufacturer_carname
    QRegularExpression nameRegex("^[a-z0-9]+_\\d{4,}_[a-z0-9]+_[a-z0-9]+$");
    if (!nameRegex.match(carName.toLower()).hasMatch()) {
        errors.append("Car name must follow format: modder_year_manufacturer_carname");
    }

    // Check required components exist in tree
    QStringList requiredNodes = {"ben_body", "ben_dashboard", "ben_seat", "ben_engine", "ben_gearbox", "ben_tcam"};
    for (const QString& req : requiredNodes) {
        if (!componentTree.contains(req)) {
            errors.append("Missing required node: " + req);
        }
    }

    // Check for common issues
    if (!componentTree.contains("ben_fwing") && !componentTree.contains("fwing")) {
        warnings.append("No front wing found");
    }
    if (!componentTree.contains("ben_rwing") && !componentTree.contains("rwing")) {
        warnings.append("No rear wing found");
    }
    if (!componentTree.contains("WHEEL_RF") && !componentTree.contains("WHEEL_LF")) {
        warnings.append("No wheel system nodes found");
    }

    result["valid"] = errors.isEmpty();
    result["errors"] = errors;
    result["warnings"] = warnings;
    result["nodeCount"] = componentTree.size();

    return result;
}

QVariantMap FormatToolsQmlBridge::autoFixNaming(const QString& carName, const QVariantMap& componentTree) {
    QVariantMap result;
    QVariantMap fixedTree = componentTree;
    QStringList fixes;

    // Extract prefix from car name
    QStringList parts = carName.split("_");
    QString prefix = (parts.size() >= 3) ? generatePrefix(parts[2]) : "";

    // Add missing required nodes
    QStringList requiredNodes = {"ben_body", "ben_dashboard", "ben_seat", "ben_engine", "ben_gearbox", "ben_tcam"};
    for (const QString& req : requiredNodes) {
        if (!fixedTree.contains(req)) {
            fixedTree[req] = QVariantMap();
            fixes.append("Added missing: " + req);
        }
    }

    // Add wheel system if missing
    if (!fixedTree.contains("WHEEL_RF") && !fixedTree.contains("HUB_RF")) {
        QVariantMap hubRf;
        hubRf["SUSP_RF"] = QVariantMap();
        hubRf["WHEEL_RF"] = QVariantMap();
        fixedTree["HUB_RF"] = hubRf;
        fixes.append("Added missing: HUB_RF");
    }

    result["fixedTree"] = fixedTree;
    result["fixes"] = fixes;
    result["prefix"] = prefix;
    result["nodeCount"] = fixedTree.size();

    return result;
}

// ============================================================================
// Project Scaffolding
// ============================================================================

bool FormatToolsQmlBridge::createCarProject(const QString& basePath, const QString& carName, const QVariantMap& metadata) {
    QString carPath = basePath + "/" + carName;
    QDir().mkpath(carPath);

    // Create folder structure
    QStringList dirs = {
        "/body",
        "/textures",
        "/data",
        "/ai",
        "/sound",
        "/skin_01",
        "/skin_02"
    };

    for (const QString& dir : dirs) {
        QDir().mkpath(carPath + dir);
    }

    // Create car.ini
    QString manufacturer = metadata.value("manufacturer", "Unknown").toString();
    QString year = metadata.value("year", "2024").toString();
    QString prefix = generatePrefix(manufacturer);

    QFile iniFile(carPath + "/data/car.ini");
    if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&iniFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[INFO]\n";
        stream << "BRAND=" << metadata.value("brand", manufacturer).toString() << "\n";
        stream << "MODEL=" << metadata.value("model", carName).toString() << "\n";
        stream << "DESCRIPTION=" << metadata.value("description", "").toString() << "\n";
        stream << "AUTHOR=" << metadata.value("author", "").toString() << "\n";
        stream << "VERSION=1.0\n\n";
        stream << "[PHYSICS]\n";
        stream << "WHEELBASE=2.5\n";
        stream << "WIDTH=1.8\n";
        stream << "HEIGHT=1.0\n";
        stream << "MASS=700\n";
        stream << "MAX_POWER=700\n";
        stream << "MAX_TORQUE=500\n\n";
        stream << "[GRAPHICS]\n";
        stream << "WHEEL_SECTION=" << prefix << "_wheel_f_l\n";
        stream << "WHEEL_SECTION_R=" << prefix << "_wheel_r_l\n";
        stream << "BODY=" << prefix << "_body\n";
        stream << "COCKPIT=" << prefix << "_cockpit\n";
        iniFile.close();
    }

    // Create tyres.ini
    QFile tyresFile(carPath + "/data/tyres.ini");
    if (tyresFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&tyresFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[COMPOUND_DEFAULT]\n";
        stream << "DRY_TYRE=1\n";
        stream << "TREAD=0\n";
        stream << "INNER_LAYER=0\n";
        stream << "WIDTH_FRONT=245\n";
        stream << "WIDTH_REAR=305\n";
        stream << "RIM_DIAMETER=13\n\n";
        stream << "[TYRE_0]\n";
        stream << "NAME=Soft\n";
        stream << "GRIP=1.0\n";
        stream << "WEAR=1.0\n";
        tyresFile.close();
    }

    // Create engine.ini
    QFile engineFile(carPath + "/data/engine.ini");
    if (engineFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&engineFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[ENGINE_DATA]\n";
        stream << "IDLE_RPM=1000\n";
        stream << "LIMITER=15000\n";
        stream << "POWER_CURVE=750|10000,700|12000,650|14000,600|15000\n";
        stream << "RESPONSE=0.85\n";
        stream << "MIN_TORQUE=100\n";
        stream << "MAX_TORQUE=500\n";
        stream << "ENGINERotation=1\n";
        engineFile.close();
    }

    // Create layout.ini with component list
    QVariantList components = generateComponentNames(carName, manufacturer);
    QFile layoutFile(carPath + "/data/layout.ini");
    if (layoutFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&layoutFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[BODY]\n";
        for (const auto& comp : components) {
            QVariantMap c = comp.toMap();
            stream << c["name"].toString() << "=" << c["category"].toString() << "\n";
        }
        layoutFile.close();
    }

    // Create fbx.ini (persistence)
    QFile fbxFile(carPath + "/data/fbx.ini");
    if (fbxFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&fbxFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[NODES]\n";
        for (const auto& comp : components) {
            QVariantMap c = comp.toMap();
            stream << c["name"].toString() << "\n";
        }
        fbxFile.close();
    }

    emit statusMessage("Created car project: " + carPath);
    return true;
}

bool FormatToolsQmlBridge::createTrackProject(const QString& basePath, const QString& trackName, const QVariantMap& metadata) {
    QString trackPath = basePath + "/" + trackName;
    QDir().mkpath(trackPath);

    // Create folder structure
    QStringList dirs = {
        "/data",
        "/ai",
        "/map",
        "/textures",
        "/models",
        "/grass",
        "/surfaces"
    };

    for (const QString& dir : dirs) {
        QDir().mkpath(trackPath + dir);
    }

    // Create track.ini
    QFile iniFile(trackPath + "/data/track.ini");
    if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&iniFile);
        stream << "[HEADER]\n";
        stream << "VERSION=1\n\n";
        stream << "[INFO]\n";
        stream << "CONFIG_VERSION=7\n";
        stream << "NAME=" << metadata.value("name", trackName).toString() << "\n";
        stream << "DESCRIPTION=" << metadata.value("description", "").toString() << "\n";
        stream << "AUTHOR=" << metadata.value("author", "").toString() << "\n";
        stream << "COUNTRY=" << metadata.value("country", "Unknown").toString() << "\n";
        stream << "CITY=" << metadata.value("city", "Unknown").toString() << "\n";
        stream << "LENGTH=" << metadata.value("length", "0").toString() << "\n";
        stream << "WIDTH=" << metadata.value("width", "10").toString() << "\n";
        stream << "PITLANE_LENGTH=" << metadata.value("pitlane_length", "0").toString() << "\n";
        stream << "PITS=" << metadata.value("pits", "20").toString() << "\n\n";
        stream << "[SURFACE]\n";
        stream << "VIRTUAL_HEIGHT=0\n";
        stream << "DETAIL_UVMUL=1.0\n";
        iniFile.close();
    }

    // Create map.png placeholder (grid on dark background, proper UI size)
    QImage mapImage(512, 320, QImage::Format_RGB32);
    mapImage.fill(QColor("#1a1a2e"));
    QPainter pnt(&mapImage);
    pnt.setPen(QPen(QColor("#2a2a4e"), 1));
    for (int x = 0; x < 512; x += 32)
        pnt.drawLine(x, 0, x, 320);
    for (int y = 0; y < 320; y += 32)
        pnt.drawLine(0, y, 512, y);
    pnt.setPen(QColor("#555588"));
    QFont f = pnt.font(); f.setPointSize(14); pnt.setFont(f);
    pnt.drawText(mapImage.rect(), Qt::AlignCenter, "Track map will be generated\non export (F7)");
    pnt.end();
    mapImage.save(trackPath + "/map.png");

    emit statusMessage("Created track project: " + trackPath);
    return true;
}

QStringList FormatToolsQmlBridge::getRequiredFiles(const QString& projectType) {
    if (projectType == "car") {
        return {
            "data/car.ini",
            "data/tyres.ini",
            "data/engine.ini",
            "data/layout.ini",
            "data/fbx.ini",
            "data/ai.ini"
        };
    } else if (projectType == "track") {
        return {
            "data/track.ini",
            "map.png",
            "ai/fast_lane.ai",
            "ai/pit_lane.ai"
        };
    }
    return {};
}

// ============================================================================
// JSON Component Tree
// ============================================================================

static QVariantMap jsonObjToVariantMap(const QJsonObject& obj) {
    QVariantMap result;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        if (it.value().isObject()) {
            result[it.key()] = jsonObjToVariantMap(it.value().toObject());
        } else if (it.value().isArray()) {
            QJsonArray arr = it.value().toArray();
            QVariantList list;
            for (const auto& v : arr) {
                if (v.isObject()) {
                    list.append(jsonObjToVariantMap(v.toObject()));
                } else {
                    list.append(v.toVariant());
                }
            }
            result[it.key()] = list;
        } else {
            result[it.key()] = it.value().toVariant();
        }
    }
    return result;
}

static QJsonObject variantMapToJsonObj(const QVariantMap& map) {
    QJsonObject result;
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().canConvert<QVariantMap>()) {
            result[it.key()] = variantMapToJsonObj(it.value().toMap());
        } else if (it.value().canConvert<QVariantList>()) {
            QJsonArray arr;
            QVariantList list = it.value().toList();
            for (const auto& v : list) {
                if (v.canConvert<QVariantMap>()) {
                    arr.append(variantMapToJsonObj(v.toMap()));
                } else {
                    arr.append(QJsonValue::fromVariant(v));
                }
            }
            result[it.key()] = arr;
        } else {
            result[it.key()] = QJsonValue::fromVariant(it.value());
        }
    }
    return result;
}

bool FormatToolsQmlBridge::loadComponentTree(const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot open component tree: " + jsonPath);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit errorMessage("Invalid JSON in component tree");
        return false;
    }

    QJsonObject root = doc.object();
    if (root.contains("tree")) {
        m_componentTree = jsonObjToVariantMap(root["tree"].toObject());
    }
    if (root.contains("manufacturers")) {
        m_manufacturers = jsonObjToVariantMap(root["manufacturers"].toObject());
    }
    if (root.contains("categories")) {
        m_categories = jsonObjToVariantMap(root["categories"].toObject());
    }
    if (root.contains("required_nodes")) {
        m_requiredNodes.clear();
        for (const auto& v : root["required_nodes"].toArray()) {
            m_requiredNodes.append(v.toString());
        }
    }

    emit statusMessage("Loaded component tree: " + QString::number(m_componentTree.size()) + " nodes");
    return true;
}

bool FormatToolsQmlBridge::saveComponentTree(const QString& jsonPath) {
    QJsonObject root;
    root["tree"] = variantMapToJsonObj(m_componentTree);
    root["manufacturers"] = variantMapToJsonObj(m_manufacturers);
    root["categories"] = variantMapToJsonObj(m_categories);

    QJsonArray reqArr;
    for (const QString& s : m_requiredNodes) {
        reqArr.append(s);
    }
    root["required_nodes"] = reqArr;

    QJsonObject meta;
    meta["version"] = "1.0";
    meta["description"] = "AC car component hierarchy";
    root["meta"] = meta;

    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorMessage("Cannot write component tree: " + jsonPath);
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();

    emit statusMessage("Saved component tree to " + jsonPath);
    return true;
}

QStringList FormatToolsQmlBridge::getManufacturers() {
    if (m_manufacturers.isEmpty()) {
        // Load defaults
        m_manufacturers["benetton"] = "ben";
        m_manufacturers["ferrari"] = "fer";
        m_manufacturers["mclaren"] = "mcl";
        m_manufacturers["williams"] = "wil";
        m_manufacturers["mercedes"] = "mer";
        m_manufacturers["red_bull"] = "reb";
        m_manufacturers["aston_martin"] = "ast";
        m_manufacturers["alpine"] = "alp";
        m_manufacturers["haas"] = "has";
        m_manufacturers["alfa"] = "alf";
        m_manufacturers["alphatauri"] = "ata";
        m_manufacturers["racing_point"] = "rac";
        m_manufacturers["force_india"] = "frc";
        m_manufacturers["toro_rosso"] = "tor";
        m_manufacturers["lotus"] = "lot";
        m_manufacturers["caterham"] = "cat";
        m_manufacturers["marussia"] = "mar";
        m_manufacturers["hrt"] = "hrt";
        m_manufacturers["sauber"] = "sau";
        m_manufacturers["prost"] = "pro";
        m_manufacturers["jordan"] = "jor";
        m_manufacturers["minardi"] = "min";
        m_manufacturers["tyrell"] = "tyr";
        m_manufacturers["brabham"] = "bra";
        m_manufacturers["renault"] = "ren";
        m_manufacturers["porsche"] = "por";
        m_manufacturers["bmw"] = "bmw";
        m_manufacturers["audi"] = "aud";
        m_manufacturers["toyota"] = "toy";
        m_manufacturers["honda"] = "hon";
        m_manufacturers["nissan"] = "nis";
        m_manufacturers["mazda"] = "maz";
        m_manufacturers["subaru"] = "sub";
        m_manufacturers["mitsubishi"] = "mit";
        m_manufacturers["ford"] = "for";
        m_manufacturers["chevrolet"] = "che";
        m_manufacturers["dodge"] = "dod";
        m_manufacturers["chrysler"] = "chr";
        m_manufacturers["pontiac"] = "pon";
        m_manufacturers["buick"] = "bui";
        m_manufacturers["cadillac"] = "cad";
        m_manufacturers["lincoln"] = "lin";
        m_manufacturers["acura"] = "acr";
        m_manufacturers["infiniti"] = "inf";
        m_manufacturers["lexus"] = "lex";
        m_manufacturers["genesis"] = "gen";
        m_manufacturers["hyundai"] = "hyu";
        m_manufacturers["kia"] = "kia";
        m_manufacturers["volvo"] = "vol";
        m_manufacturers["jaguar"] = "jag";
        m_manufacturers["maserati"] = "mas";
        m_manufacturers["lamborghini"] = "lam";
        m_manufacturers["bugatti"] = "bug";
        m_manufacturers["pagani"] = "pag";
        m_manufacturers["koenigsegg"] = "koen";
        m_manufacturers["noble"] = "nob";
        m_manufacturers["radical"] = "rad";
        m_manufacturers["ultima"] = "ult";
        m_manufacturers["tvr"] = "tvr";
        m_manufacturers["de_tomaso"] = "dtm";
        m_manufacturers["seat"] = "sea";
        m_manufacturers["skoda"] = "sko";
        m_manufacturers["dacia"] = "dac";
        m_manufacturers["lada"] = "lad";
        m_manufacturers["gaz"] = "gaz";
        m_manufacturers["kamaz"] = "kam";
        m_manufacturers["mack"] = "mac";
        m_manufacturers["peterbilt"] = "pet";
        m_manufacturers["kenworth"] = "ken";
        m_manufacturers["freightliner"] = "fre";
        m_manufacturers["iveco"] = "ivc";
        m_manufacturers["man"] = "man";
        m_manufacturers["scania"] = "sca";
        m_manufacturers["daf"] = "daf";
    }
    return m_manufacturers.keys();
}

QVariantMap FormatToolsQmlBridge::getCategories() {
    if (m_categories.isEmpty()) {
        m_categories["Body"] = QVariantList{"ben_body", "ben_nosecone", "ben_cockpit_parts", "ben_intcockpit"};
        m_categories["Aero"] = QVariantList{"ben_fwing", "ben_rwing", "ben_airscope"};
        m_categories["Cockpit"] = QVariantList{"ben_dashboard", "ben_seat", "ben_display"};
        m_categories["Glass"] = QVariantList{"ben_cwind"};
        m_categories["Engine"] = QVariantList{"ben_engine", "ben_engine_low", "ben_exhaust"};
        m_categories["Drivetrain"] = QVariantList{"ben_gearbox", "ben_gearbox_parts"};
        m_categories["Suspension"] = QVariantList{"SUSPRR", "SUSPLR", "SUPRF", "SUPLF"};
        m_categories["Wheels"] = QVariantList{"WHEEL_RR", "WHEEL_LR", "WHEEL_RF", "WHEEL_LF"};
        m_categories["Camera"] = QVariantList{"ben_tcam"};
        m_categories["Steering"] = QVariantList{"STEER_HR"};
    }
    return m_categories;
}

// ============================================================================
// Hierarchy Import/Export
// ============================================================================

QVariantMap FormatToolsQmlBridge::importHierarchyIni(const QString& filePath) {
    QVariantMap tree;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorMessage("Cannot open hierarchy INI: " + filePath);
        return tree;
    }

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;

        if (line.contains('=')) {
            int eqPos = line.indexOf('=');
            QString name = line.left(eqPos).trimmed();
            tree[name] = QVariantMap();
        }
    }

    file.close();
    return tree;
}

bool FormatToolsQmlBridge::exportHierarchyIni(const QString& filePath, const QVariantMap& tree) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorMessage("Cannot write hierarchy INI: " + filePath);
        return false;
    }

    QTextStream stream(&file);
    stream << "; AC Car Node Hierarchy\n";
    stream << "; Generated by Format Tools\n\n";

    QStringList flat = flattenTree(tree);
    for (const QString& node : flat) {
        stream << node << "=0\n";
    }

    file.close();
    emit exportComplete(filePath);
    return true;
}

QVariantMap FormatToolsQmlBridge::importHierarchyJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorMessage("Cannot open hierarchy JSON: " + filePath);
        return QVariantMap();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit errorMessage("Invalid JSON in hierarchy file");
        return QVariantMap();
    }

    QJsonObject root = doc.object();
    if (root.contains("tree")) {
        return jsonObjToVariantMap(root["tree"].toObject());
    }
    return jsonObjToVariantMap(root);
}

bool FormatToolsQmlBridge::exportHierarchyJson(const QString& filePath, const QVariantMap& tree) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorMessage("Cannot write hierarchy JSON: " + filePath);
        return false;
    }

    QJsonObject root;
    root["tree"] = variantMapToJsonObj(tree);
    root["nodeCount"] = countNodes(tree);

    file.write(QJsonDocument(root).toJson());
    file.close();

    emit exportComplete(filePath);
    return true;
}

// ============================================================================
// Tree Manipulation
// ============================================================================

QVariantMap FormatToolsQmlBridge::addNode(const QVariantMap& tree, const QString& path, const QString& nodeName) {
    QVariantMap result = tree;
    QStringList parts = path.split("/");

    QVariantMap current = result;
    for (int i = 0; i < parts.size(); i++) {
        const QString& part = parts[i];
        if (part.isEmpty()) continue;
        if (!current.contains(part)) {
            current[part] = QVariantMap();
        }
        if (i < parts.size() - 1) {
            current = current[part].toMap();
        }
    }
    
    current[nodeName] = QVariantMap();
    return result;
}

QVariantMap FormatToolsQmlBridge::removeNode(const QVariantMap& tree, const QString& path) {
    QVariantMap result = tree;
    QStringList parts = path.split("/");

    QVariantMap current = result;
    for (int i = 0; i < parts.size() - 1; i++) {
        if (parts[i].isEmpty()) continue;
        if (!current.contains(parts[i])) return result;
        current = current[parts[i]].toMap();
    }

    if (!parts.last().isEmpty()) {
        current.remove(parts.last());
    }

    return result;
}

QVariantMap FormatToolsQmlBridge::renameNode(const QVariantMap& tree, const QString& path, const QString& newName) {
    QVariantMap result = tree;
    QStringList parts = path.split("/");

    QVariantMap current = result;
    for (int i = 0; i < parts.size() - 1; i++) {
        if (parts[i].isEmpty()) continue;
        if (!current.contains(parts[i])) return result;
        current = current[parts[i]].toMap();
    }

    QString oldName = parts.last();
    if (current.contains(oldName)) {
        QVariantMap children = current[oldName].toMap();
        current.remove(oldName);
        current[newName] = children;
    }

    return result;
}

int FormatToolsQmlBridge::countNodes(const QVariantMap& tree) {
    int count = 0;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        count++;
        if (it.value().canConvert<QVariantMap>()) {
            count += countNodes(it.value().toMap());
        }
    }
    return count;
}

QStringList FormatToolsQmlBridge::flattenTree(const QVariantMap& tree, const QString& prefix) {
    QStringList result;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        QString fullName = prefix.isEmpty() ? it.key() : prefix + "/" + it.key();
        result.append(fullName);
        if (it.value().canConvert<QVariantMap>()) {
            QVariantMap children = it.value().toMap();
            if (!children.isEmpty()) {
                result.append(flattenTree(children, fullName));
            }
        }
    }
    return result;
}

} // namespace ks

// ============================================================================
// FormatToolsModule
// ============================================================================

namespace ks {

FormatToolsModule::FormatToolsModule(QWidget* parent)
    : ModuleGuiBase(parent)
{
    setObjectName("FormatToolsModule");
}

bool FormatToolsModule::initialize()
{
    if (m_uiBuilt) return true;
    bool ok = ModuleGuiBase::initialize();
    LOG_INFO("FormatToolsModule", "Initializing Format Tools module");
    return ok;
}

void FormatToolsModule::shutdown()
{
    ModuleGuiBase::shutdown();
    LOG_INFO("FormatToolsModule", "Shutting down Format Tools module");
}

void FormatToolsModule::importFile(const QString& filePath)
{
    if (!filePath.isEmpty()) {
        LOG_INFO("FormatToolsModule", QString("Importing: %1").arg(filePath));
        emit FormatToolsQmlBridge::instance()->statusMessage("Importing: " + filePath);
        emit FormatToolsQmlBridge::instance()->importComplete(filePath, 0);
    }
}

void FormatToolsModule::exportFile(const QString& filePath)
{
    if (!filePath.isEmpty()) {
        LOG_INFO("FormatToolsModule", QString("Exporting: %1").arg(filePath));
        emit FormatToolsQmlBridge::instance()->statusMessage("Exporting: " + filePath);
        emit FormatToolsQmlBridge::instance()->exportComplete(filePath);
    }
}

void FormatToolsModule::buildUI()
{
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );

    // Tab 1: AI Line
    QWidget* aiTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(aiTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* importGroup = new QGroupBox("AI Line Import");
        QFormLayout* iform = new QFormLayout(importGroup);
        m_aiLinePathEdit = new QLineEdit();
        m_aiLinePathEdit->setPlaceholderText("Path to AI line file (.ai)");
        iform->addRow("File:", m_aiLinePathEdit);
        m_aiScalingSpin = new QDoubleSpinBox();
        m_aiScalingSpin->setRange(0.01, 100.0);
        m_aiScalingSpin->setValue(1.0);
        m_aiScalingSpin->setDecimals(3);
        iform->addRow("Scaling:", m_aiScalingSpin);
        QHBoxLayout* aiBtnLayout = new QHBoxLayout();
        m_importAiBtn = new QPushButton("Import AI Line");
        connect(m_importAiBtn, &QPushButton::clicked, this, &FormatToolsModule::onImportAiLine);
        aiBtnLayout->addWidget(m_importAiBtn);
        m_exportAiBtn = new QPushButton("Export AI Line");
        connect(m_exportAiBtn, &QPushButton::clicked, this, &FormatToolsModule::onExportAiLine);
        aiBtnLayout->addWidget(m_exportAiBtn);
        iform->addRow(aiBtnLayout);
        layout->addWidget(importGroup);

        m_aiLineOutput = new QTextEdit();
        m_aiLineOutput->setReadOnly(true);
        m_aiLineOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_aiLineOutput, 1);
    }
    m_tabWidget->addTab(aiTab, "AI Line");

    // Tab 2: CSV
    QWidget* csvTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(csvTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* csvGroup = new QGroupBox("CSV Border Tools");
        QFormLayout* cform = new QFormLayout(csvGroup);
        m_csvPathEdit = new QLineEdit();
        m_csvPathEdit->setPlaceholderText("Path to CSV file");
        cform->addRow("File:", m_csvPathEdit);
        m_csvScalingSpin = new QDoubleSpinBox();
        m_csvScalingSpin->setRange(0.001, 100.0);
        m_csvScalingSpin->setValue(0.01);
        m_csvScalingSpin->setDecimals(4);
        cform->addRow("Scaling:", m_csvScalingSpin);
        QHBoxLayout* csvBtnLayout = new QHBoxLayout();
        m_importCsvBtn = new QPushButton("Import CSV");
        connect(m_importCsvBtn, &QPushButton::clicked, this, &FormatToolsModule::onImportCsv);
        csvBtnLayout->addWidget(m_importCsvBtn);
        m_exportCsvBtn = new QPushButton("Export CSV");
        connect(m_exportCsvBtn, &QPushButton::clicked, this, &FormatToolsModule::onExportCsv);
        csvBtnLayout->addWidget(m_exportCsvBtn);
        cform->addRow(csvBtnLayout);
        layout->addWidget(csvGroup);

        m_csvOutput = new QTextEdit();
        m_csvOutput->setReadOnly(true);
        m_csvOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_csvOutput, 1);
    }
    m_tabWidget->addTab(csvTab, "CSV");

    // Tab 3: Camera & Overlay
    QWidget* camTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(camTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* camGroup = new QGroupBox("Camera.ini Tools");
        QVBoxLayout* cl = new QVBoxLayout(camGroup);
        m_camPathEdit = new QLineEdit();
        m_camPathEdit->setPlaceholderText("Path to camera.ini");
        cl->addWidget(m_camPathEdit);
        QHBoxLayout* camBtnLayout = new QHBoxLayout();
        m_importCamBtn = new QPushButton("Import Camera.ini");
        connect(m_importCamBtn, &QPushButton::clicked, this, &FormatToolsModule::onImportCameraIni);
        camBtnLayout->addWidget(m_importCamBtn);
        m_exportCamBtn = new QPushButton("Export Camera.ini");
        connect(m_exportCamBtn, &QPushButton::clicked, this, &FormatToolsModule::onExportCameraIni);
        camBtnLayout->addWidget(m_exportCamBtn);
        cl->addLayout(camBtnLayout);
        m_camOutput = new QTextEdit();
        m_camOutput->setReadOnly(true);
        m_camOutput->setMaximumHeight(100);
        m_camOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        cl->addWidget(m_camOutput);
        layout->addWidget(camGroup);

        QGroupBox* overlayGroup = new QGroupBox("Overlay.ini Tools");
        QVBoxLayout* ol = new QVBoxLayout(overlayGroup);
        m_overlayPathEdit = new QLineEdit();
        m_overlayPathEdit->setPlaceholderText("Path to overlay.ini");
        ol->addWidget(m_overlayPathEdit);
        QHBoxLayout* overlayBtnLayout = new QHBoxLayout();
        m_importOverlayBtn = new QPushButton("Import Overlay.ini");
        connect(m_importOverlayBtn, &QPushButton::clicked, this, &FormatToolsModule::onImportOverlayIni);
        overlayBtnLayout->addWidget(m_importOverlayBtn);
        m_exportOverlayBtn = new QPushButton("Export Overlay.ini");
        connect(m_exportOverlayBtn, &QPushButton::clicked, this, &FormatToolsModule::onExportOverlayIni);
        overlayBtnLayout->addWidget(m_exportOverlayBtn);
        ol->addLayout(overlayBtnLayout);
        m_overlayOutput = new QTextEdit();
        m_overlayOutput->setReadOnly(true);
        m_overlayOutput->setMaximumHeight(100);
        m_overlayOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        ol->addWidget(m_overlayOutput);
        layout->addWidget(overlayGroup);

        layout->addStretch();
    }
    m_tabWidget->addTab(camTab, "Camera/Overlay");

    // Tab 4: Naming Conventions
    QWidget* namingTab = new QWidget();
    {
        QVBoxLayout* layout = new QVBoxLayout(namingTab);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        QGroupBox* namingGroup = new QGroupBox("Naming Convention Generator");
        QFormLayout* nform = new QFormLayout(namingGroup);
        m_modderEdit = new QLineEdit();
        m_modderEdit->setPlaceholderText("Modder name");
        nform->addRow("Modder:", m_modderEdit);
        m_yearSpin = new QSpinBox();
        m_yearSpin->setRange(1900, 2100);
        m_yearSpin->setValue(2024);
        nform->addRow("Year:", m_yearSpin);
        m_manufacturerEdit = new QLineEdit();
        m_manufacturerEdit->setPlaceholderText("Manufacturer");
        nform->addRow("Manufacturer:", m_manufacturerEdit);
        m_carNameEdit = new QLineEdit();
        m_carNameEdit->setPlaceholderText("Car name");
        nform->addRow("Car Name:", m_carNameEdit);
        m_generateNamesBtn = new QPushButton("Generate Names");
        connect(m_generateNamesBtn, &QPushButton::clicked, this, &FormatToolsModule::onGenerateNames);
        nform->addRow(m_generateNamesBtn);
        layout->addWidget(namingGroup);

        m_namingOutput = new QTextEdit();
        m_namingOutput->setReadOnly(true);
        m_namingOutput->setStyleSheet("QTextEdit { background: #0a0a0a; color: #c8c8c8; font-family: Consolas; font-size: 10px; }");
        layout->addWidget(m_namingOutput, 1);
    }
    m_tabWidget->addTab(namingTab, "Naming");

    m_mainLayout->insertWidget(1, m_tabWidget, 1);
    m_uiBuilt = true;
}

void FormatToolsModule::onImportAiLine()
{
    QString path = m_aiLinePathEdit->text();
    if (path.isEmpty())
        path = selectFile("Select AI Line File", "AI Line (*.ai);;All Files (*)");
    if (path.isEmpty()) return;
    m_aiLinePathEdit->setText(path);

    auto* bridge = FormatToolsQmlBridge::instance();
    QVariantMap result = bridge->importAiLine(path, (float)m_aiScalingSpin->value());
    m_aiLineOutput->append(QString("Imported: %1 (%2 points)")
        .arg(path).arg(result.value("pointCount", 0).toInt()));
    log("AI Line imported: " + path);
}

void FormatToolsModule::onExportAiLine()
{
    QString path = QFileDialog::getSaveFileName(this, "Export AI Line File", QString(), "AI Line (*.ai);;All Files (*)");
    if (path.isEmpty()) return;

    auto* bridge = FormatToolsQmlBridge::instance();
    bool ok = bridge->exportAiLine(path, QVariantList(), (float)m_aiScalingSpin->value());
    if (ok) {
        m_aiLineOutput->append("Exported: " + path);
        logSuccess("AI Line exported: " + path);
    }
}

void FormatToolsModule::onImportCsv()
{
    QString path = selectFile("Select CSV File", "CSV (*.csv);;All Files (*)");
    if (path.isEmpty()) return;
    m_csvPathEdit->setText(path);

    auto* bridge = FormatToolsQmlBridge::instance();
    QVariantList result = bridge->importCsv(path, (float)m_csvScalingSpin->value());
    m_csvOutput->append(QString("Imported CSV: %1 (%2 rows)").arg(path).arg(result.size()));
    log(QString("CSV imported: %1 rows").arg(result.size()));
}

void FormatToolsModule::onExportCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Export CSV File", QString(), "CSV (*.csv);;All Files (*)");
    if (path.isEmpty()) return;

    auto* bridge = FormatToolsQmlBridge::instance();
    bridge->exportCsv(path, QVariantList(), (float)m_csvScalingSpin->value());
    m_csvOutput->append("Exported: " + path);
    logSuccess("CSV exported: " + path);
}

void FormatToolsModule::onImportCameraIni()
{
    QString path = selectFile("Select camera.ini", "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;
    m_camPathEdit->setText(path);

    auto* bridge = FormatToolsQmlBridge::instance();
    QVariantList cams = bridge->importCameraIni(path);
    m_camOutput->append(QString("Imported %1 cameras from %2").arg(cams.size()).arg(path));
    log(QString("Camera.ini imported: %1 cameras").arg(cams.size()));
}

void FormatToolsModule::onExportCameraIni()
{
    QString path = QFileDialog::getSaveFileName(this, "Export camera.ini", QString(), "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;

    auto* bridge = FormatToolsQmlBridge::instance();
    bridge->exportCameraIni(path, QVariantList());
    m_camOutput->append("Exported: " + path);
    logSuccess("Camera.ini exported: " + path);
}

void FormatToolsModule::onImportOverlayIni()
{
    QString path = selectFile("Select overlay.ini", "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;
    m_overlayPathEdit->setText(path);

    auto* bridge = FormatToolsQmlBridge::instance();
    QVariantList overlays = bridge->importOverlayIni(path);
    m_overlayOutput->append(QString("Imported %1 overlays from %2").arg(overlays.size()).arg(path));
    log(QString("Overlay.ini imported: %1 overlays").arg(overlays.size()));
}

void FormatToolsModule::onExportOverlayIni()
{
    QString path = QFileDialog::getSaveFileName(this, "Export overlay.ini", QString(), "INI Files (*.ini);;All Files (*)");
    if (path.isEmpty()) return;

    auto* bridge = FormatToolsQmlBridge::instance();
    bridge->exportOverlayIni(path, QVariantList());
    m_overlayOutput->append("Exported: " + path);
    logSuccess("Overlay.ini exported: " + path);
}

void FormatToolsModule::onGenerateNames()
{
    auto* bridge = FormatToolsQmlBridge::instance();
    QString carName = bridge->generateCarName(
        m_modderEdit->text(),
        m_yearSpin->value(),
        m_manufacturerEdit->text(),
        m_carNameEdit->text()
    );
    QString prefix = bridge->generatePrefix(m_manufacturerEdit->text());
    m_namingOutput->clear();
    m_namingOutput->append("Generated Car Name: " + carName);
    m_namingOutput->append("Manufacturer Prefix: " + prefix);
    m_namingOutput->append("");
    QVariantMap tree = bridge->getComponentTree();
    QVariantList components = bridge->generateComponentNames(carName, m_manufacturerEdit->text());
    m_namingOutput->append(QString("Component Count: %1").arg(components.size()));
    logSuccess("Names generated for: " + carName);
}

} // namespace ks
