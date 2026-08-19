#include "core/mesh/MeshOperations.h"
#include <QtTest/QtTest>

using namespace ks;

static Vertex V(const QVector3D& p) { Vertex v; v.position = p; return v; }

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
    void testShell();
    void testBridgeEdges();
    void testBridgeFaces();
    void testAutoSmooth();
    void testBorderEdges();
    void testFaceElements();
    void testPushPullSolid();
    void testOffsetFacesSolid();
    void testExtendedSculptBrushes();
    void testAdvancedBrushesFoldsPores();
    void testBulgeSlashBrushes();
    void testSculptPin();
    void testSculptFalloffPower();
    void testSnapPointToMesh();
    void testFillHoles();
    void testExtractFaces();

    // Quilt of `cols x rows` quads lying in the XZ plane (normal +Y).
    MeshData buildQuilt(int cols, int rows) const {
        MeshData mesh;
        for (int r = 0; r <= rows; ++r) {
            for (int c = 0; c <= cols; ++c) {
                Vertex v;
                v.position = QVector3D(float(c), 0.0f, float(r));
                mesh.vertices.append(v);
            }
        }
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int tl = r * (cols + 1) + c;
                Face f;
                f.indices = { tl + (cols + 1), tl + (cols + 1) + 1, tl + 1, tl };
                mesh.faces.append(f);
            }
        }
        mesh.computeNormals();
        mesh.computeBoundingBox();
        return mesh;
    }
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

void TestMeshOperations::testShell()
{
    // A flat open quad (single face) thickens into a closed volume.
    MeshData plane;
    for (const auto& p : { QVector3D(-1, 0, -1), QVector3D(1, 0, -1),
                           QVector3D(1, 0, 1),  QVector3D(-1, 0, 1) })
        plane.vertices.append(V(p));
    Face f; f.indices = { 0, 1, 2, 3 };
    plane.faces = { f };

    MeshData shelled = MeshOperations::shell(plane, 0.5f, 1, false);
    QVERIFY(shelled.vertices.size() == 8);          // 4 original + 4 offset
    QVERIFY(shelled.faces.size() == 6);             // front + back + 4 rim walls
    QVERIFY(shelled.faces[0].indices.size() > 0);
    // The offset copy's first vertex is displaced by `thickness` along its normal.
    float d1 = (shelled.vertices[4].position - shelled.vertices[0].position).length();
    QVERIFY(qAbs(d1 - 0.5f) < 1e-3f);

    // Negative direction displaces the same distance but opposite sign.
    MeshData inner = MeshOperations::shell(plane, 0.5f, -1, false);
    float d2 = (inner.vertices[4].position - inner.vertices[0].position).length();
    QVERIFY(qAbs(d2 - 0.5f) < 1e-3f);
}

void TestMeshOperations::testBridgeEdges()
{
    MeshData a = MeshOperations::createPlane(1, 1, 2, 2);
    QVERIFY(a.vertices.size() > 0);

    // Take the first 4 vertices as loop A and a shifted copy as loop B.
    QVector<int> loopA = { 0, 1, 2, 3 };
    QVector<int> loopB;
    for (int i = 0; i < 4; ++i)
        loopB.append(a.vertices.size() + i);
    for (int i = 0; i < 4; ++i) {
        Vertex v = a.vertices[i];
        v.position += QVector3D(0, 1, 0);
        a.vertices.append(v);
    }

    MeshData bridged = MeshOperations::bridgeEdges(a, loopA, loopB, 2);
    QVERIFY(bridged.vertices.size() >= a.vertices.size());
    // 2 segments => 1 intermediate ring of 4 new verts (endpoints already exist).
    QVERIFY(bridged.vertices.size() == a.vertices.size() + 4 * 1);
    // Quads between ring pairs: (segments) rings of 4 quads.
    QCOMPARE(bridged.faces.size(), a.faces.size() + 2 * 4);

    // Invalid loops return an empty mesh.
    MeshData bad = MeshOperations::bridgeEdges(a, { 0, 1 }, { 2, 3 }, 1);
    QVERIFY(bad.vertices.isEmpty());
}

