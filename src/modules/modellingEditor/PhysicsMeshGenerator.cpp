#include "PhysicsMeshGenerator.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QtMath>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <numeric>

#if HAS_BULLET
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#endif

#if HAS_VHACD
#include "VHACD.h"
#endif

namespace ks {

// ============================================================================
// PhysicsMeshGenerator Implementation
// ============================================================================

PhysicsMeshGenerator::PhysicsMeshGenerator(QObject* parent)
    : QObject(parent)
{
}

PhysicsMeshGenerator::~PhysicsMeshGenerator()
{
}

// ============================================================================
// Main Generation Methods
// ============================================================================

bool PhysicsMeshGenerator::generateCollisionMesh(
    const QVector<PhysicsVertex>& inputVertices,
    const QVector<PhysicsFace>& inputFaces,
    CollisionType type,
    const QString& meshName)
{
    if (inputVertices.isEmpty() || inputFaces.isEmpty()) {
        emit error("Input mesh is empty");
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    qDebug() << "Generating collision mesh:" << meshName
             << "Type:" << type
             << "Vertices:" << inputVertices.size()
             << "Faces:" << inputFaces.size();

    PhysicsMesh result;

    // Generate based on collision type
    switch (type) {
        case Chassis:
            result = generateSimplifiedChassis(inputVertices, inputFaces);
            break;
        case Suspension:
            result = generateSuspensionArm(inputVertices, inputFaces);
            break;
        case Wheels:
            result = generateWheelCollider(inputVertices, inputFaces);
            break;
        case Interior:
        case Extra:
        case GroundEffect:
        case AeroBody:
            result = generateConvexDecomposition(inputVertices, inputFaces, type);
            break;
    }

    result.name = meshName.isEmpty() ? QString("Physics_%1").arg(type) : meshName;
    result.type = type;

    // Compute properties
    if (m_config.generateNormals) {
        computeFaceNormals(result.vertices, result.faces);
    }

    computeBoundingBox(result.vertices, result.boundingBoxMin, result.boundingBoxMax);
    result.centerOfMass = computeCenterOfMass(result.vertices);

    // Add to appropriate collection
    addMeshToType(result, type);

    qDebug() << "Collision mesh generated in" << timer.elapsed() << "ms:"
             << result.vertices.size() << "vertices,"
             << result.faces.size() << "faces";

    emit meshGenerated();
    return true;
}

PhysicsMeshGenerator::CarCollisionSet PhysicsMeshGenerator::generateFullCarCollision(
    const QVector<PhysicsVertex>& chassisVertices,
    const QVector<PhysicsFace>& chassisFaces,
    const QVector<PhysicsVertex>& suspensionVertices,
    const QVector<PhysicsFace>& suspensionFaces,
    const QVector<PhysicsVertex>& wheelVertices,
    const QVector<PhysicsFace>& wheelFaces)
{
    CarCollisionSet carSet;

    QElapsedTimer timer;
    timer.start();

    qDebug() << "Generating full car collision set...";

    // Generate chassis collision
    if (!chassisVertices.isEmpty() && !chassisFaces.isEmpty()) {
        carSet.chassis = generateSimplifiedChassis(chassisVertices, chassisFaces);
        carSet.chassis.name = "Chassis";
        carSet.chassis.type = Chassis;
        computeBoundingBox(carSet.chassis.vertices, carSet.chassis.boundingBoxMin, carSet.chassis.boundingBoxMax);
        carSet.chassis.centerOfMass = computeCenterOfMass(carSet.chassis.vertices);
    }

    // Generate suspension arms (simplified convex shapes)
    if (!suspensionVertices.isEmpty() && !suspensionFaces.isEmpty()) {
        PhysicsMesh suspension = generateSuspensionArm(suspensionVertices, suspensionFaces);
        suspension.name = "Suspension";
        suspension.type = Suspension;

        // Split into 4 wheel positions (simplified - in reality would need bone data)
        float centerX = 0.0f;
        float frontZ = 0.0f;
        float rearZ = 0.0f;

        if (!suspension.vertices.isEmpty()) {
            float minZ = 1e9f, maxZ = -1e9f;
            float sumX = 0.0f;
            for (const auto& v : suspension.vertices) {
                minZ = qMin(minZ, v.position.z());
                maxZ = qMax(maxZ, v.position.z());
                sumX += v.position.x();
            }
            frontZ = maxZ;
            rearZ = minZ;
            centerX = sumX / suspension.vertices.size();
        }

        // Create 4 suspension arms (simplified)
        auto createSuspensionArm = [&](const QString& name, float offsetX, float offsetZ) {
            PhysicsMesh arm = suspension;
            arm.name = name;
            arm.type = Suspension;
            QMatrix4x4 offsetMat;
            offsetMat.translate(offsetX, 0.0f, offsetZ);
            for (auto& v : arm.vertices) {
                QVector4D pos(v.position, 1.0f);
                pos = offsetMat * pos;
                v.position = pos.toVector3D();
            }
            computeBoundingBox(arm.vertices, arm.boundingBoxMin, arm.boundingBoxMax);
            arm.centerOfMass = computeCenterOfMass(arm.vertices);
            return arm;
        };

        carSet.suspensionFL = createSuspensionArm("Suspension_FL", -0.8f, frontZ * 0.8f);
        carSet.suspensionFR = createSuspensionArm("Suspension_FR", 0.8f, frontZ * 0.8f);
        carSet.suspensionRL = createSuspensionArm("Suspension_RL", -0.8f, rearZ * 0.8f);
        carSet.suspensionRR = createSuspensionArm("Suspension_RR", 0.8f, rearZ * 0.8f);
    }

    // Generate wheel colliders (cylinder approximation)
    if (!wheelVertices.isEmpty() && !wheelFaces.isEmpty()) {
        PhysicsMesh wheel = generateWheelCollider(wheelVertices, wheelFaces);
        wheel.name = "Wheel";
        wheel.type = Wheels;

        // Create 4 wheels at standard positions
        auto createWheel = [&](const QString& name, float offsetX, float offsetZ) {
            PhysicsMesh w = wheel;
            w.name = name;
            w.type = Wheels;
            QMatrix4x4 offsetMat;
            offsetMat.translate(offsetX, 0.0f, offsetZ);
            for (auto& v : w.vertices) {
                QVector4D pos(v.position, 1.0f);
                pos = offsetMat * pos;
                v.position = pos.toVector3D();
            }
            computeBoundingBox(w.vertices, w.boundingBoxMin, w.boundingBoxMax);
            w.centerOfMass = computeCenterOfMass(w.vertices);
            return w;
        };

        carSet.wheelFL = createWheel("Wheel_FL", -0.8f, 1.3f);
        carSet.wheelFR = createWheel("Wheel_FR", 0.8f, 1.3f);
        carSet.wheelRL = createWheel("Wheel_RL", -0.8f, -1.3f);
        carSet.wheelRR = createWheel("Wheel_RR", 0.8f, -1.3f);
    }

    // Generate interior collision (simplified box)
    {
        PhysicsMesh interior;
        interior.name = "Interior";
        interior.type = Interior;

        // Create simple box for cockpit
        float boxW = 0.4f, boxH = 0.3f, boxD = 0.4f;
        QVector3D center(0.0f, 0.8f, 0.0f);

        interior.vertices = {
            {center + QVector3D(-boxW, -boxH, -boxD), QVector3D(0, -1, 0)},
            {center + QVector3D(boxW, -boxH, -boxD), QVector3D(0, -1, 0)},
            {center + QVector3D(boxW, boxH, -boxD), QVector3D(0, 0, -1)},
            {center + QVector3D(-boxW, boxH, -boxD), QVector3D(0, 0, -1)},
            {center + QVector3D(-boxW, -boxH, boxD), QVector3D(0, -1, 0)},
            {center + QVector3D(boxW, -boxH, boxD), QVector3D(0, -1, 0)},
            {center + QVector3D(boxW, boxH, boxD), QVector3D(0, 0, 1)},
            {center + QVector3D(-boxW, boxH, boxD), QVector3D(0, 0, 1)}
        };

        interior.faces = {
            {0, 1, 2}, {0, 2, 3},  // Front
            {4, 6, 5}, {4, 7, 6},  // Back
            {0, 4, 5}, {0, 5, 1},  // Bottom
            {2, 6, 7}, {2, 7, 3},  // Top
            {0, 3, 7}, {0, 7, 4},  // Left
            {1, 5, 6}, {1, 6, 2}   // Right
        };

        computeBoundingBox(interior.vertices, interior.boundingBoxMin, interior.boundingBoxMax);
        interior.centerOfMass = computeCenterOfMass(interior.vertices);
        carSet.interior = interior;
    }

    // Store all meshes
    m_chassisMeshes.clear();
    m_suspensionMeshes.clear();
    m_wheelMeshes.clear();
    m_interiorMeshes.clear();

    if (!carSet.chassis.vertices.isEmpty()) m_chassisMeshes.append(carSet.chassis);
    m_suspensionMeshes << carSet.suspensionFL << carSet.suspensionFR
                       << carSet.suspensionRL << carSet.suspensionRR;
    m_wheelMeshes << carSet.wheelFL << carSet.wheelFR
                  << carSet.wheelRL << carSet.wheelRR;
    m_interiorMeshes.append(carSet.interior);

    qDebug() << "Full car collision set generated in" << timer.elapsed() << "ms";
    emit meshGenerated();

    return carSet;
}

// ============================================================================
// Configuration
// ============================================================================

void PhysicsMeshGenerator::setSimplificationRatio(float ratio)
{
    ratio = qBound(0.01f, ratio, 1.0f);
    if (qAbs(m_config.simplificationRatio - ratio) > 0.001f) {
        m_config.simplificationRatio = ratio;
        emit simplificationRatioChanged(ratio);
    }
}

// ============================================================================
// Mesh Access
// ============================================================================

QVector<PhysicsMeshGenerator::PhysicsMesh> PhysicsMeshGenerator::allMeshes() const
{
    QVector<PhysicsMesh> all;
    all << m_chassisMeshes << m_suspensionMeshes << m_wheelMeshes
        << m_interiorMeshes << m_extraMeshes << m_groundEffectMeshes << m_aeroBodyMeshes;
    return all;
}

QVector<PhysicsMeshGenerator::PhysicsMesh> PhysicsMeshGenerator::meshesByType(CollisionType type) const
{
    switch (type) {
        case Chassis: return m_chassisMeshes;
        case Suspension: return m_suspensionMeshes;
        case Wheels: return m_wheelMeshes;
        case Interior: return m_interiorMeshes;
        case Extra: return m_extraMeshes;
        case GroundEffect: return m_groundEffectMeshes;
        case AeroBody: return m_aeroBodyMeshes;
        default: return {};
    }
}

// ============================================================================
// Export Methods
// ============================================================================

QJsonObject PhysicsMeshGenerator::toJson() const
{
    QJsonObject json;
    json["simplificationRatio"] = m_config.simplificationRatio;
    json["maxConvexParts"] = m_config.maxConvexParts;
    json["mergeDistance"] = m_config.mergeDistance;
    json["useVHACD"] = m_config.useVHACD;

    QJsonArray meshesArray;
    for (const auto& mesh : allMeshes()) {
        QJsonObject meshObj;
        meshObj["name"] = mesh.name;
        meshObj["type"] = static_cast<int>(mesh.type);
        meshObj["vertexCount"] = mesh.vertices.size();
        meshObj["faceCount"] = mesh.faces.size();

        QJsonArray vertsArray;
        for (const auto& v : mesh.vertices) {
            QJsonObject vertObj;
            vertObj["x"] = v.position.x();
            vertObj["y"] = v.position.y();
            vertObj["z"] = v.position.z();
            vertsArray.append(vertObj);
        }
        meshObj["vertices"] = vertsArray;

        QJsonArray facesArray;
        for (const auto& f : mesh.faces) {
            QJsonObject faceObj;
            faceObj["v0"] = f.v0;
            faceObj["v1"] = f.v1;
            faceObj["v2"] = f.v2;
            facesArray.append(faceObj);
        }
        meshObj["faces"] = facesArray;

        meshesArray.append(meshObj);
    }
    json["meshes"] = meshesArray;

    return json;
}

bool PhysicsMeshGenerator::fromJson(const QJsonObject& json)
{
    clearAllMeshes();

    m_config.simplificationRatio = json["simplificationRatio"].toDouble(0.1f);
    m_config.maxConvexParts = json["maxConvexParts"].toInt(32);
    m_config.mergeDistance = json["mergeDistance"].toDouble(0.01f);
    m_config.useVHACD = json["useVHACD"].toBool(true);

    QJsonArray meshesArray = json["meshes"].toArray();
    for (const auto& meshVal : meshesArray) {
        QJsonObject meshObj = meshVal.toObject();
        PhysicsMesh mesh;
        mesh.name = meshObj["name"].toString();
        mesh.type = static_cast<CollisionType>(meshObj["type"].toInt());

        QJsonArray vertsArray = meshObj["vertices"].toArray();
        for (const auto& vertVal : vertsArray) {
            QJsonObject vertObj = vertVal.toObject();
            PhysicsVertex v;
            v.position = QVector3D(vertObj["x"].toDouble(),
                                   vertObj["y"].toDouble(),
                                   vertObj["z"].toDouble());
            mesh.vertices.append(v);
        }

        QJsonArray facesArray = meshObj["faces"].toArray();
        for (const auto& faceVal : facesArray) {
            QJsonObject faceObj = faceVal.toObject();
            PhysicsFace f;
            f.v0 = faceObj["v0"].toInt();
            f.v1 = faceObj["v1"].toInt();
            f.v2 = faceObj["v2"].toInt();
            mesh.faces.append(f);
        }

        computeBoundingBox(mesh.vertices, mesh.boundingBoxMin, mesh.boundingBoxMax);
        mesh.centerOfMass = computeCenterOfMass(mesh.vertices);

        addMeshToType(mesh, mesh.type);
    }

    return true;
}

bool PhysicsMeshGenerator::exportAcPhysicsMesh(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "# Assetto Corsa Physics Mesh Export\n";
    stream << "# Generated by ksEditor PhysicsMeshGenerator\n";
    stream << "# Meshes: " << allMeshes().size() << "\n\n";

    for (const auto& mesh : allMeshes()) {
        stream << "[MESH:" << mesh.name << "]\n";
        stream << "TYPE=" << static_cast<int>(mesh.type) << "\n";
        stream << "VERTICES=" << mesh.vertices.size() << "\n";
        stream << "FACES=" << mesh.faces.size() << "\n";
        stream << "CENTER_OF_MASS=" << mesh.centerOfMass.x() << ","
               << mesh.centerOfMass.y() << "," << mesh.centerOfMass.z() << "\n";
        stream << "BOUNDING_BOX_MIN=" << mesh.boundingBoxMin.x() << ","
               << mesh.boundingBoxMin.y() << "," << mesh.boundingBoxMin.z() << "\n";
        stream << "BOUNDING_BOX_MAX=" << mesh.boundingBoxMax.x() << ","
               << mesh.boundingBoxMax.y() << "," << mesh.boundingBoxMax.z() << "\n\n";

        stream << "VERTICES:\n";
        for (const auto& v : mesh.vertices) {
            stream << v.position.x() << " " << v.position.y() << " " << v.position.z() << "\n";
        }

        stream << "\nFACES:\n";
        for (const auto& f : mesh.faces) {
            stream << f.v0 << " " << f.v1 << " " << f.v2 << "\n";
        }

        stream << "\n\n";
    }

    file.close();
    return true;
}

bool PhysicsMeshGenerator::exportObj(const QString& filePath, CollisionType type) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    stream << "# Physics Mesh Export - " << filePath << "\n";
    stream << "# Type: " << static_cast<int>(type) << "\n\n";

    auto meshes = meshesByType(type);
    int vertexOffset = 0;

    for (const auto& mesh : meshes) {
        stream << "o " << mesh.name << "\n";

        for (const auto& v : mesh.vertices) {
            stream << "v " << v.position.x() << " " << v.position.y() << " " << v.position.z() << "\n";
        }

        for (const auto& f : mesh.faces) {
            stream << "f " << (f.v0 + 1 + vertexOffset) << " "
                   << (f.v1 + 1 + vertexOffset) << " "
                   << (f.v2 + 1 + vertexOffset) << "\n";
        }

        vertexOffset += mesh.vertices.size();
        stream << "\n";
    }

    file.close();
    return true;
}

// ============================================================================
// Static Utility Methods
// ============================================================================

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::computeConvexHull(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces)
{
    PhysicsMesh result;

#if HAS_BULLET
    // Use Bullet Physics for convex hull computation
    btConvexHullShape* hull = new btConvexHullShape();

    for (const auto& v : vertices) {
        hull->addPoint(btVector3(v.position.x(), v.position.y(), v.position.z()));
    }

    // Convert to mesh
    btShapeHull shapeHull(hull);
    shapeHull.buildHull(hull->getMargin());

    const unsigned int* indices = shapeHull.getIndexPointer();
    const btVector3* points = shapeHull.getVertexPointer();

    result.vertices.resize(shapeHull.numVertices());
    for (int i = 0; i < shapeHull.numVertices(); ++i) {
        result.vertices[i].position = QVector3D(points[i].x(), points[i].y(), points[i].z());
    }

    result.faces.resize(shapeHull.numTriangles());
    for (int i = 0; i < shapeHull.numTriangles(); ++i) {
        result.faces[i].v0 = indices[i * 3 + 0];
        result.faces[i].v1 = indices[i * 3 + 1];
        result.faces[i].v2 = indices[i * 3 + 2];
    }

    delete hull;
#else
    // Fallback: Incremental convex hull algorithm (Quickhull-style)
    // Builds a proper convex hull when Bullet Physics is unavailable
    if (vertices.size() < 4) {
        result.vertices = vertices;
        if (vertices.size() == 3)
            result.faces = {{0, 1, 2}};
        return result;
    }

    struct HullFace {
        int v0, v1, v2;
        QVector3D normal;
        bool visible;

        HullFace(int a, int b, int c)
            : v0(a), v1(b), v2(c), normal(0, 0, 0), visible(true) {}

        void computeNormal(const QVector<PhysicsVertex>& verts) {
            const QVector3D& a = verts[v0].position;
            const QVector3D& b = verts[v1].position;
            const QVector3D& c = verts[v2].position;
            normal = QVector3D::crossProduct(b - a, c - a);
        }
    };

    auto faceDistance = [&vertices](const HullFace& f, const QVector3D& pt) -> float {
        const QVector3D& a = vertices[f.v0].position;
        return QVector3D::dotProduct(f.normal, pt - a);
    };

    auto ensureOutward = [&vertices](HullFace& f, int oppositeIdx) {
        f.computeNormal(vertices);
        const QVector3D& a = vertices[f.v0].position;
        const QVector3D& pt = vertices[oppositeIdx].position;
        if (QVector3D::dotProduct(f.normal, pt - a) > 0) {
            std::swap(f.v1, f.v2);
            f.computeNormal(vertices);
        }
    };

    // Step 1: Build initial tetrahedron from extreme points
    int p0 = 0, p1 = 0;
    float minX = vertices[0].position.x(), maxX = vertices[0].position.x();
    for (int i = 1; i < vertices.size(); ++i) {
        if (vertices[i].position.x() < minX) { minX = vertices[i].position.x(); p0 = i; }
        if (vertices[i].position.x() > maxX) { maxX = vertices[i].position.x(); p1 = i; }
    }

    if (p0 == p1) { result.vertices = vertices; return result; }

    int p2 = -1;
    float maxDist = 0;
    QVector3D lineDir = (vertices[p1].position - vertices[p0].position).normalized();
    for (int i = 0; i < vertices.size(); ++i) {
        if (i == p0 || i == p1) continue;
        QVector3D v = vertices[i].position - vertices[p0].position;
        float d = (v - lineDir * QVector3D::dotProduct(v, lineDir)).length();
        if (d > maxDist) { maxDist = d; p2 = i; }
    }

    if (p2 == -1 || maxDist < 1e-8f) {
        result.vertices = vertices;
        return result;
    }

    int p3 = -1;
    maxDist = 0;
    QVector3D planeN = QVector3D::crossProduct(
        vertices[p1].position - vertices[p0].position,
        vertices[p2].position - vertices[p0].position);
    float planeLen = planeN.length();
    if (planeLen < 1e-8f) {
        result.vertices = vertices;
        result.faces = {{p0, p1, p2}};
        return result;
    }
    planeN /= planeLen;

    for (int i = 0; i < vertices.size(); ++i) {
        if (i == p0 || i == p1 || i == p2) continue;
        float d = qAbs(QVector3D::dotProduct(planeN, vertices[i].position - vertices[p0].position));
        if (d > maxDist) { maxDist = d; p3 = i; }
    }

    if (p3 == -1 || maxDist < 1e-8f) {
        result.vertices = vertices;
        result.faces = {{p0, p1, p2}};
        return result;
    }

    QVector<HullFace> faceList;
    faceList.reserve(vertices.size() * 2);
    faceList.append(HullFace(p0, p1, p2)); ensureOutward(faceList.last(), p3);
    faceList.append(HullFace(p1, p2, p3)); ensureOutward(faceList.last(), p0);
    faceList.append(HullFace(p2, p0, p3)); ensureOutward(faceList.last(), p1);
    faceList.append(HullFace(p0, p1, p3)); ensureOutward(faceList.last(), p2);

    // Step 2: Process remaining points
    for (int pi = 0; pi < vertices.size(); ++pi) {
        if (pi == p0 || pi == p1 || pi == p2 || pi == p3) continue;
        const QVector3D& point = vertices[pi].position;

        // Find all faces visible from this point
        bool anyVisible = false;
        for (auto& f : faceList) {
            f.visible = faceDistance(f, point) > 1e-8f;
            if (f.visible) anyVisible = true;
        }

        if (!anyVisible) continue;

        // Collect horizon edges: edges shared by one visible and one non-visible face
        struct Edge { int va, vb; };
        QVector<Edge> horizon;
        horizon.reserve(64);

        for (int fi = 0; fi < faceList.size(); ++fi) {
            if (!faceList[fi].visible) continue;

            const int edgeVerts[3][2] = {
                {faceList[fi].v0, faceList[fi].v1},
                {faceList[fi].v1, faceList[fi].v2},
                {faceList[fi].v2, faceList[fi].v0}
            };

            for (int e = 0; e < 3; ++e) {
                int a = edgeVerts[e][0], b = edgeVerts[e][1];

                bool otherVisible = false;
                for (int fj = 0; fj < faceList.size(); ++fj) {
                    if (fj == fi || !faceList[fj].visible) continue;
                    if ((faceList[fj].v0 == a && faceList[fj].v1 == b) ||
                        (faceList[fj].v1 == a && faceList[fj].v2 == b) ||
                        (faceList[fj].v2 == a && faceList[fj].v0 == b) ||
                        (faceList[fj].v0 == b && faceList[fj].v1 == a) ||
                        (faceList[fj].v1 == b && faceList[fj].v2 == a) ||
                        (faceList[fj].v2 == b && faceList[fj].v0 == a)) {
                        otherVisible = true;
                        break;
                    }
                }

                if (!otherVisible) {
                    horizon.append({a, b});
                }
            }
        }

        // Remove visible faces
        faceList.erase(std::remove_if(faceList.begin(), faceList.end(),
            [](const HullFace& f) { return f.visible; }), faceList.end());

        // Add new faces from point to horizon edges
        int oldSize = faceList.size();
        for (const auto& he : horizon) {
            faceList.append(HullFace(pi, he.va, he.vb));
        }

        // Compute normals for new faces and verify outward orientation
        QVector3D centroid(0, 0, 0);
        int count = 0;
        for (int i = 0; i < oldSize; ++i) {
            centroid += vertices[faceList[i].v0].position;
            centroid += vertices[faceList[i].v1].position;
            centroid += vertices[faceList[i].v2].position;
            count += 3;
        }
        if (count > 0) centroid /= count;

        for (int i = oldSize; i < faceList.size(); ++i) {
            faceList[i].computeNormal(vertices);
            if (faceDistance(faceList[i], centroid) > 0) {
                std::swap(faceList[i].v1, faceList[i].v2);
                faceList[i].computeNormal(vertices);
            }
        }
    }

    // Extract result
    result.vertices = vertices;
    result.faces.reserve(faceList.size());
    for (const auto& f : faceList) {
        result.faces.append({f.v0, f.v1, f.v2});
    }
#endif

    return result;
}

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::simplifyMesh(
    const PhysicsMesh& input,
    float ratio)
{
    PhysicsMesh result = input;

    if (ratio >= 1.0f || input.faces.size() <= 4) {
        return result;
    }

    int targetFaces = qMax(4, static_cast<int>(input.faces.size() * ratio));

    // Simple edge collapse decimation
    QVector<bool> removed(input.faces.size(), false);
    int removedCount = 0;

    while (removedCount < input.faces.size() - targetFaces) {
        float minEdgeLen = 1e9f;
        int bestFace = -1;
        int bestEdge = -1;

        for (int i = 0; i < input.faces.size(); ++i) {
            if (removed[i]) continue;

            const auto& f = input.faces[i];
            int verts[3] = {f.v0, f.v1, f.v2};

            for (int e = 0; e < 3; ++e) {
                int v1 = verts[e];
                int v2 = verts[(e + 1) % 3];

                if (v1 < result.vertices.size() && v2 < result.vertices.size()) {
                    float edgeLen = (result.vertices[v1].position - result.vertices[v2].position).length();
                    if (edgeLen < minEdgeLen) {
                        minEdgeLen = edgeLen;
                        bestFace = i;
                        bestEdge = e;
                    }
                }
            }
        }

        if (bestFace < 0) break;

        // Collapse edge
        const auto& f = input.faces[bestFace];
        int v1 = f.v0;
        int v2 = f.v1;

        // Merge v2 into v1
        if (v1 < result.vertices.size() && v2 < result.vertices.size()) {
            result.vertices[v1].position = (result.vertices[v1].position + result.vertices[v2].position) * 0.5f;
        }

        // Update face references
        for (auto& face : result.faces) {
            if (face.v0 == v2) face.v0 = v1;
            if (face.v1 == v2) face.v1 = v1;
            if (face.v2 == v2) face.v2 = v1;
        }

        removed[bestFace] = true;
        removedCount++;
    }

    // Remove degenerate faces
    QVector<PhysicsFace> validFaces;
    for (int i = 0; i < result.faces.size(); ++i) {
        if (!removed[i]) {
            const auto& f = result.faces[i];
            if (f.v0 != f.v1 && f.v1 != f.v2 && f.v0 != f.v2) {
                validFaces.append(f);
            }
        }
    }
    result.faces = validFaces;

    return result;
}

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::mergeMeshes(
    const QVector<PhysicsMesh>& meshes)
{
    PhysicsMesh result;
    int vertexOffset = 0;

    for (const auto& mesh : meshes) {
        for (const auto& v : mesh.vertices) {
            result.vertices.append(v);
        }

        for (const auto& f : mesh.faces) {
            PhysicsFace newFace = f;
            newFace.v0 += vertexOffset;
            newFace.v1 += vertexOffset;
            newFace.v2 += vertexOffset;
            result.faces.append(newFace);
        }

        vertexOffset += mesh.vertices.size();
    }

    return result;
}

QVector3D PhysicsMeshGenerator::computeCenterOfMass(const QVector<PhysicsVertex>& vertices)
{
    if (vertices.isEmpty()) return QVector3D(0, 0, 0);

    QVector3D sum(0, 0, 0);
    for (const auto& v : vertices) {
        sum += v.position;
    }
    return sum / vertices.size();
}

void PhysicsMeshGenerator::computeBoundingBox(
    const QVector<PhysicsVertex>& vertices,
    QVector3D& min,
    QVector3D& max)
{
    if (vertices.isEmpty()) {
        min = max = QVector3D(0, 0, 0);
        return;
    }

    min = max = vertices[0].position;
    for (const auto& v : vertices) {
        min.setX(qMin(min.x(), v.position.x()));
        min.setY(qMin(min.y(), v.position.y()));
        min.setZ(qMin(min.z(), v.position.z()));
        max.setX(qMax(max.x(), v.position.x()));
        max.setY(qMax(max.y(), v.position.y()));
        max.setZ(qMax(max.z(), v.position.z()));
    }
}

// ============================================================================
// Validation
// ============================================================================

PhysicsMeshGenerator::ValidationResult PhysicsMeshGenerator::validateCollisionMesh(
    const PhysicsMesh& mesh) const
{
    ValidationResult result;

    // Check for empty mesh
    if (mesh.vertices.isEmpty()) {
        result.valid = false;
        result.errors.append("Mesh has no vertices");
        return result;
    }

    if (mesh.faces.isEmpty()) {
        result.valid = false;
        result.errors.append("Mesh has no faces");
        return result;
    }

    // Check face indices
    for (int i = 0; i < mesh.faces.size(); ++i) {
        const auto& f = mesh.faces[i];
        if (f.v0 >= mesh.vertices.size() || f.v1 >= mesh.vertices.size() || f.v2 >= mesh.vertices.size()) {
            result.valid = false;
            result.errors.append(QString("Face %1 has invalid vertex indices").arg(i));
        }
    }

    // Compute stats
    result.totalVertices = mesh.vertices.size();
    result.totalTriangles = mesh.faces.size();

    // Compute surface area
    result.totalSurfaceArea = 0.0f;
    for (const auto& f : mesh.faces) {
        if (f.v0 < mesh.vertices.size() && f.v1 < mesh.vertices.size() && f.v2 < mesh.vertices.size()) {
            QVector3D v0 = mesh.vertices[f.v0].position;
            QVector3D v1 = mesh.vertices[f.v1].position;
            QVector3D v2 = mesh.vertices[f.v2].position;

            QVector3D e1 = v1 - v0;
            QVector3D e2 = v2 - v0;
            float area = QVector3D::crossProduct(e1, e2).length() * 0.5f;
            result.totalSurfaceArea += area;
        }
    }

    // Compute volume (signed)
    result.totalVolume = 0.0f;
    for (const auto& f : mesh.faces) {
        if (f.v0 < mesh.vertices.size() && f.v1 < mesh.vertices.size() && f.v2 < mesh.vertices.size()) {
            QVector3D v0 = mesh.vertices[f.v0].position;
            QVector3D v1 = mesh.vertices[f.v1].position;
            QVector3D v2 = mesh.vertices[f.v2].position;

            result.totalVolume += QVector3D::dotProduct(v0, QVector3D::crossProduct(v1, v2));
        }
    }
    result.totalVolume = qAbs(result.totalVolume) / 6.0f;

    return result;
}

PhysicsMeshGenerator::ValidationResult PhysicsMeshGenerator::validateFullCarSet(
    const CarCollisionSet& carSet) const
{
    ValidationResult result;

    // Validate each component
    auto validateAndMerge = [&](const PhysicsMesh& mesh, const QString& name) {
        auto meshResult = validateCollisionMesh(mesh);
        if (!meshResult.valid) {
            for (const auto& err : meshResult.errors) {
                result.errors.append(name + ": " + err);
            }
        }
        for (const auto& warn : meshResult.warnings) {
            result.warnings.append(name + ": " + warn);
        }
        result.totalVertices += meshResult.totalVertices;
        result.totalTriangles += meshResult.totalTriangles;
        result.totalVolume += meshResult.totalVolume;
        result.totalSurfaceArea += meshResult.totalSurfaceArea;
    };

    validateAndMerge(carSet.chassis, "Chassis");
    validateAndMerge(carSet.suspensionFL, "Suspension_FL");
    validateAndMerge(carSet.suspensionFR, "Suspension_FR");
    validateAndMerge(carSet.suspensionRL, "Suspension_RL");
    validateAndMerge(carSet.suspensionRR, "Suspension_RR");
    validateAndMerge(carSet.wheelFL, "Wheel_FL");
    validateAndMerge(carSet.wheelFR, "Wheel_FR");
    validateAndMerge(carSet.wheelRL, "Wheel_RL");
    validateAndMerge(carSet.wheelRR, "Wheel_RR");
    validateAndMerge(carSet.interior, "Interior");

    for (int i = 0; i < carSet.extras.size(); ++i) {
        validateAndMerge(carSet.extras[i], QString("Extra_%1").arg(i));
    }

    result.valid = result.errors.isEmpty();
    return result;
}

// ============================================================================
// Internal Helper Methods
// ============================================================================

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::generateConvexDecomposition(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces,
    CollisionType type)
{
    PhysicsMesh result;

#if HAS_VHACD
    if (m_config.useVHACD) {
        auto parts = generateVHACDDecomposition(vertices, faces, m_config.maxConvexParts);
        if (!parts.isEmpty()) {
            return mergeMeshes(parts);
        }
    }
#endif

#if HAS_BULLET
    // Fallback to single convex hull
    return computeConvexHull(vertices, faces);
#else
    // Fallback to simplified mesh
    result.vertices = vertices;
    result.faces = faces;
    return simplifyMesh(result, m_config.simplificationRatio);
#endif
}

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::generateSimplifiedChassis(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces)
{
    // Chassis needs to be accurate but simplified
    PhysicsMesh result;

#if HAS_VHACD
    if (m_config.useVHACD) {
        // Use convex decomposition for accurate chassis collision
        auto parts = generateVHACDDecomposition(vertices, faces, qMin(8, m_config.maxConvexParts));
        if (!parts.isEmpty()) {
            return mergeMeshes(parts);
        }
    }
#endif

#if HAS_BULLET
    // Single convex hull for simple chassis
    return computeConvexHull(vertices, faces);
#else
    // Simplified mesh
    result.vertices = vertices;
    result.faces = faces;
    return simplifyMesh(result, m_config.simplificationRatio);
#endif
}

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::generateSuspensionArm(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces)
{
    // Suspension arms are simple convex shapes
    return computeConvexHull(vertices, faces);
}

PhysicsMeshGenerator::PhysicsMesh PhysicsMeshGenerator::generateWheelCollider(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces)
{
    // Wheels are approximated as cylinders
    PhysicsMesh result;

    // Compute bounding box
    QVector3D min, max;
    computeBoundingBox(vertices, min, max);

    QVector3D center = (min + max) * 0.5f;
    QVector3D size = max - min;

    float radius = qMax(size.x(), size.z()) * 0.5f;
    float width = size.y();

    // Create cylinder
    int segments = 16;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        // Bottom ring
        PhysicsVertex v1;
        v1.position = center + QVector3D(x, -width * 0.5f, z);
        v1.normal = QVector3D(cosf(angle), 0, sinf(angle));
        result.vertices.append(v1);

        // Top ring
        PhysicsVertex v2;
        v2.position = center + QVector3D(x, width * 0.5f, z);
        v2.normal = QVector3D(cosf(angle), 0, sinf(angle));
        result.vertices.append(v2);
    }

