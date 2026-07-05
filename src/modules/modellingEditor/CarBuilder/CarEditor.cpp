#include "CarEditor.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QImage>

namespace ks {

CarEditor* CarEditor::s_instance = nullptr;

CarEditor::CarEditor(QObject* parent)
    : QObject(parent)
{
}

CarEditor* CarEditor::instance()
{
    if (!s_instance) {
        s_instance = new CarEditor();
    }
    return s_instance;
}

void CarEditor::newCar(const QString& name)
{
    m_currentCar = name;
    m_parts.clear();
    m_config.id = name.toLower().replace(" ", "_");
    m_config.name = name;
    m_config.carClass = "GT";
    m_config.maxSpeed = 300;
    m_config.acceleration = 0.8f;
    m_config.handling = 0.8f;
    m_config.braking = 0.8f;
    
    loadDefaultParts();
    
    qDebug() << "Created new car:" << name;
    emit carModified();
}

bool CarEditor::loadCar(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open car file:" << path;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();
    
    m_config.id = json["id"].toString();
    m_config.name = json["name"].toString();
    m_config.carClass = json["class"].toString();
    m_config.maxSpeed = json["maxSpeed"].toInt();
    m_config.acceleration = json["acceleration"].toDouble();
    m_config.handling = json["handling"].toDouble();
    m_config.braking = json["braking"].toDouble();
    
    m_parts.clear();
    QJsonObject partsObj = json["parts"].toObject();
    for (auto it = partsObj.begin(); it != partsObj.end(); ++it) {
        QJsonObject partObj = it.value().toObject();
        CarPart part;
        part.id = it.key();
        part.name = partObj["name"].toString();
        part.meshFile = partObj["mesh"].toString();
        part.parentId = partObj["parent"].toString();
        
        QJsonArray posArr = partObj["position"].toArray();
        if (posArr.size() == 3) {
            part.position = QVector3D(posArr[0].toDouble(), posArr[1].toDouble(), posArr[2].toDouble());
        }
        
        QJsonArray rotArr = partObj["rotation"].toArray();
        if (rotArr.size() == 3) {
            part.rotation = QVector3D(rotArr[0].toDouble(), rotArr[1].toDouble(), rotArr[2].toDouble());
        }
        
        QJsonArray scaleArr = partObj["scale"].toArray();
        if (scaleArr.size() == 3) {
            part.scale = QVector3D(scaleArr[0].toDouble(), scaleArr[1].toDouble(), scaleArr[2].toDouble());
        }
        
        part.visible = partObj["visible"].toBool(true);
        m_parts[part.id] = part;
    }
    
    m_currentCar = path;
    emit carLoaded(path);
    qDebug() << "Car loaded:" << path;
    return true;
}

bool CarEditor::saveCar(const QString& path)
{
    QJsonObject json;
    json["id"] = m_config.id;
    json["name"] = m_config.name;
    json["class"] = m_config.carClass;
    json["maxSpeed"] = m_config.maxSpeed;
    json["acceleration"] = m_config.acceleration;
    json["handling"] = m_config.handling;
    json["braking"] = m_config.braking;
    
    QJsonObject partsObj;
    for (auto it = m_parts.begin(); it != m_parts.end(); ++it) {
        QJsonObject partObj;
        partObj["name"] = it.value().name;
        partObj["mesh"] = it.value().meshFile;
        partObj["parent"] = it.value().parentId;
        partObj["position"] = QJsonArray({it.value().position.x(), it.value().position.y(), it.value().position.z()});
        partObj["rotation"] = QJsonArray({it.value().rotation.x(), it.value().rotation.y(), it.value().rotation.z()});
        partObj["scale"] = QJsonArray({it.value().scale.x(), it.value().scale.y(), it.value().scale.z()});
        partObj["visible"] = it.value().visible;
        partsObj[it.key()] = partObj;
    }
    json["parts"] = partsObj;
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save car file:" << path;
        return false;
    }
    
    file.write(QJsonDocument(json).toJson());
    emit carSaved(path);
    qDebug() << "Car saved:" << path;
    return true;
}