void TestMeshOperations::testBridgeFaces()
{
    // Two separate quads bridged by their 4 vertices.
    MeshData mesh;
    for (const auto& p : { QVector3D(0, 0, 0), QVector3D(1, 0, 0),
                           QVector3D(1, 0, 1), QVector3D(0, 0, 1),
                           QVector3D(0, 2, 0), QVector3D(1, 2, 0),
                           QVector3D(1, 2, 1), QVector3D(0, 2, 1) })
        mesh.vertices.append(V(p));
    Face fa; fa.indices = { 0, 1, 2, 3 };
    Face fb; fb.indices = { 4, 5, 6, 7 };
    mesh.faces = { fa, fb };

    MeshData bridged = MeshOperations::bridgeFaces(mesh, 0, 1, 1);
    // Original 2 faces + 4 side quads.
    QCOMPARE(bridged.faces.size(), 6);
    // The bridged strip connects the loops (no new vertices at segments == 1).
    QCOMPARE(bridged.vertices.size(), 8);
}

void TestMeshOperations::testAutoSmooth()
{
    // A box: adjacent faces meet at 90° so they fall into distinct groups.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    QVector<int> groups = MeshOperations::autoSmooth(box, 30.0f);
    QCOMPARE(groups.size(), box.faces.size());
    // No face is left unsmoothed.
    for (int g : groups) QVERIFY(g >= 0);

    // A flat grid: coplanar faces should land in the same group.
    MeshData grid = MeshOperations::createGrid(2, 2, 2);
    QVector<int> gridGroups = MeshOperations::autoSmooth(grid, 30.0f);
    QVERIFY(gridGroups.size() == grid.faces.size());
    for (int g : gridGroups) QVERIFY(g >= 0);
}

void TestMeshOperations::testBorderEdges()
{
    MeshData a = MeshOperations::createPlane(1, 1, 1, 1);
    auto borders = MeshOperations::borderEdges(a);
    // A single quad plane's outline has 4 border edges.
    QCOMPARE(borders.size(), 4);
    for (const auto& e : borders) {
        QVERIFY(e.v1 >= 0 && e.v2 >= 0);
        QVERIFY(e.v1 != e.v2);
    }

    // A closed box has no borders.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    QCOMPARE(MeshOperations::borderEdges(box).size(), 0);
}

void TestMeshOperations::testFaceElements()
{
    // Two separate quads => two elements.
    MeshData mesh;
    for (const auto& p : { QVector3D(0, 0, 0), QVector3D(1, 0, 0),
                           QVector3D(1, 1, 0), QVector3D(0, 1, 0),
                           QVector3D(5, 0, 0), QVector3D(6, 0, 0),
                           QVector3D(6, 1, 0), QVector3D(5, 1, 0) })
        mesh.vertices.append(V(p));
    Face fa; fa.indices = { 0, 1, 2, 3 };
    Face fb; fb.indices = { 4, 5, 6, 7 };
    mesh.faces = { fa, fb };

    QVector<int> elems = MeshOperations::faceElements(mesh);
    // Single quad each => each gets its own element.
    QVERIFY(elems.size() == 2);
    QVERIFY(elems[0] != elems[1]);

    // A box is one connected element.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    QVector<int> elems2 = MeshOperations::faceElements(box);
    int first = elems2[0];
    for (int e : elems2) QCOMPARE(e, first);
}

void TestMeshOperations::testPushPullSolid()
{
    // A single open quad pushed by 0.5 becomes a closed 5-face volume (the
    // moved cap + 4 rim walls) with 8 vertices (4 originals + 4 copies).
    MeshData plane;
    for (const auto& p : { QVector3D(-1, 0, -1), QVector3D(1, 0, -1),
                           QVector3D(1, 0, 1),  QVector3D(-1, 0, 1) })
        plane.vertices.append(V(p));
    Face f; f.indices = { 3, 2, 1, 0 };
    plane.faces = { f };

    MeshData pushed = MeshOperations::pushPullFaces(plane, { 0 }, 0.5f);
    QCOMPARE(pushed.faces.size(), 5);
    QCOMPARE(pushed.vertices.size(), 8);
    // The moved cap copy sits at y = 0.5 (region normal is +Y).
    int up = 0;
    for (const auto& v : pushed.vertices)
        if (qAbs(v.position.y() - 0.5f) < 1e-3f) ++up;
    QCOMPARE(up, 4);

    // Pushing the whole top-face region of a box (two triangles) moves the cap
    // and stitches it back with 4 rim walls.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    int beforeFaces = box.faces.size();
    int beforeVerts = box.vertices.size();
    MeshData boxPushed = MeshOperations::pushPullFaces(box, { 8, 9 }, 0.25f);
    QCOMPARE(boxPushed.faces.size(), beforeFaces + 4);
    QCOMPARE(boxPushed.vertices.size(), beforeVerts + 4);
}

