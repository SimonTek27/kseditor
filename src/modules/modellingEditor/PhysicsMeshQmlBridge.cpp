#include "PhysicsMeshQmlBridge.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

namespace ks {

// ============================================================================
// PhysicsMeshQmlBridge Implementation
// ============================================================================

PhysicsMeshQmlBridge::PhysicsMeshQmlBridge(QObject* parent)
    : QObject(parent)
{
    connect(&m_generator, &PhysicsMeshGenerator::meshGenerated,
            this, &PhysicsMeshQmlBridge::onMeshGenerated);
    connect(&m_generator, &PhysicsMeshGenerator::error,
            this, &PhysicsMeshQmlBridge::onGenerationError);
    connect(&m_generator, &PhysicsMeshGenerator::generationProgress,
            this, &PhysicsMeshQmlBridge::generationProgress);
}

PhysicsMeshQmlBridge::~PhysicsMeshQmlBridge()
{
}

int PhysicsMeshQmlBridge::meshCount() const
{
    return m_generator.allMeshes().size();
}

QStringList PhysicsMeshQmlBridge::meshNames() const
{
    QStringList names;
    for (const auto& mesh : m_generator.allMeshes()) {
        names.append(mesh.name);
    }
    return names;
}

QVariantList PhysicsMeshQmlBridge::meshInfo() const
{
    QVariantList info;
    for (const auto& mesh : m_generator.allMeshes()) {
        QVariantMap meshData;
        meshData["name"] = mesh.name;
        meshData["type"] = static_cast<int>(mesh.type);
        meshData["vertexCount"] = mesh.vertices.size();
        meshData["faceCount"] = mesh.faces.size();
        meshData["centerX"] = mesh.centerOfMass.x();
        meshData["centerY"] = mesh.centerOfMass.y();
        meshData["centerZ"] = mesh.centerOfMass.z();
        info.append(meshData);
    }
    return info;
}

// ============================================================================
// QML-Callable Methods
// ============================================================================

void PhysicsMeshQmlBridge::generateFromMesh(const QVariant& vertices, const QVariant& faces, int collisionType)
{
    auto verts = qmlToVertices(vertices);
    auto faceData = qmlToFaces(faces);

    if (verts.isEmpty() || faceData.isEmpty()) {
        emit error("Input mesh data is empty");
        return;
    }

    PhysicsMeshGenerator::CollisionType type = intToCollisionType(collisionType);
    bool success = m_generator.generateCollisionMesh(verts, faceData, type);

    if (success) {
        m_isGenerated = true;
        emit generationStateChanged();
    }
}

void PhysicsMeshQmlBridge::generateChassis(const QVariant& vertices, const QVariant& faces)
{
    generateFromMesh(vertices, faces, static_cast<int>(PhysicsMeshGenerator::Chassis));
}

void PhysicsMeshQmlBridge::generateSuspension(const QVariant& vertices, const QVariant& faces)
{
    generateFromMesh(vertices, faces, static_cast<int>(PhysicsMeshGenerator::Suspension));
}

void PhysicsMeshQmlBridge::generateWheels(const QVariant& vertices, const QVariant& faces)
{
    generateFromMesh(vertices, faces, static_cast<int>(PhysicsMeshGenerator::Wheels));
}

void PhysicsMeshQmlBridge::generateInterior(const QVariant& vertices, const QVariant& faces)
{
    generateFromMesh(vertices, faces, static_cast<int>(PhysicsMeshGenerator::Interior));
}

void PhysicsMeshQmlBridge::generateExtra(const QVariant& vertices, const QVariant& faces, const QString& name)
{
    auto verts = qmlToVertices(vertices);
    auto faceData = qmlToFaces(faces);

    if (verts.isEmpty() || faceData.isEmpty()) {
        emit error("Input mesh data is empty");
        return;
    }

    bool success = m_generator.generateCollisionMesh(verts, faceData, PhysicsMeshGenerator::Extra, name);

    if (success) {
        m_isGenerated = true;
        emit generationStateChanged();
    }
}

void PhysicsMeshQmlBridge::generateFullCar(
    const QVariant& chassisVerts, const QVariant& chassisFaces,
    const QVariant& suspensionVerts, const QVariant& suspensionFaces,
    const QVariant& wheelVerts, const QVariant& wheelFaces)
{
    auto cv = qmlToVertices(chassisVerts);
    auto cf = qmlToFaces(chassisFaces);
    auto sv = qmlToVertices(suspensionVerts);
    auto sf = qmlToFaces(suspensionFaces);
    auto wv = qmlToVertices(wheelVerts);
    auto wf = qmlToFaces(wheelFaces);

    if (cv.isEmpty() && sv.isEmpty() && wv.isEmpty()) {
        emit error("No mesh data provided");
        return;
    }

    auto carSet = m_generator.generateFullCarCollision(cv, cf, sv, sf, wv, wf);

    m_isGenerated = true;
    emit generationStateChanged();
}

QVariantMap PhysicsMeshQmlBridge::getMeshData(const QString& meshName) const
{
    QVariantMap data;

    for (const auto& mesh : m_generator.allMeshes()) {
        if (mesh.name == meshName) {
            data["name"] = mesh.name;
            data["type"] = static_cast<int>(mesh.type);
            data["vertices"] = verticesToQml(mesh.vertices);
            data["faces"] = facesToQml(mesh.faces);
            data["centerX"] = mesh.centerOfMass.x();
            data["centerY"] = mesh.centerOfMass.y();
            data["centerZ"] = mesh.centerOfMass.z();
            data["boundingMinX"] = mesh.boundingBoxMin.x();
            data["boundingMinY"] = mesh.boundingBoxMin.y();
            data["boundingMinZ"] = mesh.boundingBoxMin.z();
            data["boundingMaxX"] = mesh.boundingBoxMax.x();
            data["boundingMaxY"] = mesh.boundingBoxMax.y();
            data["boundingMaxZ"] = mesh.boundingBoxMax.z();
            break;
        }
    }

    return data;
}

QVariantList PhysicsMeshQmlBridge::getMeshVertices(const QString& meshName) const
{
    for (const auto& mesh : m_generator.allMeshes()) {
        if (mesh.name == meshName) {
            return verticesToQml(mesh.vertices);
        }
    }
    return QVariantList();
}

QVariantList PhysicsMeshQmlBridge::getMeshFaces(const QString& meshName) const
{
    for (const auto& mesh : m_generator.allMeshes()) {
        if (mesh.name == meshName) {
            return facesToQml(mesh.faces);
        }
    }
    return QVariantList();
}

void PhysicsMeshQmlBridge::setSimplificationRatio(float ratio)
{
    m_generator.setSimplificationRatio(ratio);
    emit simplificationRatioChanged();
}

void PhysicsMeshQmlBridge::setMaxConvexParts(int maxParts)
{
    m_generator.setMaxConvexParts(maxParts);
    emit maxConvexPartsChanged();
}

void PhysicsMeshQmlBridge::setUseVHACD(bool use)
{
    m_generator.setUseVHACD(use);
    emit useVHACDChanged();
}

QVariantMap PhysicsMeshQmlBridge::validateMesh(const QString& meshName) const
{
    QVariantMap result;

    for (const auto& mesh : m_generator.allMeshes()) {
        if (mesh.name == meshName) {
            auto validation = m_generator.validateCollisionMesh(mesh);
            result["valid"] = validation.valid;
            result["totalVertices"] = validation.totalVertices;
            result["totalTriangles"] = validation.totalTriangles;
            result["totalVolume"] = validation.totalVolume;
            result["totalSurfaceArea"] = validation.totalSurfaceArea;
            result["warnings"] = validation.warnings;
            result["errors"] = validation.errors;
            break;
        }
    }

    return result;
}

QVariantMap PhysicsMeshQmlBridge::validateFullCar() const
{
    QVariantMap result;

    // Build a CarCollisionSet from current meshes
    PhysicsMeshGenerator::CarCollisionSet carSet;

    auto findMesh = [&](const QString& name) -> PhysicsMeshGenerator::PhysicsMesh {
        for (const auto& mesh : m_generator.allMeshes()) {
            if (mesh.name == name) return mesh;
        }
        return PhysicsMeshGenerator::PhysicsMesh();
    };

    carSet.chassis = findMesh("Chassis");
    carSet.suspensionFL = findMesh("Suspension_FL");
    carSet.suspensionFR = findMesh("Suspension_FR");
    carSet.suspensionRL = findMesh("Suspension_RL");
    carSet.suspensionRR = findMesh("Suspension_RR");
    carSet.wheelFL = findMesh("Wheel_FL");
    carSet.wheelFR = findMesh("Wheel_FR");
    carSet.wheelRL = findMesh("Wheel_RL");
    carSet.wheelRR = findMesh("Wheel_RR");
    carSet.interior = findMesh("Interior");

    auto validation = m_generator.validateFullCarSet(carSet);
    result["valid"] = validation.valid;
    result["totalVertices"] = validation.totalVertices;
    result["totalTriangles"] = validation.totalTriangles;
    result["totalVolume"] = validation.totalVolume;
    result["totalSurfaceArea"] = validation.totalSurfaceArea;
    result["warnings"] = validation.warnings;
    result["errors"] = validation.errors;

    return result;
}

bool PhysicsMeshQmlBridge::exportAcPhysicsMesh(const QString& filePath) const
{
    return m_generator.exportAcPhysicsMesh(filePath);
}

bool PhysicsMeshQmlBridge::exportObj(const QString& filePath, int collisionType) const
{
    return m_generator.exportObj(filePath, intToCollisionType(collisionType));
}

bool PhysicsMeshQmlBridge::exportJson(const QString& filePath) const
{
    QJsonObject json = m_generator.toJson();
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(json).toJson());
    return true;
}

