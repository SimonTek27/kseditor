#include <QtTest/QtTest>
#include "core/mesh/PhysicsMeshGenerator.h"
#include "core/mesh/MeshOperations.h"
using namespace ks;

class TestPhysicsMeshGenerator : public QObject {
    Q_OBJECT

private slots:
    void testBoxCollision();
    void testSphereCollision();
    void testCapsuleCollision();
    void testCylinderCollision();
    void testComputeVolume();
    void testComputeCenter();
    void testValidateCollisionMesh();
    void testSingleConvexHull();
};

void TestPhysicsMeshGenerator::testBoxCollision()
{
    MeshData box = PhysicsMeshGenerator::createBoxCollision(QVector3D(1, 2, 3));
    QVERIFY(!box.vertices.isEmpty());
    QVERIFY(!box.faces.isEmpty());
    QVERIFY(box.faces.size() >= 6);

    for (const auto& v : box.vertices) {
        QVERIFY(qAbs(v.position.x()) <= 1.001f);
        QVERIFY(qAbs(v.position.y()) <= 2.001f);
        QVERIFY(qAbs(v.position.z()) <= 3.001f);
    }
}

void TestPhysicsMeshGenerator::testSphereCollision()
{
    MeshData sphere = PhysicsMeshGenerator::createSphereCollision(5.0f);
    QVERIFY(!sphere.vertices.isEmpty());
    QVERIFY(!sphere.faces.isEmpty());

    float maxDist = 0.0f;
    for (const auto& v : sphere.vertices) {
        float d = v.position.length();
        maxDist = qMax(maxDist, d);
    }
    QVERIFY(qAbs(maxDist - 5.0f) < 0.5f);
}

void TestPhysicsMeshGenerator::testCapsuleCollision()
{
    MeshData cap = PhysicsMeshGenerator::createCapsuleCollision(2.0f, 6.0f);
    QVERIFY(!cap.vertices.isEmpty());
    QVERIFY(!cap.faces.isEmpty());

    float minY = 1e9f, maxY = -1e9f;
    float maxRadial = 0.0f;
    for (const auto& v : cap.vertices) {
        minY = qMin(minY, v.position.y());
        maxY = qMax(maxY, v.position.y());
        float r = qSqrt(v.position.x() * v.position.x() + v.position.z() * v.position.z());
        maxRadial = qMax(maxRadial, r);
    }

    QVERIFY(qAbs(minY + 5.0f) < 0.5f);
    QVERIFY(qAbs(maxY - 5.0f) < 0.5f);
}

void TestPhysicsMeshGenerator::testCylinderCollision()
{
    MeshData cyl = PhysicsMeshGenerator::createCylinderCollision(3.0f, 8.0f);
    QVERIFY(!cyl.vertices.isEmpty());
    QVERIFY(!cyl.faces.isEmpty());

    for (const auto& v : cyl.vertices) {
        QVERIFY(v.position.y() >= -4.001f);
        QVERIFY(v.position.y() <= 4.001f);
        float r = qSqrt(v.position.x() * v.position.x() + v.position.z() * v.position.z());
        QVERIFY(r <= 3.001f);
    }
}

void TestPhysicsMeshGenerator::testComputeVolume()
{
    MeshData box = PhysicsMeshGenerator::createBoxCollision(QVector3D(1, 1, 1));
    float vol = PhysicsMeshGenerator::computeMeshVolume(box);
    QVERIFY(qAbs(vol - 8.0f) < 1.0f);
}

void TestPhysicsMeshGenerator::testComputeCenter()
{
    MeshData box = PhysicsMeshGenerator::createBoxCollision(QVector3D(1, 2, 3));
    QVector3D center = PhysicsMeshGenerator::computeMeshCenter(box);
    QVERIFY(center.length() < 0.01f);
}

void TestPhysicsMeshGenerator::testValidateCollisionMesh()
{
    MeshData box = PhysicsMeshGenerator::createBoxCollision(QVector3D(1, 1, 1));
    QString error;
    QVERIFY(PhysicsMeshGenerator::validateCollisionMesh(box, &error));

    MeshData empty;
    QVERIFY(!PhysicsMeshGenerator::validateCollisionMesh(empty));
}

void TestPhysicsMeshGenerator::testSingleConvexHull()
{
    MeshData box = PhysicsMeshGenerator::createBoxCollision(QVector3D(1, 1, 1));
    ConvexHullResult hull = PhysicsMeshGenerator::generateSingleConvexHull(box);
    QVERIFY(hull.success || !hull.errorMessage.isEmpty());
    if (hull.success) {
        QVERIFY(hull.hulls.size() >= 1);
    }
}

QTEST_MAIN(TestPhysicsMeshGenerator)
#include "test_PhysicsMeshGenerator.moc"
