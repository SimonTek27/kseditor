#include "core/mesh/MeshOperations.h"
#include <QtTest/QtTest>

using namespace ks;

class TestMeshOperations : public QObject {
    Q_OBJECT

private slots:
    void testCreateBox();
    void testCreateSphere();
    void testCreateCylinder();
    void testCreateCone();
    void testCreatePlane();
    void testCreateTorus();
    void testCreateGrid();
    void testCreateIcosphere();
    void testTriangulate();
    void testMirror();
    void testArray();
    void testRadialArray();
    void testMergeMeshes();
    void testWeldVertices();
    void testSubdivide();
    void testDisplace();
    void testFlipFaces();
    void testComputeNormals();
    void testBoundingBox();
};

void TestMeshOperations::testCreateBox()
{
    MeshData box = MeshOperations::createBox(2.0f, 1.0f, 3.0f);
    QVERIFY(box.getVertexCount() > 0);
    QVERIFY(box.getTriangleCount() > 0);
    QCOMPARE(box.getTriangleCount(), 12);
}

void TestMeshOperations::testCreateSphere()
{
    MeshData sphere = MeshOperations::createSphere(0.5f, 16, 8);
    QVERIFY(sphere.getVertexCount() > 0);
    QVERIFY(sphere.getTriangleCount() > 0);
}

void TestMeshOperations::testCreateCylinder()
{
    MeshData cyl = MeshOperations::createCylinder(0.5f, 1.0f, 16);
    QVERIFY(cyl.getVertexCount() > 0);
    QVERIFY(cyl.getTriangleCount() > 0);
}

void TestMeshOperations::testCreateCone()
{
    MeshData cone = MeshOperations::createCone(0.5f, 1.0f, 16);
    QVERIFY(cone.getVertexCount() > 0);
    QVERIFY(cone.getTriangleCount() > 0);
}

void TestMeshOperations::testCreatePlane()
{
    MeshData plane = MeshOperations::createPlane(2.0f, 2.0f, 2, 2);
    QVERIFY(plane.getVertexCount() >= 9);
    QVERIFY(plane.getTriangleCount() >= 8);
}

void TestMeshOperations::testCreateTorus()
{
    MeshData torus = MeshOperations::createTorus(0.5f, 0.2f, 16, 8);
    QVERIFY(torus.getVertexCount() > 0);
    QVERIFY(torus.getTriangleCount() > 0);
}

void TestMeshOperations::testCreateGrid()
{
    MeshData grid = MeshOperations::createGrid(2.0f, 2.0f, 10, 10);
    QVERIFY(grid.getVertexCount() >= 121);
    QVERIFY(grid.getTriangleCount() >= 200);
}

void TestMeshOperations::testCreateIcosphere()
{
    MeshData ico = MeshOperations::createIcosphere(0.5f, 2);
    QVERIFY(ico.getVertexCount() > 0);
    QVERIFY(ico.getTriangleCount() > 0);
}

void TestMeshOperations::testTriangulate()
{
    MeshData mesh;
    mesh.vertices.resize(4);
    mesh.vertices[0].position = QVector3D(0,0,0);
    mesh.vertices[1].position = QVector3D(1,0,0);
    mesh.vertices[2].position = QVector3D(1,1,0);
    mesh.vertices[3].position = QVector3D(0,1,0);
    mesh.faces.append(Face{0,1,2,3});

    QCOMPARE(mesh.getTriangleCount(), 1);
    mesh.triangulate();
    QCOMPARE(mesh.getTriangleCount(), 2);
}

void TestMeshOperations::testMirror()
{
    MeshData box = MeshOperations::createBox(1,1,1);
    MeshData mirrored = MeshOperations::mirror(box, QVector3D(1,0,0));
    QVERIFY(mirrored.getVertexCount() > box.getVertexCount());
}

void TestMeshOperations::testArray()
{
    MeshData box = MeshOperations::createBox(0.5f, 0.5f, 0.5f);
    MeshData arr = MeshOperations::array(box, 5, QVector3D(1,0,0));
    QVERIFY(arr.getVertexCount() > box.getVertexCount());
}

void TestMeshOperations::testRadialArray()
{
    MeshData box = MeshOperations::createBox(0.5f, 0.5f, 0.5f);
    MeshData arr = MeshOperations::radialArray(box, 8, QVector3D(0,1,0), 360.0f);
    QVERIFY(arr.getVertexCount() > box.getVertexCount());
}

void TestMeshOperations::testMergeMeshes()
{
    MeshData a = MeshOperations::createBox(1,1,1);
    MeshData b = MeshOperations::createSphere(0.5f);
    int before = a.getVertexCount();
    MeshOperations::mergeMeshes(a, b);
    QVERIFY(a.getVertexCount() > before);
}

void TestMeshOperations::testWeldVertices()
{
    MeshData mesh;
    mesh.vertices.resize(6);
    for (int i = 0; i < 6; ++i)
        mesh.vertices[i].position = QVector3D(i < 3 ? 0.0f : 1.0f, 0, 0);
    mesh.faces.append(Face{0,1,2});
    mesh.faces.append(Face{3,4,5});
    MeshData welded = MeshOperations::weldVertices(mesh, 0.01f);
    QVERIFY(welded.getVertexCount() <= mesh.getVertexCount());
}

void TestMeshOperations::testSubdivide()
{
    MeshData box = MeshOperations::createBox(1,1,1);
    MeshData subdivided = MeshOperations::subdivide(box, 1);
    QVERIFY(subdivided.getVertexCount() > box.getVertexCount());
    QVERIFY(subdivided.getTriangleCount() > box.getTriangleCount());
}

void TestMeshOperations::testDisplace()
{
    MeshData plane = MeshOperations::createPlane(2, 2, 4, 4);
    QImage heightmap(16, 16, QImage::Format_Grayscale8);
    heightmap.fill(128);
    MeshData displaced = MeshOperations::displace(plane, heightmap, 1.0f);
    QCOMPARE(displaced.getVertexCount(), plane.getVertexCount());
}

void TestMeshOperations::testFlipFaces()
{
    MeshData box = MeshOperations::createBox(1,1,1);
    auto normsBefore = box.normals;
    box.flipFaces();
    box.computeNormals();
}

void TestMeshOperations::testComputeNormals()
{
    MeshData box = MeshOperations::createBox(1,1,1);
    box.computeNormals();
    QCOMPARE(box.normals.size(), box.vertices.size());
    for (const auto& n : box.normals) {
        QVERIFY(!qIsNaN(n.length()));
    }
}

void TestMeshOperations::testBoundingBox()
{
    MeshData box = MeshOperations::createBox(2, 4, 6);
    box.computeBoundingBox();
    QCOMPARE(box.boundingBoxMin, QVector3D(-1, -2, -3));
    QCOMPARE(box.boundingBoxMax, QVector3D(1, 2, 3));
}

QTEST_MAIN(TestMeshOperations)
#include "test_MeshOperations.moc"
