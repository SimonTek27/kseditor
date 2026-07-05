#include "ReplayParser.h"
#include <QDataStream>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <cstring>
#include <cmath>

// ============================================================================
// ReplayParser implementation
// ============================================================================

QString ReplayParser::m_lastError;

ReplayParser::ReplayData ReplayParser::parse(const QString& replayPath, QString* error) {
    ReplayData data;
    data.filePath = replayPath;

    QFile file(replayPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open replay file: " + replayPath;
        if (error) *error = m_lastError;
        return data;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Parse header
    if (!parseHeader(stream, data)) {
        m_lastError = "Failed to parse replay header";
        if (error) *error = m_lastError;
        file.close();
        return data;
    }

    // Parse cars
    if (!parseCars(stream, data)) {
        m_lastError = "Failed to parse car data";
        if (error) *error = m_lastError;
        file.close();
        return data;
    }

    // Parse frames
    if (!parseFrames(stream, data)) {
        m_lastError = "Failed to parse frame data";
        if (error) *error = m_lastError;
        file.close();
        return data;
    }

    data.isValid = true;
    data.duration = data.frames.isEmpty() ? 0 : data.frames.last().timestamp - data.frames.first().timestamp;

    file.close();
    return data;
}

bool ReplayParser::exportToCSV(const ReplayData& data, const QString& csvPath, int carId) {
    QFile file(csvPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Cannot create CSV file: " + csvPath;
        return false;
    }

    QTextStream stream(&file);
    stream << "timestamp,car_id,position_x,position_y,position_z,";
    stream << "rotation_x,rotation_y,rotation_z,rotation_w,";
    stream << "velocity_x,velocity_y,velocity_z,";
    stream << "speed,rpm,throttle,brake,steering,gear,";
    stream << "tyre_temp_fl,tyre_temp_fr,tyre_temp_rl,tyre_temp_rr,";
    stream << "tyre_wear_fl,tyre_wear_fr,tyre_wear_rl,tyre_wear_rr,";
    stream << "tyre_pressure_fl,tyre_pressure_fr,tyre_pressure_rl,tyre_pressure_rr,";
    stream << "fuel,damage\n";

    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;

        stream << frame.timestamp << ","
               << frame.carId << ","
               << frame.position[0] << "," << frame.position[1] << "," << frame.position[2] << ","
               << frame.rotation[0] << "," << frame.rotation[1] << "," << frame.rotation[2] << "," << frame.rotation[3] << ","
               << frame.velocity[0] << "," << frame.velocity[1] << "," << frame.velocity[2] << ","
               << frame.speed << "," << frame.rpm << "," << frame.throttle << "," << frame.brake << ","
               << frame.steering << "," << frame.gear << ","
               << frame.tyreTemps[0] << "," << frame.tyreTemps[1] << "," << frame.tyreTemps[2] << "," << frame.tyreTemps[3] << ","
               << frame.tyreWear[0] << "," << frame.tyreWear[1] << "," << frame.tyreWear[2] << "," << frame.tyreWear[3] << ","
               << frame.tyrePressure[0] << "," << frame.tyrePressure[1] << "," << frame.tyrePressure[2] << "," << frame.tyrePressure[3] << ","
               << frame.fuel << "," << frame.damage << "\n";
    }

    file.close();
    return true;
}

bool ReplayParser::exportToJSON(const ReplayData& data, const QString& jsonPath) {
    QJsonObject root;

    // Session info
    QJsonObject session;
    session["track"] = data.session.trackName;
    session["config"] = data.session.trackConfig;
    session["type"] = data.session.sessionType;
    session["length"] = data.session.sessionLength;
    session["laps"] = data.session.lapsCount;
    session["ambientTemp"] = data.session.ambientTemp;
    session["roadTemp"] = data.session.roadTemp;
    session["weather"] = data.session.weather;
    root["session"] = session;

    // Cars
    QJsonArray carsArray;
    for (const ReplayCar& car : data.cars) {
        QJsonObject carObj;
        carObj["id"] = car.id;
        carObj["name"] = car.name;
        carObj["team"] = car.team;
        carObj["guid"] = car.guid;
        carObj["carModel"] = car.carModel;
        carObj["skin"] = car.skin;
        carObj["gridPosition"] = car.gridPosition;
        carObj["isPlayer"] = car.isPlayer;
        carsArray.append(carObj);
    }
    root["cars"] = carsArray;

    // Frames (sampled for JSON to keep file size manageable)
    QJsonArray framesArray;
    int sampleRate = qMax(1, data.frames.size() / 10000); // Max 10k frames in JSON

    for (int i = 0; i < data.frames.size(); i += sampleRate) {
        const ReplayFrame& frame = data.frames[i];
        QJsonObject frameObj;

        frameObj["t"] = frame.timestamp;
        frameObj["car"] = frame.carId;

        QJsonArray pos;
        pos.append(frame.position[0]);
        pos.append(frame.position[1]);
        pos.append(frame.position[2]);
        frameObj["pos"] = pos;

        frameObj["speed"] = frame.speed;
        frameObj["rpm"] = frame.rpm;
        frameObj["throttle"] = frame.throttle;
        frameObj["brake"] = frame.brake;
        frameObj["gear"] = frame.gear;

        framesArray.append(frameObj);
    }
    root["frames"] = framesArray;

    root["duration"] = data.duration;
    root["frameCount"] = data.frames.size();

    // Write file
    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = "Cannot create JSON file: " + jsonPath;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();

    return true;
}

