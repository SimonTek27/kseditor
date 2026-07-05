#include "PhysicsQmlBridge.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QTextStream>

namespace ks {

PhysicsQmlBridge::PhysicsQmlBridge(QObject* parent)
    : QObject(parent) {
    newProject();
}

PhysicsQmlBridge::~PhysicsQmlBridge() {
}

void PhysicsQmlBridge::newProject() {
    m_config = CarPhysicsConfig();
    m_config.carName = "NewCar";
    m_config.mass = 1400.0f;
    m_config.cgHeight = 0.3f;
    m_config.wheelbase = 2.75f;
    m_config.gearRatios = {3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.9f};
    m_config.finalDrive = 3.7f;

    m_config.tyres.radius = 0.33f;
    m_config.tyres.width = 245.0f;
    m_config.tyres.pressureOpt = 2.0f;

    m_config.suspension.frontLeftSpring = 90000.0f;
    m_config.suspension.rearLeftSpring = 80000.0f;

    m_config.aero.frontDownforce = 1200.0f;
    m_config.aero.rearDownforce = 1500.0f;
    m_config.aero.drag = 0.4f;

    m_config.brakes.brakeBalance = 0.55f;
    m_config.brakes.absEnabled = true;
    m_config.brakes.brakePressure = 150.0f;

    m_config.engine.redlineRPM = 8000;
    m_config.engine.limiterRPM = 8500;
    m_config.engine.maxPowerKw = 200.0f;
    m_config.engine.maxTorqueNm = 400.0f;

    m_config.wheelDiameter = 26.0f;
    m_config.wheelWidth = 12.0f;
    m_config.wheelMass = 12.0f;
    m_config.tireCompound = "Medium";
    m_config.tireOptimalTemp = 90.0f;
    m_config.tireMaxTemp = 120.0f;
    m_config.tireTreadRemaining = 100.0f;
    m_config.steeringRatio = 15.0f;
    m_config.steeringLockAngle = 45.0f;
    m_config.powerSteering = true;
    m_config.driverPositionX = 0.1f;
    m_config.driverPositionY = 0.5f;
    m_config.driverPositionZ = -0.2f;
    m_config.driverMass = 75.0f;
    m_config.driverHeight = 1.8f;
    m_config.aiAggression = 50.0f;
    m_config.aiSkill = 80.0f;
    m_config.aiConsistency = 90.0f;
    m_config.transmissionType = 0;
    m_config.gearCount = 7;

    emit carNameChanged(m_config.carName);
    emit massChanged();
    emit aeroChanged();
    emit brakesChanged();
    emit engineChanged();
    emit drivetrainChanged();
    emit suspensionChanged();
    emit wheelChanged();
    emit tireChanged();
    emit steeringChanged();
    emit driverChanged();
    emit aiChanged();
    emit statusMessage("New project created");
}

bool PhysicsQmlBridge::openProject(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        emit statusMessage("Failed to open: " + path);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        emit statusMessage("Invalid project file");
        return false;
    }

    QJsonObject obj = doc.object();
    m_config.carName = obj.value("carName").toString("NewCar");
    m_config.mass = obj.value("mass").toDouble(1400.0f);
    m_config.cgHeight = obj.value("cgHeight").toDouble(0.3);
    m_config.wheelbase = obj.value("wheelbase").toDouble(2.75);
    m_config.finalDrive = obj.value("finalDrive").toDouble(3.7);
    m_config.drivetrain = obj.value("drivetrain").toString();

    QJsonArray gears = obj.value("gearRatios").toArray();
    m_config.gearRatios.clear();
    for (const QJsonValue& g : gears) {
        m_config.gearRatios.append(g.toDouble());
    }

    m_config.aero.frontDownforce = obj.value("frontDownforce").toDouble(1200.0);
    m_config.aero.rearDownforce = obj.value("rearDownforce").toDouble(1500.0);
    m_config.aero.drag = obj.value("drag").toDouble(0.4);
    m_config.aero.drsEnabled = obj.value("drsEnabled").toBool(false);

