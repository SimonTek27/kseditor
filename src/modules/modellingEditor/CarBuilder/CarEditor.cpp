#include "CarEditor.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QImage>
#include <cstring>
#include "plugins/simulators/kunos/assettocorsa/acFiles/KN5Parser.h"

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
    
    using namespace KN5Parser;
    KN5File kn5;
    kn5.filePath = kn5Path;

    // --- Materials ---
    Material bodyMat;
    bodyMat.id = 0;
    bodyMat.name = "body_material";
    bodyMat.shaderName = "ksCarPaint";
    bodyMat.type = Material::Type::Normal;
    bodyMat.properties["ksDiffuse"] = "0.8, 0.8, 0.8";
    bodyMat.properties["ksSpecular"] = "0.9";
    bodyMat.properties["ksRoughness"] = "0.3";
    bodyMat.properties["ksMetallic"] = "0.8";
    kn5.materials.append(bodyMat);

    Material wheelMat;
    wheelMat.id = 1;
    wheelMat.name = "wheel_material";
    wheelMat.shaderName = "ksTyre";
    wheelMat.type = Material::Type::Normal;
    wheelMat.properties["ksDiffuse"] = "0.1, 0.1, 0.1";
    wheelMat.properties["ksSpecular"] = "0.5";
    kn5.materials.append(wheelMat);

    Material glassMat;
    glassMat.id = 2;
    glassMat.name = "glass_material";
    glassMat.shaderName = "ksGlass";
    glassMat.type = Material::Type::Transparent;
    glassMat.alphaBlending = true;
    glassMat.alphaRef = 0.1f;
    glassMat.properties["ksDiffuse"] = "0.3, 0.3, 0.3";
    glassMat.properties["ksOpacity"] = "0.3";
    kn5.materials.append(glassMat);

    Material lightMat;
    lightMat.id = 3;
    lightMat.name = "light_material";
    lightMat.shaderName = "ksEmissive";
    lightMat.type = Material::Type::Additive;
    lightMat.properties["ksDiffuse"] = "1.0, 1.0, 1.0";
    lightMat.properties["ksEmissive"] = "1.0, 0.8, 0.2";
    kn5.materials.append(lightMat);

    // --- Generate meshes for each part ---
    quint32 nodeIdx = 0;
    quint32 vertIdx = 0;
    quint32 idxOffset = 0;

    // Helper lambda to create a box mesh for a part
    auto createBoxMesh = [&](const CarPart& part, quint32 matIdx) -> Mesh {
        Mesh mesh;
        mesh.name = part.name.toUpper().replace(" ", "_").toStdString().c_str();
        mesh.name = part.name.toUpper().replace(" ", "_");
        mesh.nodeIndex = nodeIdx++;
        mesh.castShadows = true;
        mesh.isVisible = part.visible;
        mesh.isTransparent = (matIdx == 2);
        mesh.materialType = (matIdx == 2) ? Mesh::MaterialType::Transparent : Mesh::MaterialType::Standard;

        auto& vl = mesh.vertexLayout;
        vl.attributes = {
            {AttributeType::Position,  0},
            {AttributeType::Normal,   12},
            {AttributeType::TexCoord0, 24}
        };
        vl.vertexSize = 32;

        // Generate box geometry scaled by part.scale
        float sx = part.scale.x() * 0.5f;
        float sy = part.scale.y() * 0.5f;
        float sz = part.scale.z() * 0.5f;

        // 8 box vertices (corners), 6 faces (2 triangles each)
        struct BoxVert { float x, y, z; float nx, ny, nz; float u, v; };
        BoxVert boxVerts[24] = {
            // Front face (Z+)
            {-sx,-sy, sz,  0, 0, 1,  0, 0}, { sx,-sy, sz,  0, 0, 1,  1, 0},
            { sx, sy, sz,  0, 0, 1,  1, 1}, {-sx, sy, sz,  0, 0, 1,  0, 1},
            // Back face (Z-)
            { sx,-sy,-sz,  0, 0,-1,  0, 0}, {-sx,-sy,-sz,  0, 0,-1,  1, 0},
            {-sx, sy,-sz,  0, 0,-1,  1, 1}, { sx, sy,-sz,  0, 0,-1,  0, 1},
            // Top face (Y+)
            {-sx, sy, sz,  0, 1, 0,  0, 0}, { sx, sy, sz,  0, 1, 0,  1, 0},
            { sx, sy,-sz,  0, 1, 0,  1, 1}, {-sx, sy,-sz,  0, 1, 0,  0, 1},
            // Bottom face (Y-)
            {-sx,-sy,-sz,  0,-1, 0,  0, 0}, { sx,-sy,-sz,  0,-1, 0,  1, 0},
            { sx,-sy, sz,  0,-1, 0,  1, 1}, {-sx,-sy, sz,  0,-1, 0,  0, 1},
            // Right face (X+)
            { sx,-sy, sz,  1, 0, 0,  0, 0}, { sx,-sy,-sz,  1, 0, 0,  1, 0},
            { sx, sy,-sz,  1, 0, 0,  1, 1}, { sx, sy, sz,  1, 0, 0,  0, 1},
            // Left face (X-)
            {-sx,-sy,-sz, -1, 0, 0,  0, 0}, {-sx,-sy, sz, -1, 0, 0,  1, 0},
            {-sx, sy, sz, -1, 0, 0,  1, 1}, {-sx, sy,-sz, -1, 0, 0,  0, 1}
        };

        mesh.vertexData.resize(24 * vl.vertexSize);
        char* dst = mesh.vertexData.data();
        for (const auto& bv : boxVerts) {
            float pos[3] = { bv.x + part.position.x(), bv.y + part.position.y(), bv.z + part.position.z() };
            float nrm[3] = { bv.nx, bv.ny, bv.nz };
            float uv[2]  = { bv.u, bv.v };
            std::memcpy(dst,      pos, 12);
            std::memcpy(dst + 12, nrm, 12);
            std::memcpy(dst + 24, uv,   8);
            dst += vl.vertexSize;
        }

        // 12 triangles (2 per face)
        quint16 boxIndices[36] = {
            0,1,2, 0,2,3,    4,5,6, 4,6,7,
            8,9,10, 8,10,11, 12,13,14, 12,14,15,
            16,17,18, 16,18,19, 20,21,22, 20,22,23
        };
        mesh.indexData.resize(36 * 2);
        std::memcpy(mesh.indexData.data(), boxIndices, 36 * 2);

        // Bounding box
        mesh.boundingMin = { -sx + part.position.x(), -sy + part.position.y(), -sz + part.position.z() };
        mesh.boundingMax = {  sx + part.position.x(),  sy + part.position.y(),  sz + part.position.z() };
        float dx = sx * 2, dy = sy * 2, dz = sz * 2;
        mesh.boundingRadius = std::sqrt(dx*dx + dy*dy + dz*dz) * 0.5f;

        SubMesh sub;
        sub.materialIndex = matIdx;
        sub.vertexOffset = vertIdx;
        sub.vertexCount = 24;
        sub.indexOffset = idxOffset;
        sub.indexCount = 36;
        sub.boundingMin.x = mesh.boundingMin.x;
        sub.boundingMin.y = mesh.boundingMin.y;
        sub.boundingMin.z = mesh.boundingMin.z;
        sub.boundingMax.x = mesh.boundingMax.x;
        sub.boundingMax.y = mesh.boundingMax.y;
        sub.boundingMax.z = mesh.boundingMax.z;
        mesh.subMeshes.append(sub);

        vertIdx += 24;
        idxOffset += 36;

        return mesh;
    };

    // Generate meshes for each part with appropriate material
    for (auto it = m_parts.begin(); it != m_parts.end(); ++it) {
        const CarPart& part = it.value();
        quint32 matIdx = 0;

        // Assign material based on part ID
        QString partId = part.id.toLower();
        if (partId.contains("wheel") || partId.contains("tyre") || partId.contains("tire")) {
            matIdx = 1;
        } else if (partId.contains("glass") || partId.contains("window") || partId.contains("windshield")) {
            matIdx = 2;
        } else if (partId.contains("light") || partId.contains("headlight") || partId.contains("taillight")) {
            matIdx = 3;
        }

        kn5.meshes.append(createBoxMesh(part, matIdx));
    }

    // Ensure at least body mesh exists
    if (kn5.meshes.isEmpty()) {
        CarPart defaultBody;
        defaultBody.id = "body";
        defaultBody.name = "Body";
        defaultBody.position = QVector3D(0, 0.5f, 0);
        defaultBody.scale = QVector3D(2.0f, 0.8f, 4.0f);
        kn5.meshes.append(createBoxMesh(defaultBody, 0));
    }

    // Write KN5 file
    if (!KN5Parser::writeKN5(kn5Path, kn5)) {
        qWarning() << "Failed to write KN5 file:" << kn5Path;
        return false;
    }

    // Write ui_car.json metadata
    QFile iniFile(outputPath + "/ui_car.json");
    if (iniFile.open(QIODevice::WriteOnly)) {
        QJsonObject ui;
        ui["name"] = m_config.name;
        ui["class"] = m_config.carClass;
        ui["brand"] = "Custom";
        ui["version"] = "1.0";
        ui["meshes"] = kn5.meshes.size();
        ui["materials"] = kn5.materials.size();
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

void CarEditor::loadPaint(const QString& path)
{
    qDebug() << "Loading paint from:" << path;

    PaintEditor::instance()->setCarPath(path);

    emit paintLoaded(path);
    emit carModified();
}

void CarEditor::savePaint(const QString& path)
{
    qDebug() << "Saving paint to:" << path;

    PaintEditor::instance()->saveCurrentSkin();

    emit paintSaved(path);
}

bool CarEditor::loadPaintTexture(const QString& skinPath)
{
    return PaintEditor::instance()->loadPaintTexture(skinPath);
}

bool CarEditor::savePaintTexture(const QImage& texture, const QString& skinPath)
{
    return PaintEditor::instance()->savePaintTexture(texture, skinPath);
}

QStringList CarEditor::getSkins() const
{
    return PaintEditor::instance()->getSkinNames();
}

bool CarEditor::setCurrentSkin(const QString& skinName)
{
    bool ok = PaintEditor::instance()->setCurrentSkin(skinName);
    if (ok) {
        m_paintConfig = PaintEditor::instance()->currentConfig();
        emit paintLoaded(skinName);
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