bool CarEditor::exportToAC(const QString& outputPath)
{
    QDir dir(outputPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString carId = m_config.id;
    
    QString kn5Path = outputPath + "/" + carId + ".kn5";
    qDebug() << "Exporting car to AC format:" << kn5Path;
    
    QFile iniFile(outputPath + "/ui_car.json");
    if (iniFile.open(QIODevice::WriteOnly)) {
        QJsonObject ui;
        ui["name"] = m_config.name;
        ui["class"] = m_config.carClass;
        ui["brand"] = "Custom";
        ui["version"] = "1.0";
        iniFile.write(QJsonDocument(ui).toJson());
        iniFile.close();
    }
    
    qDebug() << "Car exported successfully to:" << outputPath;
    return true;
}

void CarEditor::setCurrentPart(const QString& partId)
{
    if (m_currentPart != partId) {
        m_currentPart = partId;
        emit partChanged(partId);
    }
}

CarPart* CarEditor::getPart(const QString& partId)
{
    return m_parts.contains(partId) ? &m_parts[partId] : nullptr;
}

void CarEditor::addPart(const CarPart& part)
{
    m_parts[part.id] = part;
    qDebug() << "Added part:" << part.id;
    emit carModified();
}

void CarEditor::removePart(const QString& partId)
{
    if (m_parts.remove(partId)) {
        qDebug() << "Removed part:" << partId;
        emit carModified();
    }
}

void CarEditor::updatePartTransform(const QString& partId, const QVector3D& pos, const QVector3D& rot)
{
    if (m_parts.contains(partId)) {
        m_parts[partId].position = pos;
        m_parts[partId].rotation = rot;
        emit partChanged(partId);
        emit carModified();
    }
}

bool CarEditor::validateCar(QString& errorMsg)
{
    QVector<QString> errors = getValidationErrors();
    
    if (errors.isEmpty()) {
        errorMsg = "Car is valid";
        emit validationComplete(true, errors);
        return true;
    }
    
    errorMsg = errors.join("\n");
    emit validationComplete(false, errors);
    return false;
}

QVector<QString> CarEditor::getValidationErrors()
{
    QVector<QString> errors;
    
    if (m_config.name.isEmpty()) {
        errors.append("Car name is required");
    }
    
    QVector<QString> requiredParts = validateRequiredParts();
    errors.append(requiredParts);
    
    QVector<QString> meshErrors = validateMeshFiles();
    errors.append(meshErrors);
    
    QVector<QString> transformErrors = validateTransforms();
    errors.append(transformErrors);
    
    return errors;
}

CarConfig CarEditor::getConfig() const
{
    return m_config;
}

void CarEditor::setConfig(const CarConfig& config)
{
    m_config = config;
    emit carModified();
}

void CarEditor::loadLivery(const QString& path)
{
    qDebug() << "Loading livery from:" << path;

    LiveryEditor::instance()->setCarPath(path);

    emit liveryLoaded(path);
    emit carModified();
}

void CarEditor::saveLivery(const QString& path)
{
    qDebug() << "Saving livery to:" << path;

    LiveryEditor::instance()->saveCurrentSkin();

    emit liverySaved(path);
}

bool CarEditor::loadLiveryTexture(const QString& skinPath)
{
    return LiveryEditor::instance()->loadLiveryTexture(skinPath);
}

bool CarEditor::saveLiveryTexture(const QImage& texture, const QString& skinPath)
{
    return LiveryEditor::instance()->saveLiveryTexture(texture, skinPath);
}

QStringList CarEditor::getSkins() const
{
    return LiveryEditor::instance()->getSkinNames();
}

bool CarEditor::setCurrentSkin(const QString& skinName)
{
    bool ok = LiveryEditor::instance()->setCurrentSkin(skinName);
    if (ok) {
        m_liveryConfig = LiveryEditor::instance()->currentConfig();
        emit liveryLoaded(skinName);
    }
    return ok;
}

QJsonObject CarPhysicsData::toJson() const
{
    QJsonObject obj;
    obj["mass"] = mass;
    obj["fuelCapacity"] = fuelCapacity;
    obj["maxPower"] = maxPower;
    obj["maxRpm"] = maxRpm;
    obj["idleRpm"] = idleRpm;
    QJsonArray gears;
    for (int i = 0; i < 7; ++i) gears.append(gearRatio[i]);
    obj["gearRatios"] = gears;
    obj["suspensionTravel"] = suspensionTravel;
    obj["suspensionStiffness"] = suspensionStiffness;
    obj["dampingRate"] = dampingRate;
    obj["tireGrip"] = tireGrip;
    obj["brakePower"] = brakePower;
    obj["driveType"] = driveType;
    return obj;
}

CarPhysicsData CarPhysicsData::fromJson(const QJsonObject& obj)
{
    CarPhysicsData p;
    p.mass = obj["mass"].toDouble(p.mass);
    p.fuelCapacity = obj["fuelCapacity"].toDouble(p.fuelCapacity);
    p.maxPower = obj["maxPower"].toDouble(p.maxPower);
    p.maxRpm = obj["maxRpm"].toDouble(p.maxRpm);
    p.idleRpm = obj["idleRpm"].toDouble(p.idleRpm);
    QJsonArray gears = obj["gearRatios"].toArray();
    for (int i = 0; i < qMin(7, gears.size()); ++i)
        p.gearRatio[i] = gears[i].toDouble(p.gearRatio[i]);
    p.suspensionTravel = obj["suspensionTravel"].toDouble(p.suspensionTravel);
    p.suspensionStiffness = obj["suspensionStiffness"].toDouble(p.suspensionStiffness);
    p.dampingRate = obj["dampingRate"].toDouble(p.dampingRate);
    p.tireGrip = obj["tireGrip"].toDouble(p.tireGrip);
    p.brakePower = obj["brakePower"].toDouble(p.brakePower);
    p.driveType = obj["driveType"].toString(p.driveType);
    return p;
}

void CarEditor::loadPhysics(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "CarEditor: Cannot open physics file:" << path;
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (doc.isObject()) {
        m_physics = CarPhysicsData::fromJson(doc.object());
        qDebug() << "CarEditor: Loaded physics from" << path;
    }
}

void CarEditor::savePhysics(const QString& path)
{
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_physics.toJson()).toJson());
        file.close();
        qDebug() << "CarEditor: Saved physics to" << path;
    } else {
        qWarning() << "CarEditor: Cannot save physics to:" << path;
    }
}