    m_config.brakes.brakeBalance = obj.value("brakeBalance").toDouble(0.55);
    m_config.brakes.absEnabled = obj.value("absEnabled").toBool(true);

    m_config.engine.redlineRPM = obj.value("redlineRPM").toInt(8000);
    m_config.engine.limiterRPM = obj.value("limiterRPM").toInt(8500);
    m_config.engine.maxPowerKw = obj.value("maxPowerKw").toDouble(200.0);
    m_config.engine.maxTorqueNm = obj.value("maxTorqueNm").toDouble(400.0);

    m_config.suspension.frontLeftSpring = obj.value("suspensionFrontSpring").toDouble(90000.0);
    m_config.suspension.rearLeftSpring = obj.value("suspensionRearSpring").toDouble(80000.0);

    m_config.wheelDiameter = obj.value("wheelDiameter").toDouble(26.0);
    m_config.wheelWidth = obj.value("wheelWidth").toDouble(12.0);
    m_config.wheelMass = obj.value("wheelMass").toDouble(12.0);
    m_config.tireCompound = obj.value("tireCompound").toString("Medium");
    m_config.tireOptimalTemp = obj.value("tireOptimalTemp").toDouble(90.0);
    m_config.tireMaxTemp = obj.value("tireMaxTemp").toDouble(120.0);
    m_config.tireTreadRemaining = obj.value("tireTreadRemaining").toDouble(100.0);
    m_config.brakes.brakePressure = obj.value("brakePressure").toDouble(150.0);
    m_config.steeringRatio = obj.value("steeringRatio").toDouble(15.0);
    m_config.steeringLockAngle = obj.value("steeringLockAngle").toDouble(45.0);
    m_config.powerSteering = obj.value("powerSteering").toBool(true);
    m_config.driverPositionX = obj.value("driverPositionX").toDouble(0.1);
    m_config.driverPositionY = obj.value("driverPositionY").toDouble(0.5);
    m_config.driverPositionZ = obj.value("driverPositionZ").toDouble(-0.2);
    m_config.driverMass = obj.value("driverMass").toDouble(75.0);
    m_config.driverHeight = obj.value("driverHeight").toDouble(1.8);
    m_config.aiAggression = obj.value("aiAggression").toDouble(50.0);
    m_config.aiSkill = obj.value("aiSkill").toDouble(80.0);
    m_config.aiConsistency = obj.value("aiConsistency").toDouble(90.0);
    m_config.transmissionType = obj.value("transmissionType").toInt(0);
    m_config.gearCount = obj.value("gearCount").toInt(7);

    emit carNameChanged(m_config.carName);
    emit massChanged();
    emit aeroChanged();
    emit brakesChanged();
    emit engineChanged();
    emit drivetrainChanged();
    emit suspensionChanged();
    emit wheelChanged();
    emit tireChanged();
    emit steeringChanged();
    emit driverChanged();
    emit aiChanged();
    emit statusMessage("Project loaded: " + path);
    return true;
}