float ReplayParser::calculateMaxSpeed(const ReplayData& data, int carId) {
    float maxSpeed = 0;
    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;
        if (frame.speed > maxSpeed) maxSpeed = frame.speed;
    }
    return maxSpeed;
}

float ReplayParser::calculateMaxRPM(const ReplayData& data, int carId) {
    float maxRPM = 0;
    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;
        if (frame.rpm > maxRPM) maxRPM = frame.rpm;
    }
    return maxRPM;
}

QVector<float> ReplayParser::calculateLapTimes(const ReplayData& data, int carId) {
    QVector<float> lapTimes;
    float lapStart = 0;
    bool inLap = false;

    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;

        // Detect lap boundary (simplified - would need finish line detection in production)
        if (!inLap && frame.speed > 10) {
            lapStart = frame.timestamp;
            inLap = true;
        } else if (inLap && frame.speed < 1 && (frame.timestamp - lapStart) > 30) {
            lapTimes.append(frame.timestamp - lapStart);
            inLap = false;
        }
    }

    return lapTimes;
}

float ReplayParser::calculateAverageSpeed(const ReplayData& data, int carId) {
    float totalSpeed = 0;
    int count = 0;

    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;
        totalSpeed += frame.speed;
        count++;
    }

    return count > 0 ? totalSpeed / count : 0;
}

QVector<float> ReplayParser::calculateBrakePoints(const ReplayData& data, int carId) {
    QVector<float> brakePoints;

    for (const ReplayFrame& frame : data.frames) {
        if (carId != -1 && frame.carId != carId) continue;
        if (frame.brake > 0.1f) {
            brakePoints.append(frame.timestamp);
        }
    }

    return brakePoints;
}

bool ReplayParser::isValidReplay(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray header = file.peek(256);
    file.close();

    // Check for replay magic bytes or header pattern
    // AC replay files typically start with specific header data
    return header.size() > 100; // Basic size check
}

bool ReplayParser::parseHeader(QDataStream& stream, ReplayData& data) {
    // Read replay header
    // The exact format depends on AC version, this is a simplified parser

    quint32 magic = 0;
    stream >> magic;

    // Check for AC replay magic (0x504C4152 = "RLAP")
    if (magic != 0x504C4152) {
        // Try reading as text header
        stream.device()->seek(0);
        QByteArray headerBytes = stream.device()->read(64);
        QString headerStr = QString::fromLatin1(headerBytes);

        if (!headerStr.contains("AC_REPLAY") && !headerStr.contains("replay")) {
            return false;
        }
        stream.device()->seek(0);
    }

    // Read session info
    quint32 sessionType = 0;
    stream >> sessionType;
    data.session.sessionType = sessionType;

    float sessionLength = 0;
    stream >> sessionLength;
    data.session.sessionLength = sessionLength;

    quint32 lapsCount = 0;
    stream >> lapsCount;
    data.session.lapsCount = lapsCount;

    float ambientTemp = 0;
    stream >> ambientTemp;
    data.session.ambientTemp = ambientTemp;

    float roadTemp = 0;
    stream >> roadTemp;
    data.session.roadTemp = roadTemp;

    // Read track name (fixed length string)
    char trackNameBuf[64];
    stream.readRawData(trackNameBuf, 63);
    trackNameBuf[63] = '\0';
    data.session.trackName = QString::fromLatin1(trackNameBuf);

    char trackConfigBuf[32];
    stream.readRawData(trackConfigBuf, 31);
    trackConfigBuf[31] = '\0';
    data.session.trackConfig = QString::fromLatin1(trackConfigBuf);

    return true;
}

