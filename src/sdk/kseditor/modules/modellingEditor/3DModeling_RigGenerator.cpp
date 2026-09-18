#include "3DModeling_RigGenerator.h"
#include "3DModeling_io.h"
#include <QDebug>
#include <QJsonArray>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ks {

void RigGenerator::generateRig(QJsonObject& config) {
    QString type = config.value("type").toString();
    if (type == "tire") {
        generateTireRigFromParams(config);
    } else if (type == "engine") {
        generateEngineRigFromParams(config);
    } else {
        qWarning() << "[RigGenerator] Unknown rig type:" << type;
    }
}

bool RigGenerator::validateRig(const QJsonObject& config) const {
    if (config.isEmpty()) return false;
    QString type = config.value("type").toString();
    if (type.isEmpty()) return false;
    if (type == "tire") {
        QString error;
        return const_cast<RigGenerator*>(this)->validateTireRigParams(config, error);
    } else if (type == "engine") {
        QString error;
        return const_cast<RigGenerator*>(this)->validateEngineRigParams(config, error);
    }
    return false;
}

bool RigGenerator::generateTireRigFromParams(const QJsonObject& params)
{
    QString error;
    if (!validateTireRigParams(params, error)) {
        qWarning() << "[RigGenerator] Tire rig validation failed:" << error;
        return false;
    }

    double radius = params.value("radius").toDouble(0.33);
    double width = params.value("width").toDouble(0.2);
    int spokes = params.value("spokes").toInt(5);
    double springRate = params.value("springRate").toDouble(50000.0);
    double damping = params.value("damping").toDouble(3000.0);
    double camber = params.value("camber").toDouble(0.0);
    double toe = params.value("toe").toDouble(0.0);
    double caster = params.value("caster").toDouble(5.0);

    QJsonObject rigConfig;
    rigConfig["type"] = "tire";
    rigConfig["radius"] = radius;
    rigConfig["width"] = width;
    rigConfig["spokes"] = spokes;

    QJsonObject spring;
    spring["rate"] = springRate;
    spring["damping"] = damping;
    spring["maxTravel"] = params.value("maxTravel").toDouble(0.05);
    spring["preload"] = params.value("preload").toDouble(0.0);
    rigConfig["spring"] = spring;

    QJsonObject geometry;
    geometry["camber"] = camber;
    geometry["toe"] = toe;
    geometry["caster"] = caster;
    geometry["kingpinAngle"] = params.value("kingpinAngle").toDouble(14.0);
    geometry["scrubRadius"] = params.value("scrubRadius").toDouble(0.01);
    rigConfig["geometry"] = geometry;

    QJsonObject friction;
    friction["longitudinalPeak"] = params.value("longitudinalPeak").toDouble(1.8);
    friction["lateralPeak"] = params.value("lateralPeak").toDouble(1.6);
    friction["rollingResistance"] = params.value("rollingResistance").toDouble(0.015);
    friction["slipCurve"] = params.value("slipCurve").toDouble(8.0);
    rigConfig["friction"] = friction;

    QJsonObject wear;
    wear["rate"] = params.value("wearRate").toDouble(1.0);
    wear["grainThreshold"] = params.value("grainThreshold").toDouble(0.3);
    wear["blisterThreshold"] = params.value("blisterThreshold").toDouble(0.7);
    wear["heatGain"] = params.value("heatGain").toDouble(0.5);
    wear["heatLoss"] = params.value("heatLoss").toDouble(0.3);
    rigConfig["wear"] = wear;

    // Tire pressure affects contact patch
    QJsonObject pressure;
    pressure["value"] = params.value("pressure").toDouble(28.0);
    pressure["contactLength"] = params.value("contactLength").toDouble(0.15);
    pressure["contactWidth"] = params.value("contactWidth").toDouble(0.2);
    rigConfig["pressure"] = pressure;

    qDebug() << "[RigGenerator] Generated tire rig: radius=" << radius
             << "width=" << width << "spokes=" << spokes
             << "springRate=" << springRate << "damping=" << damping;

    emit rigGenerated("tire", rigConfig);
    return true;
}