bool PhysicsMeshQmlBridge::importJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    bool success = m_generator.fromJson(doc.object());

    if (success) {
        m_isGenerated = true;
        emit generationStateChanged();
    }

    return success;
}

void PhysicsMeshQmlBridge::clearAll()
{
    m_generator.clearAllMeshes();
    m_isGenerated = false;
    emit generationStateChanged();
    emit meshCountChanged();
}

QStringList PhysicsMeshQmlBridge::getCollisionTypeNames() const
{
    return {"Chassis", "Suspension", "Wheels", "Interior", "Extra", "GroundEffect", "AeroBody"};
}

int PhysicsMeshQmlBridge::getCollisionTypeValue(const QString& name) const
{
    if (name == "Chassis") return 0;
    if (name == "Suspension") return 1;
    if (name == "Wheels") return 2;
    if (name == "Interior") return 3;
    if (name == "Extra") return 4;
    if (name == "GroundEffect") return 5;
    if (name == "AeroBody") return 6;
    return 0;
}

// ============================================================================
// Private Slots
// ============================================================================

void PhysicsMeshQmlBridge::onMeshGenerated()
{
    emit meshCountChanged();
    emit meshGenerated(QString());
}

void PhysicsMeshQmlBridge::onGenerationError(const QString& message)
{
    emit error(message);
}