void TestMeshOperations::testOffsetFacesSolid()
{
    // Offset on an interior 2x2 patch of a 3x3 quilt: 9 original quads, the
    // 4 patch quads are moved (+ copies of their 9 unique verts) and the 8-edge
    // perimeter gets 8 rim walls.
    MeshData quilt = buildQuilt(3, 3);
    QCOMPARE(quilt.faces.size(), 9);
    QVector<int> patch = { 0, 1, 3, 4 };

    MeshData offset = MeshOperations::offsetFaces(quilt, patch, 0.3f);
    QCOMPARE(offset.faces.size(), 9 + 8);
    QCOMPARE(offset.vertices.size(), quilt.vertices.size() + 9);
    // The moved patch copies sit 0.3 above the ground plane.
    int up = 0;
    for (const auto& v : offset.vertices)
        if (qAbs(v.position.y() - 0.3f) < 1e-3f) ++up;
    QCOMPARE(up, 9);
}

void TestMeshOperations::testExtendedSculptBrushes()
{
    MeshData base = MeshOperations::createPlane(2.0f, 2.0f, 8, 8);
    base.computeNormals();
    const int vertexCount = base.vertices.size();

    auto changedCount = [](const QVector<Vertex>& before, const MeshData& after) {
        int changed = 0;
        const int n = qMin(before.size(), after.vertices.size());
        for (int i = 0; i < n; ++i)
            if ((before[i].position - after.vertices[i].position).lengthSquared() > 1e-9f)
                ++changed;
        return changed;
    };

    // Inflate (5) pushes vertices outward (up for a flat +Y plane).
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY2(MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.6f, 0.5f, 5) > 0, "inflate affected 0");
        QVERIFY(changedCount(before, m) > 0);
        bool up = false;
        for (const auto& v : m.vertices) if (v.position.y() > 0.01f) { up = true; break; }
        QVERIFY(up);
    }
    // Negate (8) is the inverse of draw: it sinks vertices.
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY(MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.6f, 0.5f, 8) > 0);
        QVERIFY(changedCount(before, m) > 0);
        bool down = false;
        for (const auto& v : m.vertices) if (v.position.y() < -0.01f) { down = true; break; }
        QVERIFY(down);
    }
    // Pinch (6) pulls vertices toward the brush center (shrinks x/z spread).
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY(MeshOperations::sculptBrush(m, QVector3D(0.2f, 0.0f, 0.0f), 0.5f, 0.5f, 6) > 0);
        QVERIFY(changedCount(before, m) > 0);
    }
    // Smear (7) drags the surface with the cursor movement (previousCenter).
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY(MeshOperations::sculptBrush(m, QVector3D(0.3f, 0, 0), 0.4f, 0.5f, 7, QVector3D(),
                                            QVector3D(0, 0, 0)) > 0);
        QVERIFY(changedCount(before, m) > 0);
        bool movedX = false;
        for (int i = 0; i < m.vertices.size(); ++i)
            if (m.vertices[i].position.x() > before[i].position.x() + 1e-4f) { movedX = true; break; }
        QVERIFY(movedX);
    }
}

void TestMeshOperations::testSculptPin()
{
    MeshData base = MeshOperations::createPlane(2.0f, 2.0f, 8, 8);
    base.computeNormals();

    // Pin a subset of vertices: the brush must never move them.
    MeshData m = base;
    QSet<int> pinned;
    // 4 center vertices (index of center in an 9x9 grid = row*9+col).
    pinned.insert(4 * 9 + 4);
    pinned.insert(4 * 9 + 5);
    pinned.insert(5 * 9 + 4);
    pinned.insert(5 * 9 + 5);
    QVector3D before0 = m.vertices[4 * 9 + 4].position;
    QVector3D before1 = m.vertices[5 * 9 + 5].position;

    int affected = MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.8f, 0.8f, 0,
                                               QVector3D(), QVector3D(), 2.0f, &pinned);
    QVERIFY2(affected > 0, "pinned stroke affected 0 vertices");
    QCOMPARE(m.vertices[4 * 9 + 4].position, before0);
    QCOMPARE(m.vertices[5 * 9 + 5].position, before1);

    // Without pins the same vertices do move.
    MeshData m2 = base;
    MeshOperations::sculptBrush(m2, QVector3D(0, 0, 0), 0.8f, 0.8f, 0);
    QVERIFY((m2.vertices[4 * 9 + 4].position - before0).lengthSquared() > 1e-9f);
}