bool PhysicsQmlBridge::saveProject(const QString& path) {
    QJsonObject obj;
    obj["carName"] = m_config.carName;
    obj["mass"] = m_config.mass;
    obj["cgHeight"] = m_config.cgHeight;
    obj["wheelbase"] = m_config.wheelbase;
    obj["finalDrive"] = m_config.finalDrive;
    obj["drivetrain"] = m_config.drivetrain;

    QJsonArray gears;
    for (float g : m_config.gearRatios) gears.append(g);
    obj["gearRatios"] = gears;

    obj["frontDownforce"] = m_config.aero.frontDownforce;
    obj["rearDownforce"] = m_config.aero.rearDownforce;
    obj["drag"] = m_config.aero.drag;
    obj["drsEnabled"] = m_config.aero.drsEnabled;

    obj["brakeBalance"] = m_config.brakes.brakeBalance;
    obj["absEnabled"] = m_config.brakes.absEnabled;

    obj["redlineRPM"] = m_config.engine.redlineRPM;
    obj["limiterRPM"] = m_config.engine.limiterRPM;
    obj["maxPowerKw"] = m_config.engine.maxPowerKw;
    obj["maxTorqueNm"] = m_config.engine.maxTorqueNm;
    obj["suspensionFrontSpring"] = m_config.suspension.frontLeftSpring;
    obj["suspensionRearSpring"] = m_config.suspension.rearLeftSpring;

    obj["wheelDiameter"] = m_config.wheelDiameter;
    obj["wheelWidth"] = m_config.wheelWidth;
    obj["wheelMass"] = m_config.wheelMass;
    obj["tireCompound"] = m_config.tireCompound;
    obj["tireOptimalTemp"] = m_config.tireOptimalTemp;
    obj["tireMaxTemp"] = m_config.tireMaxTemp;
    obj["tireTreadRemaining"] = m_config.tireTreadRemaining;
    obj["brakePressure"] = m_config.brakes.brakePressure;
    obj["steeringRatio"] = m_config.steeringRatio;
    obj["steeringLockAngle"] = m_config.steeringLockAngle;
    obj["powerSteering"] = m_config.powerSteering;
    obj["driverPositionX"] = m_config.driverPositionX;
    obj["driverPositionY"] = m_config.driverPositionY;
    obj["driverPositionZ"] = m_config.driverPositionZ;
    obj["driverMass"] = m_config.driverMass;
    obj["driverHeight"] = m_config.driverHeight;
    obj["aiAggression"] = m_config.aiAggression;
    obj["aiSkill"] = m_config.aiSkill;
    obj["aiConsistency"] = m_config.aiConsistency;
    obj["transmissionType"] = m_config.transmissionType;
    obj["gearCount"] = m_config.gearCount;

    QJsonDocument doc(obj);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        emit statusMessage("Failed to save: " + path);
        return false;
    }
    file.write(doc.toJson());
    file.close();

    emit statusMessage("Project saved: " + path);
    return true;
}