// ============================================================================
// Private Helper Methods
// ============================================================================

QVector<PhysicsMeshGenerator::PhysicsVertex> PhysicsMeshQmlBridge::qmlToVertices(const QVariant& data) const
{
    QVector<PhysicsMeshGenerator::PhysicsVertex> vertices;

    QVariantList vertList = data.toList();
    for (const auto& vertVar : vertList) {
        QVariantMap vertMap = vertVar.toMap();
        PhysicsMeshGenerator::PhysicsVertex v;
        v.position = QVector3D(vertMap["x"].toFloat(), vertMap["y"].toFloat(), vertMap["z"].toFloat());
        if (vertMap.contains("nx")) {
            v.normal = QVector3D(vertMap["nx"].toFloat(), vertMap["ny"].toFloat(), vertMap["nz"].toFloat());
        }
        vertices.append(v);
    }

    return vertices;
}

QVector<PhysicsMeshGenerator::PhysicsFace> PhysicsMeshQmlBridge::qmlToFaces(const QVariant& data) const
{
    QVector<PhysicsMeshGenerator::PhysicsFace> faces;

    QVariantList faceList = data.toList();
    for (const auto& faceVar : faceList) {
        QVariantMap faceMap = faceVar.toMap();
        PhysicsMeshGenerator::PhysicsFace f;
        f.v0 = faceMap["v0"].toInt();
        f.v1 = faceMap["v1"].toInt();
        f.v2 = faceMap["v2"].toInt();
        if (faceMap.contains("materialId")) {
            f.materialId = faceMap["materialId"].toInt();
        }
        faces.append(f);
    }

    return faces;
}

QVariantList PhysicsMeshQmlBridge::verticesToQml(const QVector<PhysicsMeshGenerator::PhysicsVertex>& vertices) const
{
    QVariantList result;

    for (const auto& v : vertices) {
        QVariantMap vertMap;
        vertMap["x"] = v.position.x();
        vertMap["y"] = v.position.y();
        vertMap["z"] = v.position.z();
        vertMap["nx"] = v.normal.x();
        vertMap["ny"] = v.normal.y();
        vertMap["nz"] = v.normal.z();
        result.append(vertMap);
    }

    return result;
}

QVariantList PhysicsMeshQmlBridge::facesToQml(const QVector<PhysicsMeshGenerator::PhysicsFace>& faces) const
{
    QVariantList result;

    for (const auto& f : faces) {
        QVariantMap faceMap;
        faceMap["v0"] = f.v0;
        faceMap["v1"] = f.v1;
        faceMap["v2"] = f.v2;
        faceMap["materialId"] = f.materialId;
        result.append(faceMap);
    }

    return result;
}

PhysicsMeshGenerator::CollisionType PhysicsMeshQmlBridge::intToCollisionType(int type) const
{
    switch (type) {
        case 0: return PhysicsMeshGenerator::Chassis;
        case 1: return PhysicsMeshGenerator::Suspension;
        case 2: return PhysicsMeshGenerator::Wheels;
        case 3: return PhysicsMeshGenerator::Interior;
        case 4: return PhysicsMeshGenerator::Extra;
        case 5: return PhysicsMeshGenerator::GroundEffect;
        case 6: return PhysicsMeshGenerator::AeroBody;
        default: return PhysicsMeshGenerator::Chassis;
    }
}

} // namespace ks