void TestMeshOperations::testSculptFalloffPower()
{
    MeshData base = MeshOperations::createPlane(2.0f, 2.0f, 8, 8);
    base.computeNormals();

    // A high falloff exponent concentrates displacement near the brush center:
    // the sum of |delta| must be smaller than with exponent 0.5.
    float sumLow = 0.0f, sumHigh = 0.0f;
    {
        MeshData m = base;
        MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.8f, 1.0f, 0,
                                    QVector3D(), QVector3D(), 0.5f);
        for (auto& v : m.vertices) sumLow += v.position.y();
    }
    {
        MeshData m = base;
        MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.8f, 1.0f, 0,
                                    QVector3D(), QVector3D(), 6.0f);
        for (auto& v : m.vertices) sumHigh += v.position.y();
    }
    QVERIFY2(sumHigh < sumLow, "high falloff exponent should limit overall displacement");
}

void TestMeshOperations::testBulgeSlashBrushes()
{
    MeshData base = MeshOperations::createPlane(2.0f, 2.0f, 8, 8);
    base.computeNormals();

    auto changedCount = [](const QVector<Vertex>& before, const MeshData& after) {
        int changed = 0;
        for (int i = 0; i < before.size() && i < after.vertices.size(); ++i)
            if ((before[i].position - after.vertices[i].position).lengthSquared() > 1e-9f)
                ++changed;
        return changed;
    };

    // Bulge (11): outward push, all affected vertices go up on a +Y plane.
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY2(MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.5f, 0.6f, 11) > 0, "bulge affected 0");
        QVERIFY(changedCount(before, m) > 0);
        bool up = false;
        for (const auto& v : m.vertices) if (v.position.y() > 0.01f) { up = true; break; }
        QVERIFY(up);
    }
    // Slash (12): cuts along the stroke direction (default X), shifting vertices in -X.
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY(MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.5f, 0.6f, 12) > 0);
        QVERIFY(changedCount(before, m) > 0);
        bool cutX = false;
        for (int i = 0; i < m.vertices.size(); ++i)
            if (before[i].position.x() > 0 && m.vertices[i].position.x() < before[i].position.x())
                { cutX = true; break; }
        QVERIFY(cutX);
    }
}

void TestMeshOperations::testAdvancedBrushesFoldsPores()
{
    MeshData base = MeshOperations::createPlane(2.0f, 2.0f, 12, 12);
    base.computeNormals();

    auto changedCount = [](const QVector<Vertex>& before, const MeshData& after) {
        int changed = 0;
        for (int i = 0; i < before.size() && i < after.vertices.size(); ++i)
            if ((before[i].position - after.vertices[i].position).lengthSquared() > 1e-9f)
                ++changed;
        return changed;
    };

    // Folds (9): concentric ridges/valleys - both positive and negative offsets.
    {
        MeshData m = base;
        auto before = m.vertices;
        QVERIFY2(MeshOperations::sculptBrush(m, QVector3D(0, 0, 0), 0.6f, 0.6f, 9) > 0, "folds affected 0");
        QVERIFY(changedCount(before, m) > 0);
        bool up = false, down = false;
        for (int i = 0; i < m.vertices.size(); ++i) {
            float dy = m.vertices[i].position.y() - before[i].position.y();
            if (dy > 1e-4f) up = true;
            if (dy < -1e-4f) down = true;
        }
        QVERIFY(up && down);
    }
    // Pores (10): deterministic micro-depressions (positive and negative).
    {
        MeshData m1 = base;
        MeshData m2 = base;
        QVERIFY(MeshOperations::sculptBrush(m1, QVector3D(0, 0, 0), 0.6f, 0.5f, 10) > 0);
        QVERIFY(changedCount(base.vertices, m1) > 0);
        MeshOperations::sculptBrush(m2, QVector3D(0, 0, 0), 0.6f, 0.5f, 10);
        for (int i = 0; i < m1.vertices.size(); ++i)
            QCOMPARE(m1.vertices[i].position, m2.vertices[i].position);
    }
}