bool RigGenerator::generateEngineRigFromParams(const QJsonObject& params)
{
    QString error;
    if (!validateEngineRigParams(params, error)) {
        qWarning() << "[RigGenerator] Engine rig validation failed:" << error;
        return false;
    }

    int cylinders = params.value("cylinders").toInt(8);
    QString layout = params.value("layout").toString("V");
    double displacement = params.value("displacement").toDouble(4.0);
    double bore = params.value("bore").toDouble(0.086);
    double stroke = params.value("stroke").toDouble(0.086);
    int redline = params.value("redline").toInt(8500);
    double idleRpm = params.value("idleRpm").toDouble(800.0);

    QJsonObject rigConfig;
    rigConfig["type"] = "engine";
    rigConfig["cylinders"] = cylinders;
    rigConfig["layout"] = layout;
    rigConfig["displacement"] = displacement;
    rigConfig["bore"] = bore;
    rigConfig["stroke"] = stroke;
    rigConfig["redline"] = redline;
    rigConfig["idleRpm"] = idleRpm;

    // Crankshaft configuration
    QJsonObject crank;
    crank["throw"] = stroke * 0.5;
    crank["firingOrder"] = params.value("firingOrder").toString("1-8-4-3-6-5-7-2");
    crank["balance"] = params.value("crankBalance").toDouble(0.5);
    crank["inertia"] = params.value("crankInertia").toDouble(0.05);
    rigConfig["crankshaft"] = crank;

    // Power curve (torque as function of RPM)
    QJsonArray torqueCurve;
    int maxTorqueRpm = params.value("maxTorqueRpm").toInt(5500);
    double maxTorque = params.value("maxTorque").toDouble(500.0);
    for (int rpm = 1000; rpm <= redline; rpm += 500) {
        QJsonObject point;
        point["rpm"] = rpm;
        // Simple torque curve: rises to peak then falls
        double t = (double)(rpm - 1000) / (maxTorqueRpm - 1000);
        double torque;
        if (rpm <= maxTorqueRpm) {
            torque = maxTorque * (1.0 - (1.0 - t) * (1.0 - t));
        } else {
            double t2 = (double)(rpm - maxTorqueRpm) / (redline - maxTorqueRpm);
            torque = maxTorque * (1.0 - t2 * t2 * 0.4);
        }
        point["torque"] = qMax(0.0, torque);
        torqueCurve.append(point);
    }
    rigConfig["torqueCurve"] = torqueCurve;

    // Drivetrain
    QJsonObject drivetrain;
    drivetrain["type"] = params.value("drivetrain").toString("RWD");
    drivetrain["gearCount"] = params.value("gearCount").toInt(6);
    QJsonArray gears;
    QJsonArray ratios = params.value("gearRatios").toArray();
    if (ratios.isEmpty()) {
        ratios = QJsonArray{3.82, 2.36, 1.68, 1.31, 1.0, 0.79};
    }
    for (const auto& r : ratios) {
        gears.append(r.toDouble());
    }
    drivetrain["gearRatios"] = gears;
    drivetrain["finalDrive"] = params.value("finalDrive").toDouble(3.73);
    drivetrain["shiftTime"] = params.value("shiftTime").toDouble(0.05);
    rigConfig["drivetrain"] = drivetrain;

    // Engine friction and losses
    QJsonObject friction;
    friction["pumpingLoss"] = params.value("pumpingLoss").toDouble(0.3);
    friction["mechanicalLoss"] = params.value("mechanicalLoss").toDouble(0.15);
    friction["accessoryLoss"] = params.value("accessoryLoss").toDouble(0.05);
    rigConfig["friction"] = friction;

    qDebug() << "[RigGenerator] Generated engine rig: cylinders=" << cylinders
             << "layout=" << layout << "displacement=" << displacement
             << "redline=" << redline << "maxTorque=" << maxTorque;

    emit rigGenerated("engine", rigConfig);
    return true;
}