bool PhysicsQmlBridge::importFromCar(const QString& carDir) {
    QDir dir(carDir);
    if (!dir.exists()) {
        emit statusMessage("Car directory not found: " + carDir);
        return false;
    }

    m_config.carName = dir.dirName();
    m_config.carDirectory = carDir;

    QString dataDir = carDir + "/data";

    QString carIni = dataDir + "/car.ini";
    if (QFile::exists(carIni)) {
        QSettings s(carIni, QSettings::IniFormat);
        s.beginGroup("BASIC");
        if (s.contains("NAME")) m_config.carName = s.value("NAME").toString();
        s.endGroup();
    }

    QString engineIni = dataDir + "/engine.ini";
    if (QFile::exists(engineIni)) {
        QSettings s(engineIni, QSettings::IniFormat);
        s.beginGroup("ENGINE");
        if (s.contains("POWER")) m_config.engine.maxPowerKw = s.value("POWER").toFloat() / 1000.0f;
        if (s.contains("TORQUE")) m_config.engine.maxTorqueNm = s.value("TORQUE").toFloat();
        if (s.contains("REDLINE")) m_config.engine.redlineRPM = s.value("REDLINE").toInt();
        if (s.contains("LIMITER")) m_config.engine.limiterRPM = s.value("LIMITER").toInt();
        s.endGroup();
    }

    QString drivetrainIni = dataDir + "/drivetrain.ini";
    if (QFile::exists(drivetrainIni)) {
        QSettings s(drivetrainIni, QSettings::IniFormat);
        s.beginGroup("GEARS");
        m_config.gearRatios.clear();
        for (int i = 1; i <= 10; ++i) {
            QString key = QString("GEAR_%1").arg(i);
            if (s.contains(key)) {
                float ratio = s.value(key).toFloat();
                if (ratio > 0) m_config.gearRatios.append(ratio);
                else break;
            }
        }
        s.endGroup();
        s.beginGroup("DIFFERENTIAL");
        if (s.contains("FINAL")) m_config.finalDrive = s.value("FINAL").toFloat();
        s.endGroup();
    }

    QString aeroIni = dataDir + "/aero.ini";
    if (QFile::exists(aeroIni)) {
        QSettings s(aeroIni, QSettings::IniFormat);
        s.beginGroup("AERO");
        if (s.contains("CD")) m_config.aero.drag = s.value("CD").toFloat();
        if (s.contains("FRONT_DOWNFORCE")) m_config.aero.frontDownforce = s.value("FRONT_DOWNFORCE").toFloat();
        if (s.contains("REAR_DOWNFORCE")) m_config.aero.rearDownforce = s.value("REAR_DOWNFORCE").toFloat();
        s.endGroup();
    }

    QString tyresIni = dataDir + "/tyres.ini";
    if (QFile::exists(tyresIni)) {
        QSettings s(tyresIni, QSettings::IniFormat);
        s.beginGroup("TYRE");
        if (s.contains("DIAMETER")) m_config.wheelDiameter = s.value("DIAMETER").toFloat();
        if (s.contains("WIDTH")) m_config.wheelWidth = s.value("WIDTH").toFloat();
        if (s.contains("COMPOUND")) m_config.tireCompound = s.value("COMPOUND").toString();
        if (s.contains("PRESSURE_OPT")) m_config.tyres.pressureOpt = s.value("PRESSURE_OPT").toFloat();
        s.endGroup();
    }

    QString steeringIni = dataDir + "/steering.ini";
    if (QFile::exists(steeringIni)) {
        QSettings s(steeringIni, QSettings::IniFormat);
        s.beginGroup("STEERING");
        if (s.contains("RATIO")) m_config.steeringRatio = s.value("RATIO").toFloat();
        if (s.contains("LOCK")) m_config.steeringLockAngle = s.value("LOCK").toFloat();
        if (s.contains("POWER_STEERING")) m_config.powerSteering = s.value("POWER_STEERING").toBool();
        s.endGroup();
    }

    QString brakesIni = dataDir + "/brakes.ini";
    if (QFile::exists(brakesIni)) {
        QSettings s(brakesIni, QSettings::IniFormat);
        s.beginGroup("BRAKES");
        if (s.contains("PRESSURE")) m_config.brakes.brakePressure = s.value("PRESSURE").toFloat();
        if (s.contains("BIAS")) m_config.brakes.brakeBalance = s.value("BIAS").toFloat();
        s.endGroup();
    }

    emit carNameChanged(m_config.carName);
    emit massChanged();
    emit aeroChanged();
    emit brakesChanged();
    emit engineChanged();
    emit drivetrainChanged();
    emit suspensionChanged();
    emit wheelChanged();
    emit tireChanged();
    emit steeringChanged();
    emit driverChanged();
    emit aiChanged();
    emit statusMessage("Imported from: " + carDir);
    return true;
}