    // Create faces
    for (int i = 0; i < segments; ++i) {
        int base = i * 2;

        // Side faces
        result.faces.append({base, base + 1, base + 2});
        result.faces.append({base + 1, base + 3, base + 2});
    }

    // Cap faces
    int centerBottom = result.vertices.size();
    PhysicsVertex cb;
    cb.position = center + QVector3D(0, -width * 0.5f, 0);
    cb.normal = QVector3D(0, -1, 0);
    result.vertices.append(cb);

    int centerTop = result.vertices.size();
    PhysicsVertex ct;
    ct.position = center + QVector3D(0, width * 0.5f, 0);
    ct.normal = QVector3D(0, 1, 0);
    result.vertices.append(ct);

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;

        // Bottom cap
        result.faces.append({centerBottom, i * 2, next * 2});

        // Top cap
        result.faces.append({centerTop, next * 2 + 1, i * 2 + 1});
    }

    return result;
}

void PhysicsMeshGenerator::mergeCloseVertices(QVector<PhysicsVertex>& vertices, float threshold)
{
    if (vertices.isEmpty()) return;

    QVector<int> mergeMap(vertices.size(), -1);

    for (int i = 0; i < vertices.size(); ++i) {
        if (mergeMap[i] != -1) continue;

        for (int j = i + 1; j < vertices.size(); ++j) {
            if (mergeMap[j] != -1) continue;

            float dist = (vertices[i].position - vertices[j].position).length();
            if (dist < threshold) {
                mergeMap[j] = i;
            }
        }
    }

    QVector<PhysicsVertex> merged;
    QVector<int> newIndex(vertices.size(), -1);

    for (int i = 0; i < vertices.size(); ++i) {
        if (mergeMap[i] == -1) {
            newIndex[i] = merged.size();
            merged.append(vertices[i]);
        }
    }

    for (int i = 0; i < vertices.size(); ++i) {
        if (mergeMap[i] != -1) {
            int target = mergeMap[i];
            while (mergeMap[target] != -1) target = mergeMap[target];
            newIndex[i] = newIndex[target];
        }
    }

    vertices = merged;
}