void CarEditor::loadDefaultParts()
{
    CarPart body;
    body.id = "body";
    body.name = "Body";
    body.meshFile = "body.mesh";
    body.parentId = "";
    body.position = QVector3D(0, 0.5, 0);
    body.rotation = QVector3D(0, 0, 0);
    body.scale = QVector3D(1, 1, 1);
    m_parts["body"] = body;
    
    CarPart frontLeftWheel;
    frontLeftWheel.id = "wheel_fl";
    frontLeftWheel.name = "Front Left Wheel";
    frontLeftWheel.meshFile = "wheel.mesh";
    frontLeftWheel.parentId = "body";
    frontLeftWheel.position = QVector3D(-0.8, 0.3, 1.2);
    frontLeftWheel.rotation = QVector3D(0, 0, 0);
    frontLeftWheel.scale = QVector3D(1, 1, 1);
    m_parts["wheel_fl"] = frontLeftWheel;
    
    CarPart frontRightWheel;
    frontRightWheel.id = "wheel_fr";
    frontRightWheel.name = "Front Right Wheel";
    frontRightWheel.meshFile = "wheel.mesh";
    frontRightWheel.parentId = "body";
    frontRightWheel.position = QVector3D(0.8, 0.3, 1.2);
    frontRightWheel.rotation = QVector3D(0, 0, 0);
    frontRightWheel.scale = QVector3D(1, 1, 1);
    m_parts["wheel_fr"] = frontRightWheel;
    
    CarPart rearLeftWheel;
    rearLeftWheel.id = "wheel_rl";
    rearLeftWheel.name = "Rear Left Wheel";
    rearLeftWheel.meshFile = "wheel.mesh";
    rearLeftWheel.parentId = "body";
    rearLeftWheel.position = QVector3D(-0.8, 0.3, -1.2);
    rearLeftWheel.rotation = QVector3D(0, 0, 0);
    rearLeftWheel.scale = QVector3D(1, 1, 1);
    m_parts["wheel_rl"] = rearLeftWheel;
    
    CarPart rearRightWheel;
    rearRightWheel.id = "wheel_rr";
    rearRightWheel.name = "Rear Right Wheel";
    rearRightWheel.meshFile = "wheel.mesh";
    rearRightWheel.parentId = "body";
    rearRightWheel.position = QVector3D(0.8, 0.3, -1.2);
    rearRightWheel.rotation = QVector3D(0, 0, 0);
    rearRightWheel.scale = QVector3D(1, 1, 1);
    m_parts["wheel_rr"] = rearRightWheel;
    
    CarPart spoiler;
    spoiler.id = "spoiler";
    spoiler.name = "Spoiler";
    spoiler.meshFile = "spoiler.mesh";
    spoiler.parentId = "body";
    spoiler.position = QVector3D(0, 0.8, -1.8);
    spoiler.rotation = QVector3D(0, 0, 0);
    spoiler.scale = QVector3D(1, 1, 1);
    m_parts["spoiler"] = spoiler;
    
    CarPart engine;
    engine.id = "engine";
    engine.name = "Engine";
    engine.meshFile = "engine.mesh";
    engine.parentId = "body";
    engine.position = QVector3D(0, 0.6, 0.5);
    engine.rotation = QVector3D(0, 0, 0);
    engine.scale = QVector3D(1, 1, 1);
    m_parts["engine"] = engine;
}