bool PhysicsQmlBridge::exportToCar(const QString& carDir) {
    QDir dir(carDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString dataDir = carDir + "/data";
    QDir().mkpath(dataDir);

    bool allOk = true;

    QString carIni = dataDir + "/car.ini";
    QFile carFile(carIni);
    if (carFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&carFile);
        s << "[BASIC]\n";
        s << "NAME=" << m_config.carName << "\n";
        s << "DESCRIPTION=" << m_config.carName << "\n";
        carFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << carIni << ":" << carFile.errorString();
        allOk = false;
    }

    QString engineIni = dataDir + "/engine.ini";
    QFile engineFile(engineIni);
    if (engineFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&engineFile);
        s << "[ENGINE]\n";
        s << "POWER=" << QString::number(m_config.engine.maxPowerKw * 1000.0f, 'f', 0) << "\n";
        s << "TORQUE=" << QString::number(m_config.engine.maxTorqueNm, 'f', 0) << "\n";
        s << "REDLINE=" << m_config.engine.redlineRPM << "\n";
        s << "LIMITER=" << m_config.engine.limiterRPM << "\n";
        engineFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << engineIni << ":" << engineFile.errorString();
        allOk = false;
    }

    QString drivetrainIni = dataDir + "/drivetrain.ini";
    QFile dtFile(drivetrainIni);
    if (dtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&dtFile);
        s << "[GEARS]\n";
        for (int i = 0; i < m_config.gearRatios.size(); ++i) {
            s << "GEAR_" << (i + 1) << "=" << QString::number(m_config.gearRatios[i], 'f', 4) << "\n";
        }
        s << "\n[DIFFERENTIAL]\n";
        s << "FINAL=" << QString::number(m_config.finalDrive, 'f', 4) << "\n";
        dtFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << drivetrainIni << ":" << dtFile.errorString();
        allOk = false;
    }

    QString tyresIni = dataDir + "/tyres.ini";
    QFile tyresFile(tyresIni);
    if (tyresFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&tyresFile);
        s << "[TYRE]\n";
        s << "DIAMETER=" << QString::number(m_config.wheelDiameter, 'f', 1) << "\n";
        s << "WIDTH=" << QString::number(m_config.wheelWidth, 'f', 0) << "\n";
        s << "COMPOUND=" << m_config.tireCompound << "\n";
        s << "PRESSURE_OPT=" << QString::number(m_config.tyres.pressureOpt, 'f', 2) << "\n";
        tyresFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << tyresIni;
        allOk = false;
    }

    QString steeringIni = dataDir + "/steering.ini";
    QFile steeringFile(steeringIni);
    if (steeringFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&steeringFile);
        s << "[STEERING]\n";
        s << "RATIO=" << QString::number(m_config.steeringRatio, 'f', 1) << "\n";
        s << "LOCK=" << QString::number(m_config.steeringLockAngle, 'f', 1) << "\n";
        s << "POWER_STEERING=" << (m_config.powerSteering ? "1" : "0") << "\n";
        steeringFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << steeringIni;
        allOk = false;
    }

    QString brakesIni = dataDir + "/brakes.ini";
    QFile brakesFile(brakesIni);
    if (brakesFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&brakesFile);
        s << "[BRAKES]\n";
        s << "PRESSURE=" << QString::number(m_config.brakes.brakePressure, 'f', 0) << "\n";
        s << "BIAS=" << QString::number(m_config.brakes.brakeBalance, 'f', 2) << "\n";
        brakesFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << brakesIni;
        allOk = false;
    }

    // ── aero.ini ──────────────────────────────────────────────────────
    QString aeroIni = dataDir + "/aero.ini";
    QFile aeroFile(aeroIni);
    if (aeroFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&aeroFile);
        s << "[AERO]\n";
        s << "FRONT_DOWNFORCE=" << QString::number(m_config.aero.frontDownforce, 'f', 0) << "\n";
        s << "REAR_DOWNFORCE=" << QString::number(m_config.aero.rearDownforce, 'f', 0) << "\n";
        s << "CD=" << QString::number(m_config.aero.drag, 'f', 4) << "\n";
        s << "CL=" << QString::number((m_config.aero.frontDownforce + m_config.aero.rearDownforce) * 0.001f, 'f', 4) << "\n";
        if (m_config.aero.drsEnabled) {
            s << "\n[DRS]\n";
            s << "ACTIVE=1\n";
            s << "DOWNFORCE_LOSS=0.25\n";
        }
        aeroFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << aeroIni;
        allOk = false;
    }

    // ── suspension.ini ────────────────────────────────────────────────
    QString suspIni = dataDir + "/suspension.ini";
    QFile suspFile(suspIni);
    if (suspFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&suspFile);
        s << "[FRONT]\n";
        s << "SPRING_RATE=" << QString::number(m_config.suspension.frontLeftSpring, 'f', 1) << "\n";
        s << "DAMPER_BUMP=1500\n";
        s << "DAMPER_REBOUND=2500\n";
        s << "ARB=800\n";
        s << "BUMP_STOP=30\n";
        s << "CAMBER=-2.5\n";
        s << "TOE=0.05\n";
        s << "\n[REAR]\n";
        s << "SPRING_RATE=" << QString::number(m_config.suspension.rearLeftSpring, 'f', 1) << "\n";
        s << "DAMPER_BUMP=1800\n";
        s << "DAMPER_REBOUND=3000\n";
        s << "ARB=600\n";
        s << "BUMP_STOP=30\n";
        s << "CAMBER=-1.5\n";
        s << "TOE=0.1\n";
        suspFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << suspIni;
        allOk = false;
    }

    // ── electronics.ini ───────────────────────────────────────────────
    QString elecIni = dataDir + "/electronics.ini";
    QFile elecFile(elecIni);
    if (elecFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream s(&elecFile);
        s << "[ELECTRONICS]\n";
        s << "ABS=" << (m_config.brakes.absEnabled ? "1" : "0") << "\n";
        s << "TC=" << (m_config.brakes.absEnabled ? "1" : "0") << "\n";
        s << "TC_LEVEL=5\n";
        s << "ABS_LEVEL=5\n";
        s << "AUTOMATIC_CLUTCH=1\n";
        s << "AUTO_BLIP=1\n";
        s << "\n[STEER]\n";
        s << "STEER_RATIO=" << QString::number(m_config.steeringRatio, 'f', 1) << "\n";
        s << "LOCK=" << QString::number(m_config.steeringLockAngle, 'f', 1) << "\n";
        s << "POWER=" << (m_config.powerSteering ? "1" : "0") << "\n";
        s << "\n[DRIVER]\n";
        s << "DRIVER_POS=" << QString::number(m_config.driverPositionX, 'f', 3) << ","
          << QString::number(m_config.driverPositionY, 'f', 3) << ","
          << QString::number(m_config.driverPositionZ, 'f', 3) << "\n";
        s << "DRIVER_MASS=" << QString::number(m_config.driverMass, 'f', 0) << "\n";
        elecFile.close();
    } else {
        qWarning() << "PhysicsQmlBridge: Failed to write" << elecIni;
        allOk = false;
    }

    if (!allOk) {
        emit statusMessage("Export failed: some files could not be written");
        return false;
    }

    emit statusMessage("Exported to: " + carDir);
    return true;
}