void PhysicsMeshGenerator::removeDegenerateFaces(QVector<PhysicsFace>& faces, int vertexCount)
{
    QVector<PhysicsFace> valid;
    for (const auto& f : faces) {
        if (f.v0 >= 0 && f.v0 < vertexCount &&
            f.v1 >= 0 && f.v1 < vertexCount &&
            f.v2 >= 0 && f.v2 < vertexCount &&
            f.v0 != f.v1 && f.v1 != f.v2 && f.v0 != f.v2) {
            valid.append(f);
        }
    }
    faces = valid;
}

void PhysicsMeshGenerator::computeFaceNormals(QVector<PhysicsVertex>& vertices, const QVector<PhysicsFace>& faces)
{
    // Reset normals
    for (auto& v : vertices) {
        v.normal = QVector3D(0, 0, 0);
    }

    // Accumulate face normals
    for (const auto& f : faces) {
        if (f.v0 >= vertices.size() || f.v1 >= vertices.size() || f.v2 >= vertices.size()) {
            continue;
        }

        QVector3D v0 = vertices[f.v0].position;
        QVector3D v1 = vertices[f.v1].position;
        QVector3D v2 = vertices[f.v2].position;

        QVector3D faceNormal = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();

        vertices[f.v0].normal += faceNormal;
        vertices[f.v1].normal += faceNormal;
        vertices[f.v2].normal += faceNormal;
    }

    // Normalize
    for (auto& v : vertices) {
        if (v.normal.lengthSquared() > 0.0001f) {
            v.normal.normalize();
        } else {
            v.normal = QVector3D(0, 1, 0);
        }
    }
}