QJsonObject RigGenerator::createDefaultTireRigParams()
{
    QJsonObject params;
    params["radius"] = 0.33;
    params["width"] = 0.245;
    params["spokes"] = 5;
    params["springRate"] = 50000.0;
    params["damping"] = 3000.0;
    params["maxTravel"] = 0.05;
    params["preload"] = 0.0;
    params["camber"] = -1.5;
    params["toe"] = 0.0;
    params["caster"] = 5.0;
    params["kingpinAngle"] = 14.0;
    params["scrubRadius"] = 0.01;
    params["pressure"] = 28.0;
    params["contactLength"] = 0.15;
    params["contactWidth"] = 0.2;
    params["longitudinalPeak"] = 1.8;
    params["lateralPeak"] = 1.6;
    params["rollingResistance"] = 0.015;
    params["slipCurve"] = 8.0;
    params["wearRate"] = 1.0;
    params["grainThreshold"] = 0.3;
    params["blisterThreshold"] = 0.7;
    params["heatGain"] = 0.5;
    params["heatLoss"] = 0.3;
    return params;
}

QJsonObject RigGenerator::createDefaultEngineRigParams()
{
    QJsonObject params;
    params["cylinders"] = 8;
    params["layout"] = "V";
    params["displacement"] = 4.0;
    params["bore"] = 0.086;
    params["stroke"] = 0.086;
    params["redline"] = 8500;
    params["idleRpm"] = 800;
    params["maxTorqueRpm"] = 5500;
    params["maxTorque"] = 500.0;
    params["firingOrder"] = "1-8-4-3-6-5-7-2";
    params["crankBalance"] = 0.5;
    params["crankInertia"] = 0.05;
    params["drivetrain"] = "RWD";
    params["gearCount"] = 6;
    params["finalDrive"] = 3.73;
    params["shiftTime"] = 0.05;
    params["pumpingLoss"] = 0.3;
    params["mechanicalLoss"] = 0.15;
    params["accessoryLoss"] = 0.05;
    return params;
}

bool RigGenerator::validateTireRigParams(const QJsonObject& params, QString& error)
{
    if (!params.contains("radius")) {
        error = "Missing required parameter: radius";
        return false;
    }
    double radius = params.value("radius").toDouble();
    if (radius <= 0 || radius > 1.0) {
        error = "radius must be between 0 and 1.0 meters";
        return false;
    }
    if (params.contains("width")) {
        double width = params.value("width").toDouble();
        if (width <= 0 || width > 0.5) {
            error = "width must be between 0 and 0.5 meters";
            return false;
        }
    }
    if (params.contains("springRate")) {
        double rate = params.value("springRate").toDouble();
        if (rate < 0) {
            error = "springRate must be non-negative";
            return false;
        }
    }
    if (params.contains("pressure")) {
        double pressure = params.value("pressure").toDouble();
        if (pressure < 10 || pressure > 60) {
            error = "pressure must be between 10 and 60 PSI";
            return false;
        }
    }
    return true;
}

bool RigGenerator::validateEngineRigParams(const QJsonObject& params, QString& error)
{
    if (!params.contains("cylinders")) {
        error = "Missing required parameter: cylinders";
        return false;
    }
    int cylinders = params.value("cylinders").toInt();
    if (cylinders < 1 || cylinders > 16) {
        error = "cylinders must be between 1 and 16";
        return false;
    }
    if (params.contains("displacement")) {
        double disp = params.value("displacement").toDouble();
        if (disp <= 0 || disp > 20.0) {
            error = "displacement must be between 0 and 20.0 liters";
            return false;
        }
    }
    if (params.contains("redline")) {
        int redline = params.value("redline").toInt();
        if (redline < 3000 || redline > 15000) {
            error = "redline must be between 3000 and 15000 RPM";
            return false;
        }
    }
    return true;
}

void RigGenerator::createTireSpringRig(int meshId, const QJsonObject& params)
{
    QJsonObject springConfig;
    springConfig["meshId"] = meshId;
    springConfig["type"] = "spring";
    springConfig["rate"] = params.value("springRate").toDouble(50000.0);
    springConfig["damping"] = params.value("damping").toDouble(3000.0);
    springConfig["maxTravel"] = params.value("maxTravel").toDouble(0.05);
    springConfig["preload"] = params.value("preload").toDouble(0.0);
    springConfig["restLength"] = params.value("restLength").toDouble(0.3);
    springConfig["minLength"] = params.value("minLength").toDouble(0.25);
    springConfig["maxLength"] = params.value("maxLength").toDouble(0.35);

    qDebug() << "[RigGenerator] Created tire spring rig for mesh" << meshId;
    emit rigCreated("tireSpring", meshId, springConfig);
}