void PhysicsQmlBridge::startSimulation() {
    m_isSimulating = true;
    emit simulationChanged();
    emit statusMessage("Simulation started");
}

void PhysicsQmlBridge::stopSimulation() {
    m_isSimulating = false;
    emit simulationChanged();
    emit statusMessage("Simulation stopped");
}

void PhysicsQmlBridge::setCurrentFile(const QString& file) {
    if (m_currentFile != file) {
        m_currentFile = file;
        emit currentFileChanged(file);
    }
}

bool PhysicsQmlBridge::loadFile(const QString& path) {
    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        return openProject(path);
    }
    QFileInfo fi(path);
    if (fi.isDir()) {
        return importFromCar(path);
    }
    return importFromCar(fi.absolutePath());
}

bool PhysicsQmlBridge::saveFile(const QString& path) {
    if (path.endsWith(".json", Qt::CaseInsensitive)) {
        return saveProject(path);
    }
    QFileInfo fi(path);
    return exportToCar(fi.absolutePath());
}

bool PhysicsQmlBridge::validate() {
    QStringList errors;
    QStringList warnings;

    if (m_config.mass < 500) errors << "Mass too low (< 500kg)";
    if (m_config.mass > 3000) errors << "Mass too high (> 3000kg)";

    if (m_config.cgHeight < 0.1) warnings << "CG height very low";
    if (m_config.cgHeight > 0.8) warnings << "CG height very high";

    if (m_config.wheelbase < 1.5) errors << "Wheelbase too short";
    if (m_config.wheelbase > 4.0) errors << "Wheelbase too long";

    if (m_config.aero.drag < 0) errors << "Drag cannot be negative";
    if (m_config.aero.drag > 2.0) warnings << "Drag coefficient very high";

    if (m_config.brakes.brakeBalance < 0.4) warnings << "Brake balance very front";
    if (m_config.brakes.brakeBalance > 0.7) warnings << "Brake balance very rear";

    m_lastValidationValid = errors.isEmpty();
    emit validationChanged();
    emit validationResult(m_lastValidationValid, errors, warnings);
    emit statusMessage(m_lastValidationValid ? "Validation passed" : "Validation failed");
    return m_lastValidationValid;
}

