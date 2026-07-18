#include "MeshOperations.h"
#include "ShapeKeyData.h"
#include "../FileFormat/MeshData.h"
#include <QVector3D>
#include <QVector2D>
#include <QDebug>
#include <QImage>
#include <QColor>
#include <QSet>
#include <cmath>
#include <algorithm>
#include <limits>

namespace ks {

QVector<int> MeshOperations::findEdge(const MeshData& mesh, int v1, int v2) {
    QVector<int> result;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const Face& face = mesh.faces[fi];
        for (int i = 0; i < face.indices.size(); ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % face.indices.size()];
            if ((a == v1 && b == v2) || (a == v2 && b == v1)) {
                result.append(fi);
            }
        }
    }
    return result;
}

bool MeshOperations::isEdge(const MeshData& mesh, int v1, int v2) {
    return !findEdge(mesh, v1, v2).isEmpty();
}

QVector3D MeshOperations::computeFaceNormal(const MeshData& mesh, int faceIndex) {
    const Face& face = mesh.faces[faceIndex];
    QVector3D v0 = mesh.vertices[face.indices[0]].position;
    QVector3D v1 = mesh.vertices[face.indices[1]].position;
    QVector3D v2 = mesh.vertices[face.indices[2]].position;
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;
    QVector3D normal = QVector3D::crossProduct(edge1, edge2).normalized();
    return normal;
}

void MeshOperations::ensureEdgeList(MeshData& mesh) {
    mesh.edges.clear();
    QSet<QPair<int, int>> edgeSet;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const Face& face = mesh.faces[fi];
        for (int i = 0; i < face.indices.size(); ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % face.indices.size()];
            QPair<int, int> edge = qMakePair(qMin(a, b), qMax(a, b));
            if (!edgeSet.contains(edge)) {
                edgeSet.insert(edge);
                mesh.edges.append(Edge(a, b));
            }
        }
    }
}