void PhysicsMeshGenerator::addMeshToType(const PhysicsMesh& mesh, CollisionType type)
{
    switch (type) {
        case Chassis: m_chassisMeshes.append(mesh); break;
        case Suspension: m_suspensionMeshes.append(mesh); break;
        case Wheels: m_wheelMeshes.append(mesh); break;
        case Interior: m_interiorMeshes.append(mesh); break;
        case Extra: m_extraMeshes.append(mesh); break;
        case GroundEffect: m_groundEffectMeshes.append(mesh); break;
        case AeroBody: m_aeroBodyMeshes.append(mesh); break;
    }
}

void PhysicsMeshGenerator::clearAllMeshes()
{
    m_chassisMeshes.clear();
    m_suspensionMeshes.clear();
    m_wheelMeshes.clear();
    m_interiorMeshes.clear();
    m_extraMeshes.clear();
    m_groundEffectMeshes.clear();
    m_aeroBodyMeshes.clear();
}

// ============================================================================
// Bullet Physics Helpers
// ============================================================================

#if HAS_BULLET

void* PhysicsMeshGenerator::createBulletConvexHull(const QVector<PhysicsVertex>& vertices)
{
    btConvexHullShape* hull = new btConvexHullShape();

    for (const auto& v : vertices) {
        hull->addPoint(btVector3(v.position.x(), v.position.y(), v.position.z()));
    }

    return hull;
}