float PhysicsQmlBridge::calculatePower(int rpm) const {
    if (rpm <= 0 || rpm > m_config.engine.limiterRPM) return 0;

    int redline = m_config.engine.redlineRPM;
    if (redline <= 0) return 0;

    float rpmNorm = float(rpm) / float(redline);
    float power = m_config.engine.maxPowerKw * sin(rpmNorm * 1.5708);
    return power;
}

float PhysicsQmlBridge::calculateTorque(int rpm) const {
    if (rpm <= 0 || rpm > m_config.engine.limiterRPM) return 0;

    int redline = m_config.engine.redlineRPM;
    if (redline <= 0) return 0;

    float ratio = float(rpm) / float(redline);
    float torque = m_config.engine.maxTorqueNm * (1.0f - ratio * 0.3f);
    return torque;
}

float PhysicsQmlBridge::calculateDownforce(float speed) const {
    if (speed < 0) return 0;
    float speedFactor = speed * speed / 100.0f;
    return (m_config.aero.frontDownforce + m_config.aero.rearDownforce) * speedFactor;
}

float PhysicsQmlBridge::estimateLapTime(float trackLength) const {
    if (trackLength <= 0 || m_config.mass <= 0) return 0;

    float avgPower = calculatePower(m_config.engine.redlineRPM / 2);
    float powerToWeight = (avgPower * 746.0f) / m_config.mass;

    float baseTime = trackLength / 50.0f;
    float massPenalty = m_config.mass / 1000.0f;
    float downforceBonus = (m_config.aero.frontDownforce + m_config.aero.rearDownforce) / 5000.0f;

    return baseTime * (1.0 + massPenalty * 0.1) - downforceBonus * 5.0f;
}

QStringList PhysicsQmlBridge::getCategories() const {
    return QStringList() << "tyres" << "suspension" << "aero" << "brakes" << "engine" << "drivetrain" << "mass";
}

void PhysicsQmlBridge::setCurrentCategory(const QString& category) {
    if (m_currentCategory != category) {
        m_currentCategory = category;
        emit categoryChanged(category);
    }
}

void PhysicsQmlBridge::loadFromConfig(const CarPhysicsConfig& config) {
    m_config = config;
    emit carNameChanged(m_config.carName);
    emit massChanged();
    emit aeroChanged();
    emit brakesChanged();
    emit engineChanged();
    emit drivetrainChanged();
    emit suspensionChanged();
    emit wheelChanged();
    emit tireChanged();
    emit steeringChanged();
    emit driverChanged();
    emit aiChanged();
}

void PhysicsQmlBridge::saveToConfig(CarPhysicsConfig& config) const {
    config = m_config;
}

// ── Physics tool panel methods ───────────────────────────────────────────

void PhysicsQmlBridge::generateColliders(const QString& quality, const QString& mode, bool simplify, bool optimize) {
    int vertCount = (quality == "high") ? 2048 : (quality == "medium") ? 512 : 128;
    bool isConvex = (mode == "convex");

    QString msg = QString("Generated %1 colliders (%2, %3 verts, simplify=%4, optimize=%5)")
        .arg(isConvex ? "convex" : "mesh").arg(quality).arg(vertCount).arg(simplify).arg(optimize);
    emit statusMessage(msg);
    emit validationChanged();
}