QVector<QString> CarEditor::validateRequiredParts()
{
    QVector<QString> errors;
    QStringList required = {"body", "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr"};
    
    for (const QString& partId : required) {
        if (!m_parts.contains(partId)) {
            errors.append(QString("Missing required part: %1").arg(partId));
        }
    }
    
    return errors;
}

QVector<QString> CarEditor::validateMeshFiles()
{
    QVector<QString> warnings;
    
    for (auto it = m_parts.begin(); it != m_parts.end(); ++it) {
        if (it.value().meshFile.isEmpty()) {
            warnings.append(QString("Part '%1' has no mesh assigned").arg(it.key()));
        }
    }
    
    return warnings;
}

QVector<QString> CarEditor::validateTransforms()
{
    QVector<QString> warnings;
    
    for (auto it = m_parts.begin(); it != m_parts.end(); ++it) {
        if (it.value().position.isNull()) {
            warnings.append(QString("Part '%1' has default position").arg(it.key()));
        }
    }
    
    return warnings;
}

CarPart CarPartGenerator::createWheel(const QString& name, float diameter)
{
    CarPart wheel;
    wheel.id = name.toLower().replace(" ", "_");
    wheel.name = name;
    wheel.meshFile = "wheel.mesh";
    wheel.position = QVector3D(0, diameter * 0.5f, 0);
    wheel.scale = QVector3D(1, 1, 1);
    return wheel;
}

CarPart CarPartGenerator::createBody(const QString& name)
{
    CarPart body;
    body.id = name.toLower().replace(" ", "_");
    body.name = name;
    body.meshFile = "body.mesh";
    body.position = QVector3D(0, 0.5, 0);
    body.scale = QVector3D(1, 1, 1);
    return body;
}

CarPart CarPartGenerator::createSpoiler(const QString& name)
{
    CarPart spoiler;
    spoiler.id = name.toLower().replace(" ", "_");
    spoiler.name = name;
    spoiler.meshFile = "spoiler.mesh";
    spoiler.position = QVector3D(0, 0.8, -1.8);
    spoiler.rotation = QVector3D(-15, 0, 0);
    spoiler.scale = QVector3D(1, 1, 1);
    return spoiler;
}

CarPart CarPartGenerator::createEngine(const QString& name)
{
    CarPart engine;
    engine.id = name.toLower().replace(" ", "_");
    engine.name = name;
    engine.meshFile = "engine.mesh";
    engine.position = QVector3D(0, 0.6, 0.5);
    engine.scale = QVector3D(1, 1, 1);
    return engine;
}

CarPart CarPartGenerator::createInterior(const QString& name)
{
    CarPart interior;
    interior.id = name.toLower().replace(" ", "_");
    interior.name = name;
    interior.meshFile = "interior.mesh";
    interior.position = QVector3D(0, 0.7, 0);
    interior.scale = QVector3D(1, 1, 1);
    return interior;
}

} // namespace ks