void RigGenerator::createEngineCrankshaft(int meshId, const QJsonObject& params)
{
    int cylinders = params.value("cylinders").toInt(8);
    double throw_ = params.value("throw").toDouble(0.05);
    double rodLength = params.value("rodLength").toDouble(0.14);

    QJsonObject crankConfig;
    crankConfig["meshId"] = meshId;
    crankConfig["type"] = "crankshaft";
    crankConfig["cylinders"] = cylinders;
    crankConfig["throw"] = throw_;
    crankConfig["rodLength"] = rodLength;
    crankConfig["phaseAngle"] = 360.0 / cylinders;

    QJsonArray journalPositions;
    for (int i = 0; i < cylinders; ++i) {
        QJsonObject journal;
        journal["index"] = i;
        journal["offset"] = i * (rodLength * 2.0 + 0.02);
        journal["phase"] = i * crankConfig["phaseAngle"].toDouble();
        journalPositions.append(journal);
    }
    crankConfig["journalPositions"] = journalPositions;

    qDebug() << "[RigGenerator] Created crankshaft rig for mesh" << meshId
             << "cylinders=" << cylinders << "throw=" << throw_;
    emit rigCreated("crankshaft", meshId, crankConfig);
}

void RigGenerator::createPistonRig(int meshId, const QJsonObject& params)
{
    double bore = params.value("bore").toDouble(0.086);
    double stroke = params.value("stroke").toDouble(0.086);
    double compressionRatio = params.value("compressionRatio").toDouble(12.0);
    double rodLength = params.value("rodLength").toDouble(0.14);

    QJsonObject pistonConfig;
    pistonConfig["meshId"] = meshId;
    pistonConfig["type"] = "piston";
    pistonConfig["bore"] = bore;
    pistonConfig["stroke"] = stroke;
    pistonConfig["compressionRatio"] = compressionRatio;
    pistonConfig["rodLength"] = rodLength;
    pistonConfig["diameter"] = bore;
    pistonConfig["ringCount"] = params.value("ringCount").toInt(3);
    pistonConfig["wristPinOffset"] = params.value("wristPinOffset").toDouble(0.0);

    // Pin position is determined by crank throw and rod length
    double crankRadius = stroke * 0.5;
    pistonConfig["pinHeight"] = rodLength + crankRadius;
    pistonConfig["sweptVolume"] = M_PI * (bore * 0.5) * (bore * 0.5) * stroke;

    qDebug() << "[RigGenerator] Created piston rig for mesh" << meshId
             << "bore=" << bore << "stroke=" << stroke << "CR=" << compressionRatio;
    emit rigCreated("piston", meshId, pistonConfig);
}

void RigGenerator::createConnectingRodRig(int meshId, const QJsonObject& params)
{
    double length = params.value("length").toDouble(0.14);
    double bigEndDiameter = params.value("bigEndDiameter").toDouble(0.05);
    double smallEndDiameter = params.value("smallEndDiameter").toDouble(0.025);

    QJsonObject rodConfig;
    rodConfig["meshId"] = meshId;
    rodConfig["type"] = "connectingRod";
    rodConfig["length"] = length;
    rodConfig["bigEndDiameter"] = bigEndDiameter;
    rodConfig["smallEndDiameter"] = smallEndDiameter;
    rodConfig["bigEndWidth"] = params.value("bigEndWidth").toDouble(0.025);
    rodConfig["smallEndWidth"] = params.value("smallEndWidth").toDouble(0.02);
    rodConfig["rodWidth"] = params.value("rodWidth").toDouble(0.018);
    rodConfig["material"] = params.value("material").toString("steel");
    rodConfig["weight"] = params.value("weight").toDouble(0.5);

    qDebug() << "[RigGenerator] Created connecting rod rig for mesh" << meshId
             << "length=" << length;
    emit rigCreated("connectingRod", meshId, rodConfig);
}