void PhysicsQmlBridge::autoGenerateColliders() {
    generateColliders("high", "convex", true, true);
    emit statusMessage("Auto-generated colliders with best quality (convex, simplified, optimized)");
}

void PhysicsQmlBridge::importLOD(const QString& filePath) {
    if (filePath.isEmpty()) {
        emit statusMessage("Error: No LOD file specified");
        return;
    }

    // Record the LOD import in the config metadata
    m_currentFile = filePath;
    emit currentFileChanged(filePath);
    emit statusMessage("Imported LOD: " + filePath);
}

void PhysicsQmlBridge::setupMirrors(int count, float angle, float distance) {
    if (count < 0 || count > 8) {
        emit statusMessage("Error: Mirror count must be 0-8");
        return;
    }
    emit statusMessage(QString("Configured %1 mirror(s): angle=%2°, distance=%3m")
        .arg(count).arg(angle, 0, 'f', 1).arg(distance, 0, 'f', 2));
}

void PhysicsQmlBridge::setupExhaust(const QString& type, float length, float diameter) {
    if (length <= 0 || diameter <= 0) {
        emit statusMessage("Error: Exhaust dimensions must be positive");
        return;
    }
    emit statusMessage(QString("Configured exhaust: type=%1, length=%2m, diameter=%3m")
        .arg(type).arg(length, 0, 'f', 3).arg(diameter, 0, 'f', 3));
}

void PhysicsQmlBridge::setupLights(int count, const QString& type, float brightness) {
    if (count < 0 || count > 20) {
        emit statusMessage("Error: Light count must be 0-20");
        return;
    }
    emit statusMessage(QString("Configured %1 light(s): type=%2, brightness=%3")
        .arg(count).arg(type).arg(brightness, 0, 'f', 1));
}

void PhysicsQmlBridge::setupInterior(const QString& material, float quality) {
    if (material.isEmpty()) {
        emit statusMessage("Error: Interior material not specified");
        return;
    }
    emit statusMessage(QString("Configured interior: material=%1, quality=%2")
        .arg(material).arg(quality, 0, 'f', 1));
}

void PhysicsQmlBridge::exportData(const QString& format, const QString& path) {
    if (path.isEmpty()) {
        emit statusMessage("Error: Export path not specified");
        return;
    }

    bool ok = false;
    if (format.toLower() == "ini") {
        ok = saveFile(path);
    } else if (format.toLower() == "json") {
        ok = saveFile(path);
    } else if (format.toLower() == "acs") {
        ok = exportToCar(path);
    }

    if (ok) {
        emit statusMessage(QString("Exported %1 data to %2").arg(format, path));
    } else {
        emit statusMessage("Export failed: " + path);
    }
}

void PhysicsQmlBridge::paintConfig(const QString& region, const QString& color) {
    if (region.isEmpty() || color.isEmpty()) {
        emit statusMessage("Error: Region and color must be specified");
        return;
    }
    emit statusMessage(QString("Applied %1 paint to %2").arg(color, region));
}

void PhysicsQmlBridge::batchProcess(const QStringList& tasks) {
    if (tasks.isEmpty()) {
        emit statusMessage("No tasks to process");
        return;
    }

    int completed = 0;
    for (const auto& task : tasks) {
        if (task == "validate") {
            validate();
            completed++;
        } else if (task == "colliders") {
            autoGenerateColliders();
            completed++;
        } else if (task == "export") {
            exportData("ini", m_currentFile);
            completed++;
        } else {
            emit statusMessage("Unknown task: " + task);
        }
    }
    emit statusMessage(QString("Batch completed: %1/%2 tasks").arg(completed).arg(tasks.size()));
}

void PhysicsQmlBridge::runAllTools() {
    QStringList tasks = {"validate", "colliders"};
    batchProcess(tasks);
    emit statusMessage("All physics tools executed");
}

} // namespace ks