void* PhysicsMeshGenerator::createBulletConvexDecomp(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces,
    int maxParts)
{
    // For now, use single convex hull
    // Full convex decomposition would require VHACD
    return createBulletConvexHull(vertices);
}

QVector<PhysicsMeshGenerator::PhysicsMesh> PhysicsMeshGenerator::extractBulletConvexParts(void* btShape)
{
    QVector<PhysicsMesh> parts;

    if (!btShape) return parts;

    btConvexHullShape* hull = static_cast<btConvexHullShape*>(btShape);

    btShapeHull shapeHull(hull);
    shapeHull.buildHull(hull->getMargin());

    PhysicsMesh mesh;
    const unsigned int* indices = shapeHull.getIndexPointer();
    const btVector3* points = shapeHull.getVertexPointer();

    mesh.vertices.resize(shapeHull.numVertices());
    for (int i = 0; i < shapeHull.numVertices(); ++i) {
        mesh.vertices[i].position = QVector3D(points[i].x(), points[i].y(), points[i].z());
    }

    mesh.faces.resize(shapeHull.numTriangles());
    for (int i = 0; i < shapeHull.numTriangles(); ++i) {
        mesh.faces[i].v0 = indices[i * 3 + 0];
        mesh.faces[i].v1 = indices[i * 3 + 1];
        mesh.faces[i].v2 = indices[i * 3 + 2];
    }

    parts.append(mesh);
    return parts;
}