bool ReplayParser::parseCars(QDataStream& stream, ReplayData& data) {
    quint32 carCount = 0;
    stream >> carCount;

    if (carCount > 100) { // Sanity check
        return false;
    }

    for (quint32 i = 0; i < carCount; ++i) {
        ReplayCar car;
        quint32 carId = 0;
        stream >> carId;
        car.id = carId;

        // Read car name (fixed length)
        char nameBuf[64];
        stream.readRawData(nameBuf, 63);
        nameBuf[63] = '\0';
        car.name = QString::fromLatin1(nameBuf);

        // Read team
        char teamBuf[64];
        stream.readRawData(teamBuf, 63);
        teamBuf[63] = '\0';
        car.team = QString::fromLatin1(teamBuf);

        // Read GUID
        char guidBuf[64];
        stream.readRawData(guidBuf, 63);
        guidBuf[63] = '\0';
        car.guid = QString::fromLatin1(guidBuf);

        // Read car model
        char modelBuf[64];
        stream.readRawData(modelBuf, 63);
        modelBuf[63] = '\0';
        car.carModel = QString::fromLatin1(modelBuf);

        // Read skin
        char skinBuf[64];
        stream.readRawData(skinBuf, 63);
        skinBuf[63] = '\0';
        car.skin = QString::fromLatin1(skinBuf);

        quint32 gridPos = 0;
        stream >> gridPos;
        car.gridPosition = gridPos;

        quint8 isPlayer = 0;
        stream >> isPlayer;
        car.isPlayer = (isPlayer != 0);

        data.cars.append(car);
    }

    return true;
}

bool ReplayParser::parseFrames(QDataStream& stream, ReplayData& data) {
    quint32 frameCount = 0;
    stream >> frameCount;

    if (frameCount > 10000000) { // Sanity check
        return false;
    }

    data.frames.reserve(frameCount);

    for (quint32 i = 0; i < frameCount; ++i) {
        if (stream.atEnd()) break;

        ReplayFrame frame;

        stream >> frame.timestamp;
        stream >> frame.carId;

        // Position
        stream >> frame.position[0] >> frame.position[1] >> frame.position[2];

        // Rotation (quaternion)
        stream >> frame.rotation[0] >> frame.rotation[1] >> frame.rotation[2] >> frame.rotation[3];

        // Velocity
        stream >> frame.velocity[0] >> frame.velocity[1] >> frame.velocity[2];

        // Physics
        stream >> frame.speed;
        stream >> frame.rpm;
        stream >> frame.throttle;
        stream >> frame.brake;
        stream >> frame.steering;

        quint32 gear = 0;
        stream >> gear;
        frame.gear = gear;

        // Tire data
        for (int t = 0; t < 4; ++t) stream >> frame.tyreTemps[t];
        for (int t = 0; t < 4; ++t) stream >> frame.tyreWear[t];
        for (int t = 0; t < 4; ++t) stream >> frame.tyrePressure[t];

        stream >> frame.fuel;
        stream >> frame.damage;

        data.frames.append(frame);
    }

    return true;
}

// ============================================================================
// ReplayAnalyzer implementation
// ============================================================================

ReplayAnalyzer::ReplayAnalyzer(const QString& replayPath) {
    m_data = ReplayParser::parse(replayPath);
}

bool ReplayAnalyzer::load() {
    return m_data.isValid;
}

float ReplayAnalyzer::getMaxSpeed() const {
    return ReplayParser::calculateMaxSpeed(m_data);
}

float ReplayAnalyzer::getMaxRPM() const {
    return ReplayParser::calculateMaxRPM(m_data);
}

float ReplayAnalyzer::getAverageSpeed() const {
    return ReplayParser::calculateAverageSpeed(m_data);
}

QVector<float> ReplayAnalyzer::getLapTimes() const {
    // Find player car
    int playerCarId = -1;
    for (const ReplayParser::ReplayCar& car : m_data.cars) {
        if (car.isPlayer) {
            playerCarId = car.id;
            break;
        }
    }
    return ReplayParser::calculateLapTimes(m_data, playerCarId);
}

QVector<float> ReplayAnalyzer::getSpeedTrace() const {
    QVector<float> trace;
    for (const ReplayParser::ReplayFrame& frame : m_data.frames) {
        trace.append(frame.speed);
    }
    return trace;
}

QVector<float> ReplayAnalyzer::getThrottleTrace() const {
    QVector<float> trace;
    for (const ReplayParser::ReplayFrame& frame : m_data.frames) {
        trace.append(frame.throttle);
    }
    return trace;
}

QVector<float> ReplayAnalyzer::getBrakeTrace() const {
    QVector<float> trace;
    for (const ReplayParser::ReplayFrame& frame : m_data.frames) {
        trace.append(frame.brake);
    }
    return trace;
}

bool ReplayAnalyzer::exportCSV(const QString& path, int carId) {
    return ReplayParser::exportToCSV(m_data, path, carId);
}

bool ReplayAnalyzer::exportJSON(const QString& path) {
    return ReplayParser::exportToJSON(m_data, path);
}