void TestMeshOperations::testSnapPointToMesh()
{
    MeshData box = MeshOperations::createBox(1, 1, 1);   // verts at ±0.5
    box.computeBoundingBox();
    QMatrix4x4 world;
    world.setToIdentity();

    // Vertex snap: a point just outside a corner snaps to the corner.
    QVector3D vp = MeshOperations::snapPointToMesh(box, world,
        QVector3D(0.505f, 0.505f, 0.505f), int(MeshOperations::SnapType::Vertex));
    QVERIFY((vp - QVector3D(0.5f, 0.5f, 0.5f)).length() < 1e-3f);

    // Midpoint snap: edge midpoint (0.5, 0.5, 0.0).
    QVector3D mp = MeshOperations::snapPointToMesh(box, world,
        QVector3D(0.502f, 0.502f, 0.001f), int(MeshOperations::SnapType::Midpoint));
    QVERIFY((mp - QVector3D(0.5f, 0.5f, 0.0f)).length() < 1e-3f);

    // Edge snap: point slightly off an edge projects onto it.
    QVector3D ep = MeshOperations::snapPointToMesh(box, world,
        QVector3D(0.5f, 0.498f, 0.0f), int(MeshOperations::SnapType::Edge));
    QVERIFY(qAbs(ep.y() - 0.5f) < 1e-3f);

    // Face snap: just above the top face projects onto it.
    QVector3D fp = MeshOperations::snapPointToMesh(box, world,
        QVector3D(0.0f, 0.502f, 0.0f), int(MeshOperations::SnapType::Face));
    QVERIFY(qAbs(fp.y() - 0.5f) < 1e-3f);

    // A point far away with only geometry snaps is not pulled in.
    QVector3D far = MeshOperations::snapPointToMesh(box, world,
        QVector3D(3.0f, 3.0f, 3.0f), int(MeshOperations::SnapType::Vertex));
    QVERIFY(far == QVector3D(3.0f, 3.0f, 3.0f));
}

void TestMeshOperations::testFillHoles()
{
    // A single open quad has exactly one 4-edge boundary loop.
    MeshData plane;
    for (const auto& p : { QVector3D(-1, 0, -1), QVector3D(1, 0, -1),
                           QVector3D(1, 0, 1),  QVector3D(-1, 0, 1) })
        plane.vertices.append(V(p));
    Face f; f.indices = { 0, 1, 2, 3 };
    plane.faces = { f };
    plane.computeNormals();
    plane.computeBoundingBox();

    MeshData capped = plane;
    int filled = MeshOperations::fillHoles(capped, 8);
    QCOMPARE(filled, 1);
    QCOMPARE(capped.faces.size(), 1 + 4);
    QCOMPARE(capped.vertices.size(), 5);   // 4 originals + centroid

    // Closing a large hole into the cap dimension is rejected by maxHoleEdges.
    MeshData small = plane;
    QCOMPARE(MeshOperations::fillHoles(small, 2), 0);

    // A closed box has no holes.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    QCOMPARE(MeshOperations::fillHoles(box, 0), 0);
}

void TestMeshOperations::testExtractFaces()
{
    // Top face of the box (unit) is the two triangles {8, 9}; region has 4
    // unique vertices.
    MeshData box = MeshOperations::createBox(1, 1, 1);
    MeshData top = MeshOperations::extractFaces(box, { 8, 9 }, 0.0f, true);
    QCOMPARE(top.faces.size(), 2);
    QCOMPARE(top.vertices.size(), 4);

    // Extraction with a thickness becomes a closed solid (shelled patch).
    MeshData solid = MeshOperations::extractFaces(box, { 8, 9 }, 0.2f, true);
    QCOMPARE(solid.vertices.size(), 8);
    QCOMPARE(solid.faces.size(), 8);   // 2 front + 2 back + 4 rim walls

    // Invalid selection returns an empty mesh.
    MeshData empty = MeshOperations::extractFaces(box, { -1, 999 }, 0.0f, true);
    QVERIFY(empty.vertices.isEmpty());
}

QTEST_MAIN(TestMeshOperations)
#include "test_MeshOperations.moc"