void PhysicsMeshGenerator::releaseBulletShape(void* btShape)
{
    delete static_cast<btConvexHullShape*>(btShape);
}

#endif

// ============================================================================
// VHACD Helpers
// ============================================================================

#if HAS_VHACD

QVector<PhysicsMeshGenerator::PhysicsMesh> PhysicsMeshGenerator::generateVHACDDecomposition(
    const QVector<PhysicsVertex>& vertices,
    const QVector<PhysicsFace>& faces,
    int maxParts)
{
    QVector<PhysicsMesh> parts;

    VHACD::IVHACD* vhacd = VHACD::CreateVHACD();

    // Convert to VHACD format
    std::vector<float>vhacdVertices;
    std::vector<unsigned int> vhacdTriangles;

    for (const auto& v : vertices) {
        vhacdVertices.push_back(v.position.x());
        vhacdVertices.push_back(v.position.y());
        vhacdVertices.push_back(v.position.z());
    }

    for (const auto& f : faces) {
        vhacdTriangles.push_back(f.v0);
        vhacdTriangles.push_back(f.v1);
        vhacdTriangles.push_back(f.v2);
    }

    VHACD::IVHACD::Parameters params;
    params.m_concavity = 0.001;
    params.m_resolution = 100000;
    params.m_maxNumVerticesPerCH = 64;
    params.m_depth = 20;

    if (vhacd->Compute(vhacdVertices.data(), 3,
                       static_cast<unsigned int>(vhacdVertices.size() / 3),
                       reinterpret_cast<const int*>(vhacdTriangles.data()), 3,
                       static_cast<unsigned int>(vhacdTriangles.size() / 3),
                       params)) {

        int nParts = vhacd->GetNConvexHulls();

        for (int i = 0; i < nParts; ++i) {
            VHACD::IVHACD::ConvexHull hull;
            vhacd->GetConvexHull(i, hull);

            PhysicsMesh part;

            // Extract vertices
            part.vertices.resize(hull.m_nPoints);
            for (int j = 0; j < hull.m_nPoints; ++j) {
                part.vertices[j].position = QVector3D(
                    hull.m_points[j * 3],
                    hull.m_points[j * 3 + 1],
                    hull.m_points[j * 3 + 2]
                );
            }

            // Extract faces
            part.faces.resize(hull.m_nTriangles);
            for (int j = 0; j < hull.m_nTriangles; ++j) {
                part.faces[j].v0 = hull.m_triangles[j * 3];
                part.faces[j].v1 = hull.m_triangles[j * 3 + 1];
                part.faces[j].v2 = hull.m_triangles[j * 3 + 2];
            }

            parts.append(part);
        }
    }

    vhacd->Release();
    return parts;
}

#endif

} // namespace ks