MeshData MeshOperations::createBox(float width, float height, float depth) {
    MeshData mesh;
    float hw = width * 0.5f, hh = height * 0.5f, hd = depth * 0.5f;
    
    QVector<QVector3D> verts = {
        {-hw, -hh, -hd}, {hw, -hh, -hd}, {hw, hh, -hd}, {-hw, hh, -hd},
        {-hw, -hh, hd}, {hw, -hh, hd}, {hw, hh, hd}, {-hw, hh, hd}
    };
    
    for (const auto& v : verts) {
        Vertex vertex;
        vertex.position = v;
        mesh.vertices.append(vertex);
    }
    
    QVector<int> indices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 6, 5, 1, 2, 6,
        3, 6, 2, 3, 7, 6,
        0, 1, 5, 5, 4, 0
    };
    
    for (int i = 0; i < indices.size(); i += 3) {
        Face face;
        face.indices = {indices[i], indices[i+1], indices[i+2]};
        mesh.faces.append(face);
    }
    
    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createSphere(float radius, int segments, int rings) {
    MeshData mesh;
    for (int r = 0; r <= rings; ++r) {
        float phi = M_PI * r / rings;
        float y = radius * cos(phi);
        float ringRadius = radius * sin(phi);
        
        for (int s = 0; s <= segments; ++s) {
            float theta = 2 * M_PI * s / segments;
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);
            
            Vertex vertex;
            vertex.position = QVector3D(x, y, z);
            vertex.normal = QVector3D(x, y, z).normalized();
            vertex.uv = QVector2D(float(s) / segments, 1.0f - float(r) / rings);
            mesh.vertices.append(vertex);
        }
    }
    
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int a = r * (segments + 1) + s;
            int b = a + segments + 1;
            
            Face face1, face2;
            face1.indices = {a, b, a + 1};
            face2.indices = {a + 1, b, b + 1};
            mesh.faces.append(face1);
            mesh.faces.append(face2);
        }
    }
    
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createCylinder(float radius, float height, int segments) {
    MeshData mesh;
    float hh = height * 0.5f;
    
    // Top and bottom centers
    Vertex topCenter, bottomCenter;
    topCenter.position = QVector3D(0, hh, 0);
    topCenter.normal = QVector3D(0, 1, 0);
    bottomCenter.position = QVector3D(0, -hh, 0);
    bottomCenter.normal = QVector3D(0, -1, 0);
    int topIdx = mesh.vertices.size();
    mesh.vertices.append(topCenter);
    int bottomIdx = mesh.vertices.size();
    mesh.vertices.append(bottomCenter);
    
    // Side vertices
    for (int s = 0; s <= segments; ++s) {
        float theta = 2 * M_PI * s / segments;
        float x = radius * cos(theta);
        float z = radius * sin(theta);
        
        Vertex topV, bottomV;
        topV.position = QVector3D(x, hh, z);
        topV.normal = QVector3D(cos(theta), 0, sin(theta));
        topV.uv = QVector2D(float(s) / segments, 1.0f);
        
        bottomV.position = QVector3D(x, -hh, z);
        bottomV.normal = QVector3D(cos(theta), 0, sin(theta));
        bottomV.uv = QVector2D(float(s) / segments, 0.0f);
        
        mesh.vertices.append(topV);
        mesh.vertices.append(bottomV);
    }
    
    // Top and bottom faces
    for (int s = 0; s < segments; ++s) {
        int t0 = 2 + s * 2;
        int t1 = 2 + ((s + 1) % segments) * 2;
        int b0 = 3 + s * 2;
        int b1 = 3 + ((s + 1) % segments) * 2;
        
        // Top face
        Face topFace;
        topFace.indices = {topIdx, t0, t1};
        mesh.faces.append(topFace);
        
        // Bottom face
        Face bottomFace;
        bottomFace.indices = {bottomIdx, b1, b0};
        mesh.faces.append(bottomFace);
        
        // Side faces
        Face f1, f2;
        f1.indices = {t0, b0, t1};
        f2.indices = {t1, b0, b1};
        mesh.faces.append(f1);
        mesh.faces.append(f2);
    }
    
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createCone(float radius, float height, int segments) {
    MeshData mesh;
    float hh = height * 0.5f;

    Vertex tipVertex;
    tipVertex.position = QVector3D(0, hh, 0);
    tipVertex.normal = QVector3D(0, 1, 0);
    int tipIdx = mesh.vertices.size();
    mesh.vertices.append(tipVertex);

    Vertex baseCenter;
    baseCenter.position = QVector3D(0, -hh, 0);
    baseCenter.normal = QVector3D(0, -1, 0);
    int baseIdx = mesh.vertices.size();
    mesh.vertices.append(baseCenter);

    for (int s = 0; s <= segments; ++s) {
        float theta = 2.0f * float(M_PI) * s / segments;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        Vertex sideV;
        sideV.position = QVector3D(x, -hh, z);
        sideV.normal = QVector3D(cosf(theta), radius / height, sinf(theta)).normalized();
        sideV.uv = QVector2D(float(s) / segments, 0.0f);
        mesh.vertices.append(sideV);
    }

    for (int s = 0; s < segments; ++s) {
        int baseV0 = 2 + s;
        int baseV1 = 2 + ((s + 1) % segments);

        Face sideFace;
        sideFace.indices = {tipIdx, baseV1, baseV0};
        mesh.faces.append(sideFace);

        Face bottomFace;
        bottomFace.indices = {baseIdx, baseV0, baseV1};
        mesh.faces.append(bottomFace);
    }

    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createPlane(float width, float height, int subdivX, int subdivY) {
    MeshData mesh;
    float hw = width * 0.5f, hh = height * 0.5f;
    
    for (int v = 0; v <= subdivY; ++v) {
        float fy = float(v) / subdivY;
        float py = -hh + fy * height;
        
        for (int u = 0; u <= subdivX; ++u) {
            float fx = float(u) / subdivX;
            float px = -hw + fx * width;
            
            Vertex vertex;
            vertex.position = QVector3D(px, 0, py);
            vertex.normal = QVector3D(0, 1, 0);
            vertex.uv = QVector2D(fx, 1.0f - fy);
            mesh.vertices.append(vertex);
        }
    }
    
    for (int v = 0; v < subdivY; ++v) {
        for (int u = 0; u < subdivX; ++u) {
            int a = v * (subdivX + 1) + u;
            int b = a + 1;
            int c = a + subdivX + 1;
            int d = c + 1;
            
            Face f1, f2;
            f1.indices = {a, c, b};
            f2.indices = {b, c, d};
            mesh.faces.append(f1);
            mesh.faces.append(f2);
        }
    }
    
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createTorus(float majorRadius, float minorRadius, int majorSeg, int minorSeg) {
    MeshData mesh;
    
    for (int m = 0; m <= majorSeg; ++m) {
        float u = float(m) / majorSeg;
        float theta = 2 * M_PI * u;
        
        for (int n = 0; n <= minorSeg; ++n) {
            float v = float(n) / minorSeg;
            float phi = 2 * M_PI * v;
            
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);
            
            float x = (majorRadius + minorRadius * cosPhi) * cosTheta;
            float y = (majorRadius + minorRadius * cosPhi) * sinTheta;
            float z = minorRadius * sinPhi;
            
            Vertex vertex;
            vertex.position = QVector3D(x, y, z);
            vertex.normal = QVector3D(cosPhi * cosTheta, cosPhi * sinTheta, sinPhi).normalized();
            vertex.uv = QVector2D(u, v);
            mesh.vertices.append(vertex);
        }
    }
    
    for (int m = 0; m < majorSeg; ++m) {
        for (int n = 0; n < minorSeg; ++n) {
            int a = m * (minorSeg + 1) + n;
            int b = a + 1;
            int c = a + minorSeg + 1;
            int d = c + 1;
            
            Face f1, f2;
            f1.indices = {a, c, b};
            f2.indices = {b, c, d};
            mesh.faces.append(f1);
            mesh.faces.append(f2);
        }
    }
    
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createGrid(float width, float height, int uSubdiv, int vSubdiv) {
    return createPlane(width, height, uSubdiv, vSubdiv);
}

MeshData MeshOperations::createIcosphere(float radius, int subdivisions) {
    MeshData mesh = createSphere(radius, 20, 10);
    for (int i = 0; i < subdivisions; ++i) {
        mesh = subdivide(mesh, 1);
    }
    return mesh;
}

// Boolean operations
MeshData MeshOperations::booleanUnion(const MeshData& a, const MeshData& b)
{
    MeshData result = a;
    mergeMeshes(result, b);
    // Remove internal faces where meshes overlap
    QVector<bool> faceRemoved(result.faces.size(), false);
    for (int i = 0; i < result.faces.size(); ++i) {
        const auto& fi = result.faces[i];
        if (fi.indices.size() < 3) continue;
        QVector3D center;
        for (int idx : fi.indices) center += result.vertices[idx].position;
        center /= fi.indices.size();

        // Check if face center is inside the other mesh
        bool insideA = false, insideB = false;
        QVector3D normal = computeFaceNormal(result, i);

        // Simple winding-based inside test
        int windingA = 0, windingB = 0;
        for (const auto& face : a.faces) {
            if (face.indices.size() < 3) continue;
            QVector3D fc;
            for (int idx : face.indices) fc += a.vertices[idx].position;
            fc /= face.indices.size();
            if ((fc - center).length() < 0.01f) windingA++;
        }
        for (const auto& face : b.faces) {
            if (face.indices.size() < 3) continue;
            QVector3D fc;
            for (int idx : face.indices) fc += b.vertices[idx].position;
            fc /= face.indices.size();
            if ((fc - center).length() < 0.01f) windingB++;
        }
        // Remove faces that are inside both meshes (internal)
        if (windingA > 0 && windingB > 0) faceRemoved[i] = true;
    }

    MeshData filtered;
    for (int i = 0; i < result.faces.size(); ++i) {
        if (!faceRemoved[i]) filtered.faces.append(result.faces[i]);
    }
    filtered.vertices = result.vertices;
    filtered.computeNormals();
    filtered.computeBoundingBox();
    return filtered;
}

MeshData MeshOperations::booleanDifference(const MeshData& a, const MeshData& b)
{
    MeshData result = a;
    // Clip faces of a that are inside b
    QVector<bool> faceRemoved(result.faces.size(), false);
    for (int i = 0; i < result.faces.size(); ++i) {
        const auto& fi = result.faces[i];
        if (fi.indices.size() < 3) continue;
        QVector3D center;
        for (int idx : fi.indices) center += result.vertices[idx].position;
        center /= fi.indices.size();

        // Point-in-mesh test using ray casting against b
        QVector3D rayDir(0, 1, 0);
        int intersections = 0;
        for (const auto& face : b.faces) {
            if (face.indices.size() < 3) continue;
            QVector3D v0 = b.vertices[face.indices[0]].position;
            QVector3D v1 = b.vertices[face.indices[1]].position;
            QVector3D v2 = b.vertices[face.indices[2]].position;

            QVector3D edge1 = v1 - v0;
            QVector3D edge2 = v2 - v0;
            QVector3D h = QVector3D::crossProduct(rayDir, edge2);
            float a2 = QVector3D::dotProduct(edge1, h);
            if (std::abs(a2) < 1e-6f) continue;
            float f = 1.0f / a2;
            QVector3D s = center - v0;
            float u = f * QVector3D::dotProduct(s, h);
            if (u < 0.0f || u > 1.0f) continue;
            QVector3D q = QVector3D::crossProduct(s, edge1);
            float v = f * QVector3D::dotProduct(rayDir, q);
            if (v < 0.0f || u + v > 1.0f) continue;
            float t = f * QVector3D::dotProduct(edge2, q);
            if (t > 1e-6f) intersections++;
        }
        if (intersections % 2 == 1) faceRemoved[i] = true;
    }

    MeshData filtered;
    for (int i = 0; i < result.faces.size(); ++i) {
        if (!faceRemoved[i]) filtered.faces.append(result.faces[i]);
    }
    filtered.vertices = result.vertices;
    filtered.computeNormals();
    filtered.computeBoundingBox();
    return filtered;
}

MeshData MeshOperations::booleanIntersection(const MeshData& a, const MeshData& b)
{
    // Keep only faces of a that are inside b
    MeshData result;
    result.vertices = a.vertices;

    for (int i = 0; i < a.faces.size(); ++i) {
        const auto& fi = a.faces[i];
        if (fi.indices.size() < 3) continue;
        QVector3D center;
        for (int idx : fi.indices) center += a.vertices[idx].position;
        center /= fi.indices.size();

        QVector3D rayDir(0, 1, 0);
        int intersections = 0;
        for (const auto& face : b.faces) {
            if (face.indices.size() < 3) continue;
            QVector3D v0 = b.vertices[face.indices[0]].position;
            QVector3D v1 = b.vertices[face.indices[1]].position;
            QVector3D v2 = b.vertices[face.indices[2]].position;

            QVector3D edge1 = v1 - v0;
            QVector3D edge2 = v2 - v0;
            QVector3D h = QVector3D::crossProduct(rayDir, edge2);
            float a2 = QVector3D::dotProduct(edge1, h);
            if (std::abs(a2) < 1e-6f) continue;
            float f = 1.0f / a2;
            QVector3D s = center - v0;
            float u = f * QVector3D::dotProduct(s, h);
            if (u < 0.0f || u > 1.0f) continue;
            QVector3D q = QVector3D::crossProduct(s, edge1);
            float v = f * QVector3D::dotProduct(rayDir, q);
            if (v < 0.0f || u + v > 1.0f) continue;
            float t = f * QVector3D::dotProduct(edge2, q);
            if (t > 1e-6f) intersections++;
        }
        if (intersections % 2 == 1) result.faces.append(fi);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::booleanXor(const MeshData& a, const MeshData& b)
{
    // XOR = (A - B) + (B - A)
    MeshData diffAB = booleanDifference(a, b);
    MeshData diffBA = booleanDifference(b, a);
    mergeMeshes(diffAB, diffBA);
    diffAB.computeNormals();
    diffAB.computeBoundingBox();
    return diffAB;
}

// Extrude
MeshData MeshOperations::extrude(const MeshData& mesh, const QVector3D& direction, float distance, bool individualFaces)
{
    MeshData result;
    result.vertices = mesh.vertices;

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        // Compute face normal
        QVector3D v0 = mesh.vertices[face.indices[0]].position;
        QVector3D v1 = mesh.vertices[face.indices[1]].position;
        QVector3D v2 = mesh.vertices[face.indices[2]].position;
        QVector3D faceNormal = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
        QVector3D extrudeDir = direction.length() > 0 ? direction.normalized() : faceNormal;
        QVector3D offset = extrudeDir * distance;

        // Create back face vertices
        QVector<int> newIndices;
        for (int idx : face.indices) {
            Vertex v = mesh.vertices[idx];
            v.position += offset;
            v.normal = faceNormal;
            newIndices.append(result.vertices.size());
            result.vertices.append(v);
        }

        // Add side faces (connecting original to extruded)
        for (int i = 0; i < face.indices.size(); ++i) {
            int next = (i + 1) % face.indices.size();
            Face sideFace;
            sideFace.indices = {face.indices[i], face.indices[next], newIndices[next], newIndices[i]};
            sideFace.normal = QVector3D::crossProduct(
                result.vertices[face.indices[next]].position - result.vertices[face.indices[i]].position,
                result.vertices[newIndices[i]].position - result.vertices[face.indices[i]].position
            ).normalized();
            result.faces.append(sideFace);
        }

        // Add front cap (reversed winding)
        Face cap;
        for (int i = face.indices.size() - 1; i >= 0; --i) {
            cap.indices.append(newIndices[i]);
        }
        cap.normal = faceNormal;
        result.faces.append(cap);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::extrudeFaces(const MeshData& mesh, const QVector<QVector3D>& directions)
{
    MeshData result;
    result.vertices = mesh.vertices;

    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto& face = mesh.faces[fi];
        if (face.indices.size() < 3) continue;

        QVector3D dir = (fi < directions.size()) ? directions[fi] : QVector3D(0, 1, 0);
        QVector3D offset = dir;

        QVector<int> newIndices;
        for (int idx : face.indices) {
            Vertex v = mesh.vertices[idx];
            v.position += offset;
            newIndices.append(result.vertices.size());
            result.vertices.append(v);
        }

        for (int i = 0; i < face.indices.size(); ++i) {
            int next = (i + 1) % face.indices.size();
            Face sideFace;
            sideFace.indices = {face.indices[i], face.indices[next], newIndices[next], newIndices[i]};
            result.faces.append(sideFace);
        }

        Face cap;
        for (int i = face.indices.size() - 1; i >= 0; --i) cap.indices.append(newIndices[i]);
        result.faces.append(cap);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Bevel
MeshData MeshOperations::bevelEdges(const MeshData& mesh, float distance, int segments, float angleLimit)
{
    MeshData result;
    result.vertices = mesh.vertices;

    // Build edge-to-face adjacency
    QMap<QPair<int,int>, QVector<int>> edgeFaces;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const auto& face = mesh.faces[fi];
        for (int i = 0; i < face.indices.size(); ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % face.indices.size()];
            edgeFaces[qMakePair(qMin(a, b), qMax(a, b))].append(fi);
        }
    }

    for (auto it = edgeFaces.begin(); it != edgeFaces.end(); ++it) {
        if (it.value().size() < 2) continue;

        // Check angle between adjacent faces
        int fi0 = it.value()[0], fi1 = it.value()[1];
        QVector3D n0 = computeFaceNormal(mesh, fi0);
        QVector3D n1 = computeFaceNormal(mesh, fi1);
        float angle = std::acos(qBound(-1.0f, QVector3D::dotProduct(n0, n1), 1.0f));
        if (angle > angleLimit) continue;

        int a = it.key().first, b = it.key().second;
        QVector3D edgeMid = (mesh.vertices[a].position + mesh.vertices[b].position) * 0.5f;
        QVector3D bevelDir = (n0 + n1).normalized() * distance;

        // Create bevel vertices
        Vertex va = mesh.vertices[a]; va.position += bevelDir;
        Vertex vb = mesh.vertices[b]; vb.position += bevelDir;
        int newA = result.vertices.size(); result.vertices.append(va);
        int newB = result.vertices.size(); result.vertices.append(vb);

        // Create bevel face(s)
        for (int s = 0; s <= segments; ++s) {
            float t = float(s) / segments;
            Vertex v;
            v.position = mesh.vertices[a].position * (1 - t) + mesh.vertices[b].position * t + bevelDir;
            // Add face connecting to original edge
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::bevelVertices(const MeshData& mesh, float distance)
{
    MeshData result;
    result.vertices = mesh.vertices;

    for (int vi = 0; vi < mesh.vertices.size(); ++vi) {
        // Find all faces touching this vertex
        QVector3D avgNormal;
        int faceCount = 0;
        for (const auto& face : mesh.faces) {
            if (face.indices.contains(vi)) {
                avgNormal += computeFaceNormal(mesh, mesh.faces.indexOf(face));
                faceCount++;
            }
        }
        if (faceCount == 0) continue;
        avgNormal /= faceCount;

        // Create bevel ring around vertex
        QVector<int> adjacentVerts;
        for (const auto& face : mesh.faces) {
            for (int i = 0; i < face.indices.size(); ++i) {
                if (face.indices[i] == vi) {
                    int next = face.indices[(i + 1) % face.indices.size()];
                    int prev = face.indices[(i + face.indices.size() - 1) % face.indices.size()];
                    if (!adjacentVerts.contains(next)) adjacentVerts.append(next);
                    if (!adjacentVerts.contains(prev)) adjacentVerts.append(prev);
                }
            }
        }

        for (int av : adjacentVerts) {
            Vertex v = mesh.vertices[vi];
            QVector3D toNeighbor = (mesh.vertices[av].position - v.position).normalized();
            v.position += avgNormal * distance;
            v.position += toNeighbor * distance * 0.5f;
            result.vertices.append(v);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Inset
MeshData MeshOperations::insetFaces(const MeshData& mesh, float distance, float depth)
{
    MeshData result;
    result.vertices = mesh.vertices;

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        // Compute face center and normal
        QVector3D center;
        for (int idx : face.indices) center += mesh.vertices[idx].position;
        center /= face.indices.size();

        QVector3D normal = face.normal.length() > 0 ? face.normal : computeFaceNormal(mesh, mesh.faces.indexOf(face));

        // Create inset vertices
        QVector<int> insetIndices;
        for (int idx : face.indices) {
            Vertex v = mesh.vertices[idx];
            QVector3D dir = (center - v.position);
            float len = dir.length();
            if (len > 0.0001f) {
                dir.normalize();
                v.position += dir * distance;
                v.position += normal * depth;
            }
            insetIndices.append(result.vertices.size());
            result.vertices.append(v);
        }

        // Connect inset to original
        for (int i = 0; i < face.indices.size(); ++i) {
            int next = (i + 1) % face.indices.size();
            Face ringFace;
            ringFace.indices = {face.indices[i], face.indices[next], insetIndices[next], insetIndices[i]};
            result.faces.append(ringFace);
        }

        // Add inset face
        Face insetFace;
        insetFace.indices = insetIndices;
        insetFace.normal = normal;
        result.faces.append(insetFace);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Modifiers
MeshData MeshOperations::weldVertices(const MeshData& mesh, float threshold)
{
    MeshData result;
    QVector<int> remap(mesh.vertices.size(), -1);

    for (int i = 0; i < mesh.vertices.size(); ++i) {
        if (remap[i] >= 0) continue;
        remap[i] = result.vertices.size();
        result.vertices.append(mesh.vertices[i]);

        for (int j = i + 1; j < mesh.vertices.size(); ++j) {
            if (remap[j] >= 0) continue;
            float dist = (mesh.vertices[i].position - mesh.vertices[j].position).length();
            if (dist < threshold) remap[j] = remap[i];
        }
    }

    for (const auto& face : mesh.faces) {
        Face newFace;
        for (int idx : face.indices) {
            newFace.indices.append(remap[idx]);
        }
        // Remove degenerate faces
        QSet<int> unique(newFace.indices.begin(), newFace.indices.end());
        if (unique.size() >= 3) result.faces.append(newFace);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::dissolveEdges(const MeshData& mesh, const QVector<int>& edgeIndices)
{
    // Mark edges to dissolve, then merge adjacent faces
    QSet<int> dissolveEdges(edgeIndices.begin(), edgeIndices.end());
    MeshData result = mesh;
    QVector<bool> faceRemoved(result.faces.size(), false);

    for (int edgeIdx : dissolveEdges) {
        if (edgeIdx < 0 || edgeIdx >= mesh.edges.size()) continue;
        int v1 = mesh.edges[edgeIdx].v1;
        int v2 = mesh.edges[edgeIdx].v2;

        // Find faces sharing this edge
        for (int fi = 0; fi < result.faces.size(); ++fi) {
            if (faceRemoved[fi]) continue;
            const auto& face = result.faces[fi];
            for (int i = 0; i < face.indices.size(); ++i) {
                int a = face.indices[i];
                int b = face.indices[(i + 1) % face.indices.size()];
                if ((a == v1 && b == v2) || (a == v2 && b == v1)) {
                    faceRemoved[fi] = true;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < result.faces.size(); ++i) {
        if (!faceRemoved[i]) {
            // Remove dissolved vertices from face
            Face newFace;
            for (int idx : result.faces[i].indices) {
                if (!dissolveEdges.contains(idx)) {
                    newFace.indices.append(idx);
                }
            }
            if (newFace.indices.size() >= 3) {
                result.faces.append(newFace);
            }
        }
    }

    // Remove faces that were marked
    MeshData filtered;
    for (int i = 0; i < result.faces.size(); ++i) {
        if (i < faceRemoved.size() && !faceRemoved[i]) {
            filtered.faces.append(result.faces[i]);
        } else if (i >= faceRemoved.size()) {
            filtered.faces.append(result.faces[i]);
        }
    }
    filtered.vertices = result.vertices;
    filtered.computeNormals();
    filtered.computeBoundingBox();
    return filtered;
}

MeshData MeshOperations::dissolveFaces(const MeshData& mesh, const QVector<int>& faceIndices)
{
    QSet<int> removeFaces(faceIndices.begin(), faceIndices.end());
    MeshData result;
    result.vertices = mesh.vertices;

    // Collect all edges of dissolved faces to find boundary
    QMap<QPair<int,int>, int> boundaryEdgeCount;
    for (int fi : faceIndices) {
        if (fi < 0 || fi >= mesh.faces.size()) continue;
        const auto& face = mesh.faces[fi];
        for (int i = 0; i < face.indices.size(); ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % face.indices.size()];
            auto edge = qMakePair(qMin(a, b), qMax(a, b));
            boundaryEdgeCount[edge]++;
        }
    }

    // Keep non-dissolved faces
    for (int i = 0; i < mesh.faces.size(); ++i) {
        if (!removeFaces.contains(i)) {
            result.faces.append(mesh.faces[i]);
        }
    }

    // Create merged face from boundary edges (boundary edges appear exactly once)
    QVector<int> boundaryVerts;
    for (auto it = boundaryEdgeCount.begin(); it != boundaryEdgeCount.end(); ++it) {
        if (it.value() == 1) {
            if (!boundaryVerts.contains(it.key().first)) boundaryVerts.append(it.key().first);
            if (!boundaryVerts.contains(it.key().second)) boundaryVerts.append(it.key().second);
        }
    }

    if (boundaryVerts.size() >= 3) {
        Face mergedFace;
        mergedFace.indices = boundaryVerts;
        result.faces.append(mergedFace);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::dissolveVertices(const MeshData& mesh, const QVector<int>& vertexIndices)
{
    QSet<int> removeVerts(vertexIndices.begin(), vertexIndices.end());
    MeshData result;
    result.vertices = mesh.vertices;

    // Compute average position of dissolved vertices
    QVector3D avgPos;
    int count = 0;
    for (int vi : vertexIndices) {
        if (vi >= 0 && vi < mesh.vertices.size()) {
            avgPos += mesh.vertices[vi].position;
            count++;
        }
    }
    if (count > 0) avgPos /= count;

    // Replace dissolved vertices in faces
    for (const auto& face : mesh.faces) {
        Face newFace;
        for (int idx : face.indices) {
            if (removeVerts.contains(idx)) {
                newFace.indices.append(idx); // Keep for now, will be merged
            } else {
                newFace.indices.append(idx);
            }
        }
        // Remove duplicate indices
        QVector<int> unique;
        QSet<int> seen;
        for (int idx : newFace.indices) {
            if (!seen.contains(idx)) {
                seen.insert(idx);
                unique.append(idx);
            }
        }
        if (unique.size() >= 3) {
            Face filtered;
            filtered.indices = unique;
            result.faces.append(filtered);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Advanced
MeshData MeshOperations::loft(const QVector<MeshData>& profiles, bool close)
{
    if (profiles.size() < 2) return MeshData();

    MeshData result;
    int profileSize = profiles[0].vertices.size();

    // Ensure all profiles have same vertex count
    for (const auto& profile : profiles) {
        if (profile.vertices.size() != profileSize) return MeshData();
    }

    // Create vertices by stacking profiles
    for (const auto& profile : profiles) {
        result.vertices.append(profile.vertices);
    }

    // Create faces between consecutive profiles
    for (int p = 0; p < profiles.size() - 1; ++p) {
        int offset = p * profileSize;
        int nextOffset = (p + 1) * profileSize;

        for (int v = 0; v < profileSize; ++v) {
            int next = (v + 1) % profileSize;
            Face f1, f2;
            f1.indices = {offset + v, nextOffset + v, nextOffset + next, offset + next};
            result.faces.append(f1);
        }
    }

    // Close if requested (connect last profile to first)
    if (close && profiles.size() > 2) {
        int lastOffset = (profiles.size() - 1) * profileSize;
        for (int v = 0; v < profileSize; ++v) {
            int next = (v + 1) % profileSize;
            Face f;
            f.indices = {lastOffset + v, v, next, lastOffset + next};
            result.faces.append(f);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::sweep(const MeshData& profile, const QVector<QMatrix4x4>& transforms, bool close)
{
    if (transforms.size() < 2) return MeshData();

    MeshData result;
    int profileSize = profile.vertices.size();

    // Create vertices along path
    for (const auto& transform : transforms) {
        for (const auto& v : profile.vertices) {
            Vertex sv = v;
            sv.position = transform.map(sv.position);
            sv.normal = transform.map(sv.normal);
            result.vertices.append(sv);
        }
    }

    // Create faces between transforms
    for (int t = 0; t < transforms.size() - 1; ++t) {
        int offset = t * profileSize;
        int nextOffset = (t + 1) * profileSize;

        for (int v = 0; v < profileSize; ++v) {
            int next = (v + 1) % profileSize;
            Face f;
            f.indices = {offset + v, nextOffset + v, nextOffset + next, offset + next};
            result.faces.append(f);
        }
    }

    if (close) {
        int lastOffset = (transforms.size() - 1) * profileSize;
        for (int v = 0; v < profileSize; ++v) {
            int next = (v + 1) % profileSize;
            Face f;
            f.indices = {lastOffset + v, v, next, lastOffset + next};
            result.faces.append(f);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::spin(const MeshData& profile, const QVector3D& axis, float angle, int steps)
{
    if (steps < 2) return profile;

    QVector<QMatrix4x4> transforms;
    for (int i = 0; i <= steps; ++i) {
        float t = float(i) / steps;
        float currentAngle = angle * t;
        QMatrix4x4 mat;
        if (std::abs(axis.x()) > 0.9f) mat.rotate(qRadiansToDegrees(currentAngle), 1, 0, 0);
        else if (std::abs(axis.y()) > 0.9f) mat.rotate(qRadiansToDegrees(currentAngle), 0, 1, 0);
        else mat.rotate(qRadiansToDegrees(currentAngle), 0, 0, 1);
        transforms.append(mat);
    }

    return sweep(profile, transforms, false);
}

// Subdivision
MeshData MeshOperations::subdivide(const MeshData& mesh, int levels)
{
    MeshData result = mesh;
    for (int level = 0; level < levels; ++level) {
        MeshData subdivided;
        subdivided.vertices = result.vertices;

        for (const auto& face : result.faces) {
            if (face.indices.size() < 3) continue;

            // Compute face center
            QVector3D faceCenter;
            for (int idx : face.indices) faceCenter += result.vertices[idx].position;
            faceCenter /= face.indices.size();

            int centerIdx = subdivided.vertices.size();
            Vertex centerV;
            centerV.position = faceCenter;
            subdivided.vertices.append(centerV);

            // Compute edge midpoints
            QVector<int> edgeMidpoints;
            for (int i = 0; i < face.indices.size(); ++i) {
                int a = face.indices[i];
                int b = face.indices[(i + 1) % face.indices.size()];
                QVector3D midpoint = (result.vertices[a].position + result.vertices[b].position) * 0.5f;
                Vertex midV;
                midV.position = midpoint;
                edgeMidpoints.append(subdivided.vertices.size());
                subdivided.vertices.append(midV);
            }

            // Create triangles from face center to edge midpoints
            for (int i = 0; i < face.indices.size(); ++i) {
                int next = (i + 1) % face.indices.size();
                Face tri;
                tri.indices = {edgeMidpoints[i], centerIdx, edgeMidpoints[next]};
                subdivided.faces.append(tri);
            }
        }

        result = subdivided;
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::unsubdivide(const MeshData& mesh, float detail)
{
    Q_UNUSED(detail);
    // Unsubdivide is complex - simplify by merging triangles into quads
    MeshData result;
    result.vertices = mesh.vertices;

    QVector<bool> faceUsed(mesh.faces.size(), false);
    for (int i = 0; i < mesh.faces.size(); ++i) {
        if (faceUsed[i]) continue;
        if (mesh.faces[i].indices.size() != 3) {
            result.faces.append(mesh.faces[i]);
            faceUsed[i] = true;
            continue;
        }

        // Find adjacent triangle to form quad
        for (int j = i + 1; j < mesh.faces.size(); ++j) {
            if (faceUsed[j] || mesh.faces[j].indices.size() != 3) continue;

            // Check if they share an edge
            QSet<int> verts1(mesh.faces[i].indices.begin(), mesh.faces[i].indices.end());
            QSet<int> verts2(mesh.faces[j].indices.begin(), mesh.faces[j].indices.end());
            QSet<int> shared = verts1 & verts2;

            if (shared.size() == 2) {
                // Merge into quad
                QVector<int> allVerts;
                for (int v : verts1) {
                    if (!shared.contains(v)) allVerts.append(v);
                }
                for (int v : shared) allVerts.append(v);
                for (int v : verts2) {
                    if (!shared.contains(v)) allVerts.append(v);
                }

                Face quad;
                quad.indices = allVerts;
                result.faces.append(quad);
                faceUsed[i] = true;
                faceUsed[j] = true;
                break;
            }
        }

        if (!faceUsed[i]) {
            result.faces.append(mesh.faces[i]);
            faceUsed[i] = true;
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::triangulate(const MeshData& mesh)
{
    MeshData result;
    result.vertices = mesh.vertices;

    for (const auto& face : mesh.faces) {
        if (face.indices.size() < 3) continue;

        // Fan triangulation
        for (int i = 1; i < face.indices.size() - 1; ++i) {
            Face tri;
            tri.indices = {face.indices[0], face.indices[i], face.indices[i + 1]};
            tri.normal = face.normal;
            result.faces.append(tri);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::quadrangulate(const MeshData& mesh)
{
    // Try to merge adjacent triangles into quads
    MeshData result;
    result.vertices = mesh.vertices;

    QVector<bool> faceUsed(mesh.faces.size(), false);
    for (int i = 0; i < mesh.faces.size(); ++i) {
        if (faceUsed[i]) continue;

        if (mesh.faces[i].indices.size() != 3) {
            result.faces.append(mesh.faces[i]);
            faceUsed[i] = true;
            continue;
        }

        bool merged = false;
        for (int j = i + 1; j < mesh.faces.size(); ++j) {
            if (faceUsed[j] || mesh.faces[j].indices.size() != 3) continue;

            QSet<int> verts1(mesh.faces[i].indices.begin(), mesh.faces[i].indices.end());
            QSet<int> verts2(mesh.faces[j].indices.begin(), mesh.faces[j].indices.end());
            QSet<int> shared = verts1 & verts2;

            if (shared.size() == 2) {
                QVector<int> allVerts;
                for (int v : verts1) allVerts.append(v);
                for (int v : verts2) {
                    if (!shared.contains(v)) allVerts.append(v);
                }

                // Order vertices properly
                Face quad;
                QSet<int> unique = verts1 | verts2;
                for (int v : unique) quad.indices.append(v);
                result.faces.append(quad);
                faceUsed[i] = true;
                faceUsed[j] = true;
                merged = true;
                break;
            }
        }

        if (!merged) {
            result.faces.append(mesh.faces[i]);
            faceUsed[i] = true;
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Transform
MeshData MeshOperations::mirror(const MeshData& mesh, const QVector3D& axis, float offset)
{
    MeshData result = mesh;
    int vertexCount = mesh.vertices.size();

    // Mirror vertices
    for (const auto& v : mesh.vertices) {
        Vertex mv = v;
        if (std::abs(axis.x()) > 0.5f) mv.position.setX(-mv.position.x() + offset);
        if (std::abs(axis.y()) > 0.5f) mv.position.setY(-mv.position.y() + offset);
        if (std::abs(axis.z()) > 0.5f) mv.position.setZ(-mv.position.z() + offset);
        mv.normal.setX(-mv.normal.x());
        mv.normal.setY(-mv.normal.y());
        mv.normal.setZ(-mv.normal.z());
        result.vertices.append(mv);
    }

    // Mirror faces (reversed winding)
    for (const auto& face : mesh.faces) {
        Face mirrored;
        for (int i = face.indices.size() - 1; i >= 0; --i) {
            mirrored.indices.append(face.indices[i] + vertexCount);
        }
        result.faces.append(mirrored);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::array(const MeshData& mesh, int count, const QVector3D& offset)
{
    if (count < 1) return mesh;

    MeshData result;
    int vertexCount = mesh.vertices.size();

    for (int i = 0; i < count; ++i) {
        QVector3D translation = offset * i;
        int baseVertex = result.vertices.size();

        for (const auto& v : mesh.vertices) {
            Vertex av = v;
            av.position += translation;
            result.vertices.append(av);
        }

        for (const auto& face : mesh.faces) {
            Face af;
            for (int idx : face.indices) {
                af.indices.append(idx + baseVertex);
            }
            result.faces.append(af);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::radialArray(const MeshData& mesh, int count, const QVector3D& axis, float angle)
{
    if (count < 2) return mesh;

    MeshData result;
    float stepAngle = angle / count;

    for (int i = 0; i < count; ++i) {
        QMatrix4x4 rotation;
        if (std::abs(axis.x()) > 0.9f) rotation.rotate(qRadiansToDegrees(stepAngle * i), 1, 0, 0);
        else if (std::abs(axis.y()) > 0.9f) rotation.rotate(qRadiansToDegrees(stepAngle * i), 0, 1, 0);
        else rotation.rotate(qRadiansToDegrees(stepAngle * i), 0, 0, 1);

        int baseVertex = result.vertices.size();
        for (const auto& v : mesh.vertices) {
            Vertex rv = v;
            rv.position = rotation.map(rv.position);
            rv.normal = rotation.map(rv.normal);
            result.vertices.append(rv);
        }

        for (const auto& face : mesh.faces) {
            Face rf;
            for (int idx : face.indices) rf.indices.append(idx + baseVertex);
            result.faces.append(rf);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

// Mesh editing
MeshData MeshOperations::knifeCut(const MeshData& mesh, const QVector3D& cutStart, const QVector3D& cutEnd)
{
    MeshData result = mesh;
    QVector3D cutDir = (cutEnd - cutStart).normalized();
    QVector3D cutNormal = QVector3D::crossProduct(cutDir, QVector3D(0, 1, 0)).normalized();

    for (int fi = 0; fi < result.faces.size(); ++fi) {
        auto& face = result.faces[fi];
        if (face.indices.size() < 3) continue;

        // Check if face intersects the cut plane
        QVector3D faceCenter;
        for (int idx : face.indices) faceCenter += result.vertices[idx].position;
        faceCenter /= face.indices.size();

        float dist = QVector3D::dotProduct(faceCenter - cutStart, cutNormal);
        if (std::abs(dist) < 0.01f) {
            // Face is on the cut plane - split it
            QVector<int> leftVerts, rightVerts;
            for (int idx : face.indices) {
                float d = QVector3D::dotProduct(result.vertices[idx].position - cutStart, cutNormal);
                if (d < 0) leftVerts.append(idx);
                else rightVerts.append(idx);
            }

            if (leftVerts.size() >= 3 && rightVerts.size() >= 3) {
                Face leftFace, rightFace;
                leftFace.indices = leftVerts;
                rightFace.indices = rightVerts;
                result.faces[fi] = leftFace;
                result.faces.append(rightFace);
            }
        }
    }

    result.computeNormals();
    return result;
}

MeshData MeshOperations::shrinkwrap(const MeshData& mesh, const MeshData& target, const QVector3D& direction)
{
    MeshData result = mesh;

    for (auto& v : result.vertices) {
        // Project vertex onto target mesh along direction
        QVector3D rayOrigin = v.position;
        QVector3D rayDir = direction.normalized();

        float closestT = std::numeric_limits<float>::max();
        for (const auto& face : target.faces) {
            if (face.indices.size() < 3) continue;
            QVector3D v0 = target.vertices[face.indices[0]].position;
            QVector3D v1 = target.vertices[face.indices[1]].position;
            QVector3D v2 = target.vertices[face.indices[2]].position;

            QVector3D edge1 = v1 - v0;
            QVector3D edge2 = v2 - v0;
            QVector3D h = QVector3D::crossProduct(rayDir, edge2);
            float a = QVector3D::dotProduct(edge1, h);
            if (std::abs(a) < 1e-6f) continue;
            float f = 1.0f / a;
            QVector3D s = rayOrigin - v0;
            float u = f * QVector3D::dotProduct(s, h);
            if (u < 0.0f || u > 1.0f) continue;
            QVector3D q = QVector3D::crossProduct(s, edge1);
            float v2d = f * QVector3D::dotProduct(rayDir, q);
            if (v2d < 0.0f || u + v2d > 1.0f) continue;
            float t = f * QVector3D::dotProduct(edge2, q);
            if (t > 0 && t < closestT) closestT = t;
        }

        if (closestT < std::numeric_limits<float>::max()) {
            v.position = rayOrigin + rayDir * closestT;
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::displace(const MeshData& mesh, const QImage& heightmap, float strength)
{
    MeshData result = mesh;
    int w = heightmap.width();
    int h = heightmap.height();

    for (auto& v : result.vertices) {
        // Map UV to heightmap
        float u = v.uv.x();
        float vCoord = v.uv.y();
        int px = qBound(0, static_cast<int>(u * (w - 1)), w - 1);
        int py = qBound(0, static_cast<int>((1.0f - vCoord) * (h - 1)), h - 1);

        QColor pixel = heightmap.pixelColor(px, py);
        float height = pixel.lightnessF();
        v.position += v.normal * height * strength;
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

void MeshOperations::mergeMeshes(MeshData& target, const MeshData& source) {
    int vertexOffset = target.vertices.size();
    target.vertices.append(source.vertices);
    for (const auto& face : source.faces) {
        Face newFace = face;
        for (int& idx : newFace.indices) {
            idx += target.vertices.size() - source.vertices.size();
        }
        target.faces.append(newFace);
    }
}

void MeshOperations::splitMeshes(const MeshData& mesh, QVector<MeshData>& result) {
    result.clear();
    if (mesh.faces.isEmpty() || mesh.materials.isEmpty()) {
        result.append(mesh);
        return;
    }

    // Split by material
    QMap<int, QVector<int>> materialFaces;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        materialFaces[mesh.faces[fi].materialId].append(fi);
    }

    for (auto it = materialFaces.begin(); it != materialFaces.end(); ++it) {
        MeshData part;
        part.name = mesh.name + "_part" + QString::number(it.key());
        part.materialName = it.key() < mesh.materials.size() ? mesh.materials[it.key()] : QString();
        part.metallic = mesh.metallic;
        part.roughness = mesh.roughness;

        QMap<int, int> vertexMap;
        for (int fi : it.value()) {
            const Face& srcFace = mesh.faces[fi];
            Face newFace;
            newFace.materialId = 0;
            newFace.normal = srcFace.normal;
            for (int vi : srcFace.indices) {
                if (!vertexMap.contains(vi)) {
                    int newIdx = part.vertices.size();
                    part.vertices.append(mesh.vertices[vi]);
                    vertexMap[vi] = newIdx;
                }
                newFace.indices.append(vertexMap[vi]);
            }
            part.faces.append(newFace);
        }
        result.append(part);
    }
}

QVector<MeshUVIsland> MeshOperations::findUVIslands(const MeshData& mesh) {
    QVector<MeshUVIsland> islands;
    if (mesh.faces.isEmpty()) return islands;

    ensureEdgeList(const_cast<MeshData&>(mesh));

    // Build adjacency from edges - faces that share an edge are connected
    QMap<int, QSet<int>> faceAdjacency;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const Face& face = mesh.faces[fi];
        for (int i = 0; i < face.indices.size(); ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % face.indices.size()];
            QPair<int, int> key = qMakePair(qMin(a, b), qMax(a, b));

            // Find all faces sharing this edge
            for (int fj = 0; fj < mesh.faces.size(); ++fj) {
                if (fi == fj) continue;
                const Face& other = mesh.faces[fj];
                for (int j = 0; j < other.indices.size(); ++j) {
                    int oa = other.indices[j];
                    int ob = other.indices[(j + 1) % other.indices.size()];
                    if ((oa == a && ob == b) || (oa == b && ob == a)) {
                        faceAdjacency[fi].insert(fj);
                        faceAdjacency[fj].insert(fi);
                    }
                }
            }
        }
    }

    // Flood fill to find connected components
    QSet<int> visited;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        if (visited.contains(fi)) continue;

        MeshUVIsland island;
        QVector<int> stack;
        stack.append(fi);
        QVector2D minUV(1e10f, 1e10f), maxUV(-1e10f, -1e10f);

        while (!stack.isEmpty()) {
            int current = stack.takeLast();
            if (visited.contains(current)) continue;
            visited.insert(current);
            island.faceIndices.append(current);

            const Face& face = mesh.faces[current];
            for (int vi : face.indices) {
                if (vi < mesh.vertices.size() && vi < mesh.uvs.size()) {
                    const QVector2D& uv = mesh.uvs[vi];
                    minUV.setX(qMin(minUV.x(), uv.x()));
                    minUV.setY(qMin(minUV.y(), uv.y()));
                    maxUV.setX(qMax(maxUV.x(), uv.x()));
                    maxUV.setY(qMax(maxUV.y(), uv.y()));
                }
            }

            for (int neighbor : faceAdjacency[current]) {
                if (!visited.contains(neighbor)) {
                    stack.append(neighbor);
                }
            }
        }

        island.minUV = minUV;
        island.maxUV = maxUV;
        islands.append(island);
    }

    return islands;
}

geometry::GeoMeshData MeshOperations::toGeoMesh(const MeshData& mesh) {
    geometry::GeoMeshData geo;
    geo.vertices.resize(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const auto& pos = mesh.vertices[i].position;
        geo.vertices[i] = geometry::GeoVertex(pos.x(), pos.y(), pos.z());
    }
    
    for (const auto& face : mesh.faces) {
        if (face.indices.size() >= 3) {
            geo.faces.emplace_back(face.indices[0], face.indices[1], face.indices[2]);
        }
    }
    return geo;
}

MeshData MeshOperations::fromGeoMesh(const geometry::GeoMeshData& geo) {
    MeshData mesh;
    mesh.vertices.resize(geo.vertices.size());
    for (size_t i = 0; i < geo.vertices.size(); ++i) {
        mesh.vertices[i].position = QVector3D((float)geo.vertices[i].x, (float)geo.vertices[i].y, (float)geo.vertices[i].z);
    }
    for (const auto& face : geo.faces) {
        mesh.faces.append(Face({(int)face.v0, (int)face.v1, (int)face.v2}));
    }
    return mesh;
}

} // namespace ks