// ============== Procedural Mesh Generation ==============

RigMeshData RigGenerator::generateTireMesh(double radius, double width, int segments)
{
    RigMeshData mesh;
    int vertsPerRing = segments;
    int rings = 8;

    for (int j = 0; j <= rings; ++j) {
        float v = static_cast<float>(j) / rings;
        float r = radius * (1.0f - v * 0.3f);
        float z = static_cast<float>(j) * width / rings - static_cast<float>(width) / 2.0f;

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(r * std::cos(u), r * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    return mesh;
}

RigMeshData RigGenerator::generatePistonMesh(double bore, double height, int segments)
{
    RigMeshData mesh;
    double r = bore * 0.5;
    int rings = 4;

    for (int j = 0; j <= rings; ++j) {
        float v = static_cast<float>(j) / rings;
        float z = static_cast<float>(-height / 2.0 + v * height);

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(r * std::cos(u), r * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    return mesh;
}

RigMeshData RigGenerator::generateConnectingRodMesh(double length, double bigEndDiam, double smallEndDiam)
{
    RigMeshData mesh;
    double bigR = bigEndDiam * 0.5;
    double smallR = smallEndDiam * 0.5;
    int segments = 16;
    int rings = 4;

    // Big end
    for (int j = 0; j <= rings; ++j) {
        float v = static_cast<float>(j) / rings;
        float z = static_cast<float>(-bigEndDiam / 2.0 + v * bigEndDiam);
        float r = bigR;

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(r * std::cos(u), r * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    // Shaft
    for (int j = 0; j <= rings; ++j) {
        float v = static_cast<float>(j) / rings;
        float z = static_cast<float>(-bigEndDiam / 2.0 + v * length);
        double r = bigR * (1.0 - v) + smallR * v;

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(r * std::cos(u), r * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    // Small end
    for (int j = 0; j <= rings; ++j) {
        float v = static_cast<float>(j) / rings;
        float z = static_cast<float>(length - smallEndDiam / 2.0 + v * smallEndDiam);
        float r = smallR;

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(r * std::cos(u), r * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    return mesh;
}

RigMeshData RigGenerator::generateCrankshaftMesh(double throw_, double rodLength, int cylinders)
{
    RigMeshData mesh;
    int segments = 16;

    // Main shaft
    double totalLen = cylinders * (rodLength * 2.0 + 0.02);
    double shaftR = 0.015;
    int shaftRings = cylinders * 4;

    for (int j = 0; j <= shaftRings; ++j) {
        float v = static_cast<float>(j) / shaftRings;
        float z = static_cast<float>(-totalLen / 2.0 + v * totalLen);

        for (int i = 0; i <= segments; ++i) {
            float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
            RigVertex vert;
            vert.position = QVector3D(shaftR * std::cos(u), shaftR * std::sin(u), z);
            vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
            mesh.append(vert);
        }
    }

    // Crank throws (journals)
    for (int c = 0; c < cylinders; ++c) {
        float zOffset = static_cast<float>(c) * (rodLength * 2.0f + 0.02f) - totalLen / 2.0f;
        float angle = static_cast<float>(c) * 360.0f / cylinders;
        float ax = throw_ * std::cos(angle * static_cast<float>(M_PI) / 180.0f);
        float ay = throw_ * std::sin(angle * static_cast<float>(M_PI) / 180.0f);

        for (int j = 0; j <= 4; ++j) {
            float v = static_cast<float>(j) / 4;
            float z = zOffset - 0.01f + v * 0.02f;

            for (int i = 0; i <= segments; ++i) {
                float u = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
                RigVertex vert;
                vert.position = QVector3D(ax + shaftR * std::cos(u), ay + shaftR * std::sin(u), z);
                vert.normal = QVector3D(std::cos(u), std::sin(u), 0);
                mesh.append(vert);
            }
        }
    }

    return mesh;
}

} // namespace ks
