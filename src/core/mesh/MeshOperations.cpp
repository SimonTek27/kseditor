#include "MeshOperations.h"
#include <QQueue>
#include <QSet>
#include <QQuaternion>
#include <QImage>
#include <QColor>
#include <cmath>
#include <limits>

#include "../../modules/modellingEditor/BooleanOps.h"

namespace ks {

void MeshData::clear() {
    vertices.clear();
    faces.clear();
    edges.clear();
    normals.clear();
    uvs.clear();
    boundingBoxMin = QVector3D(0, 0, 0);
    boundingBoxMax = QVector3D(0, 0, 0);
    boundingRadius = 0.0f;
    shapeKeyNames.clear();
    shapeKeyDeltas.clear();
    shapeKeyWeights.clear();
    shapeKeyMute.clear();
    shapeKeyMin.clear();
    shapeKeyMax.clear();
}

void MeshData::computeBoundingBox() {
    if (vertices.isEmpty()) {
        boundingBoxMin = QVector3D(0, 0, 0);
        boundingBoxMax = QVector3D(0, 0, 0);
        boundingRadius = 0.0f;
        return;
    }

    QVector3D min = vertices[0].position;
    QVector3D max = vertices[0].position;

    for (const auto& v : vertices) {
        min = QVector3D(qMin(min.x(), v.position.x()), qMin(min.y(), v.position.y()), qMin(min.z(), v.position.z()));
        max = QVector3D(qMax(max.x(), v.position.x()), qMax(max.y(), v.position.y()), qMax(max.z(), v.position.z()));
    }

    boundingBoxMin = min;
    boundingBoxMax = max;
    boundingRadius = (max - min).length() / 2.0f;
}

void MeshData::computeNormals() {
    normals.resize(vertices.size());
    normals.fill(QVector3D(0, 0, 0));

    for (const auto& face : faces) {
        if (face.indices.size() < 3) continue;

        QVector3D v0 = vertices[face[0]].position;
        QVector3D v1 = vertices[face[1]].position;
        QVector3D v2 = vertices[face[2]].position;

        QVector3D normal = QVector3D::crossProduct(v1 - v0, v2 - v0);
        if (normal.length() > 0.0001f) {
            normal.normalize();
        }

        for (int idx : face.indices) {
            normals[idx] += normal;
        }
    }

    for (auto& n : normals) {
        if (n.length() > 0.0001f) n.normalize();
    }
}

void MeshData::computeTangents() {
    for (auto& v : vertices) {
        v.tangent = QVector3D(1, 0, 0);
    }

    if (uvs.isEmpty() || faces.isEmpty()) return;

    for (const auto& face : faces) {
        if (face.indices.size() < 3) continue;

        const Vertex& v0 = vertices[face[0]];
        const Vertex& v1 = vertices[face[1]];
        const Vertex& v2 = vertices[face[2]];

        QVector3D edge1 = v1.position - v0.position;
        QVector3D edge2 = v2.position - v0.position;

        if (face.indices[0] < uvs.size() && face.indices[1] < uvs.size() && face.indices[2] < uvs.size()) {
            QVector2D uv0 = uvs[face.indices[0]];
            QVector2D uv1 = uvs[face.indices[1]];
            QVector2D uv2 = uvs[face.indices[2]];

            float deltaU1 = uv1.x() - uv0.x();
            float deltaV1 = uv1.y() - uv0.y();
            float deltaU2 = uv2.x() - uv0.x();
            float deltaV2 = uv2.y() - uv0.y();

            float den = deltaU1 * deltaV2 - deltaU2 * deltaV1;
            if (qAbs(den) > 0.0001f) {
                float r = 1.0f / den;
                QVector3D tangent = (edge1 * deltaV2 - edge2 * deltaV1) * r;
                tangent.normalize();

                for (int idx : face.indices) {
                    vertices[idx].tangent = tangent;
                }
            }
        }
    }
}

void MeshData::flipFaces() {
    for (auto& face : faces) {
        if (face.indices.size() >= 3) {
            int tmp = face.indices[0];
            face.indices[0] = face.indices[1];
            face.indices[1] = tmp;
        }
    }
    computeNormals();
}

void MeshData::triangulate() {
    QVector<Face> newFaces;
    for (const auto& face : faces) {
        if (face.indices.size() == 3) {
            newFaces.append(face);
        } else if (face.indices.size() > 3) {
            int v0 = face.indices[0];
            for (int i = 1; i < face.indices.size() - 1; ++i) {
                Face tri;
                tri.indices = {v0, face.indices[i], face.indices[i + 1]};
                tri.materialId = face.materialId;
                newFaces.append(tri);
            }
        }
    }
    faces = newFaces;
}

static Vertex makeVert(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
    Vertex vert;
    vert.position = QVector3D(x, y, z);
    vert.normal = QVector3D(nx, ny, nz);
    vert.uv = QVector2D(u, v);
    vert.color = QVector4D(1, 1, 1, 1);
    return vert;
}

MeshData MeshOperations::createBox(float width, float height, float depth) {
    MeshData mesh;
    float hw = width / 2.0f, hh = height / 2.0f, hd = depth / 2.0f;

    mesh.vertices = QVector<Vertex>()
        << makeVert(-hw, -hh, -hd,  0, 0, -1,  0, 0)
        << makeVert( hw, -hh, -hd,  0, 0, -1,  1, 0)
        << makeVert( hw,  hh, -hd,  0, 0, -1,  1, 1)
        << makeVert(-hw,  hh, -hd,  0, 0, -1,  0, 1)
        << makeVert(-hw, -hh,  hd,  0, 0,  1,  0, 0)
        << makeVert( hw, -hh,  hd,  0, 0,  1,  1, 0)
        << makeVert( hw,  hh,  hd,  0, 0,  1,  1, 1)
        << makeVert(-hw,  hh,  hd,  0, 0,  1,  0, 1)
        << makeVert(-hw, -hh, -hd, -1, 0,  0,  0, 0)
        << makeVert(-hw,  hh, -hd, -1, 0,  0,  1, 0)
        << makeVert(-hw,  hh,  hd, -1, 0,  0,  1, 1)
        << makeVert(-hw, -hh,  hd, -1, 0,  0,  0, 1)
        << makeVert( hw, -hh, -hd,  1, 0,  0,  0, 0)
        << makeVert( hw, -hh,  hd,  1, 0,  0,  1, 0)
        << makeVert( hw,  hh,  hd,  1, 0,  0,  1, 1)
        << makeVert( hw,  hh, -hd,  1, 0,  0,  0, 1)
        << makeVert(-hw,  hh, -hd,  0, 1,  0,  0, 0)
        << makeVert( hw,  hh, -hd,  0, 1,  0,  1, 0)
        << makeVert( hw,  hh,  hd,  0, 1,  0,  1, 1)
        << makeVert(-hw,  hh,  hd,  0, 1,  0,  0, 1)
        << makeVert(-hw, -hh,  hd,  0, -1, 0,  0, 0)
        << makeVert( hw, -hh,  hd,  0, -1, 0,  1, 0)
        << makeVert( hw, -hh, -hd,  0, -1, 0,  1, 1)
        << makeVert(-hw, -hh, -hd,  0, -1, 0,  0, 1);

    mesh.faces = QVector<Face>()
        << Face(QVector<int>() << 0 << 1 << 2 << 3)
        << Face(QVector<int>() << 4 << 5 << 6 << 7)
        << Face(QVector<int>() << 8 << 9 << 10 << 11)
        << Face(QVector<int>() << 12 << 13 << 14 << 15)
        << Face(QVector<int>() << 16 << 17 << 18 << 19)
        << Face(QVector<int>() << 20 << 21 << 22 << 23);

    mesh.uvs.resize(24);
    for (int i = 0; i < 4; ++i) {
        mesh.uvs[i] = QVector2D(float(i % 2), float(i / 2));
    }

    mesh.triangulate();
    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createSphere(float radius, int segments, int rings) {
    MeshData mesh;

    for (int r = 0; r <= rings; ++r) {
        float phi = M_PI * r / rings;
        float y = radius * qCos(phi);
        float ringRadius = radius * qSin(phi);

        for (int s = 0; s <= segments; ++s) {
            float theta = 2.0f * M_PI * s / segments;
            float x = ringRadius * qCos(theta);
            float z = ringRadius * qSin(theta);

            Vertex v;
            v.position = QVector3D(x, y, z);
            v.normal = v.position.normalized();
            v.uv = QVector2D(float(s) / segments, float(r) / rings);
            mesh.vertices.append(v);
        }
    }

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            int current = r * (segments + 1) + s;
            int next = current + segments + 1;
            mesh.faces.append({current, next, current + 1});
            mesh.faces.append({current + 1, next, next + 1});
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createCylinder(float radius, float height, int segments) {
    MeshData mesh;
    float hh = height / 2.0f;

    Vertex centerTop, centerBottom;
    centerTop.position = {0, hh, 0}; centerTop.normal = {0, 1, 0};
    centerBottom.position = {0, -hh, 0}; centerBottom.normal = {0, -1, 0};

    int topCenterIdx = mesh.vertices.size(); mesh.vertices.append(centerTop);
    int bottomCenterIdx = mesh.vertices.size(); mesh.vertices.append(centerBottom);

    QVector<QVector3D> topVerts, bottomVerts;

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * qCos(angle);
        float z = radius * qSin(angle);

        Vertex top, bottom;
        top.position = {x, hh, z}; top.normal = {x / radius, 0, z / radius};
        bottom.position = {x, -hh, z}; bottom.normal = {x / radius, 0, z / radius};
        top.uv = {float(i) / segments, 1}; bottom.uv = {float(i) / segments, 0};

        topVerts.append(top.position);
        bottomVerts.append(bottom.position);

        mesh.vertices.append(top);
        mesh.vertices.append(bottom);
    }

    int bottomCenterIdxTop = topCenterIdx + 1;
    for (int i = 0; i < segments; ++i) {
        mesh.faces.append({topCenterIdx, bottomCenterIdxTop + i * 2, bottomCenterIdxTop + i * 2 + 2});
        mesh.faces.append({bottomCenterIdx, bottomCenterIdx + 1 + i * 2 + 2, bottomCenterIdx + 1 + i * 2});
        mesh.faces.append({topCenterIdx + 1 + i * 2, topCenterIdx + 1 + i * 2 + 2, bottomCenterIdx + 1 + i * 2 + 2});
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createCone(float radius, float height, int segments) {
    MeshData mesh;
    float hh = height / 2.0f;

    Vertex centerTop, centerBottom;
    centerTop.position = {0, hh, 0}; centerTop.normal = {0, 1, 0};
    centerBottom.position = {0, -hh, 0}; centerBottom.normal = {0, -1, 0};

    int topIdx = mesh.vertices.size(); mesh.vertices.append(centerTop);
    int bottomIdx = mesh.vertices.size(); mesh.vertices.append(centerBottom);

    QVector3D tipNormal(0, 1, 0);

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * qCos(angle);
        float z = radius * qSin(angle);
        QVector3D sideNormal = QVector3D(x, radius / height * 2, z).normalized();

        Vertex bottom, tip;
        bottom.position = {x, -hh, z}; bottom.normal = sideNormal;
        tip.position = {0, hh, 0}; tip.normal = sideNormal;
        bottom.uv = {float(i) / segments, 0}; tip.uv = {float(i) / segments, 1};

        mesh.vertices.append(bottom);
        mesh.vertices.append(tip);

        mesh.faces.append({bottomIdx, bottomIdx + 1 + i * 2, bottomIdx + 1 + i * 2 + 2});
        mesh.faces.append({bottomIdx + 1 + i * 2, topIdx, bottomIdx + 1 + i * 2 + 2});
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createPlane(float width, float height, int uSubdiv, int vSubdiv) {
    MeshData mesh;
    float hw = width / 2.0f, hh = height / 2.0f;

    for (int v = 0; v <= vSubdiv; ++v) {
        for (int u = 0; u <= uSubdiv; ++u) {
            float x = -hw + width * float(u) / uSubdiv;
            float y = -hh + height * float(v) / vSubdiv;
            Vertex vert;
            vert.position = {x, y, 0};
            vert.normal = {0, 0, 1};
            vert.uv = {float(u) / uSubdiv, float(v) / vSubdiv};
            mesh.vertices.append(vert);
        }
    }

    for (int v = 0; v < vSubdiv; ++v) {
        for (int u = 0; u < uSubdiv; ++u) {
            int idx = v * (uSubdiv + 1) + u;
            mesh.faces.append({idx, idx + 1, idx + uSubdiv + 2});
            mesh.faces.append({idx + 1, idx + uSubdiv + 2, idx + uSubdiv + 1});
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createTorus(float majorRadius, float minorRadius, int majorSeg, int minorSeg) {
    MeshData mesh;

    for (int m = 0; m <= majorSeg; ++m) {
        float u = 2.0f * M_PI * m / majorSeg;
        float cu = qCos(u), su = qSin(u);

        for (int n = 0; n <= minorSeg; ++n) {
            float v = 2.0f * M_PI * n / minorSeg;
            float cv = qCos(v), sv = qSin(v);

            Vertex vert;
            vert.position = QVector3D(
                (majorRadius + minorRadius * cv) * cu,
                (majorRadius + minorRadius * cv) * su,
                minorRadius * sv
            );
            vert.normal = QVector3D(cv * cu, cv * su, sv).normalized();
            vert.uv = QVector2D(float(m) / majorSeg, float(n) / minorSeg);
            mesh.vertices.append(vert);
        }
    }

    for (int m = 0; m < majorSeg; ++m) {
        for (int n = 0; n < minorSeg; ++n) {
            int i = m * (minorSeg + 1) + n;
            mesh.faces.append({i, i + minorSeg + 1, i + 1});
            mesh.faces.append({i + 1, i + minorSeg + 1, i + minorSeg + 2});
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::createGrid(float width, float height, int uSubdiv, int vSubdiv) {
    return createPlane(width, height, uSubdiv, vSubdiv);
}

MeshData MeshOperations::createIcosphere(float radius, int subdivisions) {
    MeshData mesh;

    const float phi = (1.0f + qSqrt(5.0f)) / 2.0f;
    QVector<QVector3D> baseVerts;
    baseVerts.reserve(12);
    baseVerts << QVector3D(-1, phi, 0).normalized()
              << QVector3D(1, phi, 0).normalized()
              << QVector3D(-1, -phi, 0).normalized()
              << QVector3D(1, -phi, 0).normalized()
              << QVector3D(0, -1, phi).normalized()
              << QVector3D(0, 1, phi).normalized()
              << QVector3D(0, -1, -phi).normalized()
              << QVector3D(0, 1, -phi).normalized()
              << QVector3D(phi, 0, -1).normalized()
              << QVector3D(phi, 0, 1).normalized()
              << QVector3D(-phi, 0, -1).normalized()
              << QVector3D(-phi, 0, 1).normalized();

    QVector<QVector<int>> baseFaces;
    baseFaces.reserve(20);
    baseFaces << (QVector<int>() << 0 << 11 << 5)
              << (QVector<int>() << 0 << 5 << 1)
              << (QVector<int>() << 0 << 1 << 7)
              << (QVector<int>() << 0 << 7 << 10)
              << (QVector<int>() << 0 << 10 << 11)
              << (QVector<int>() << 1 << 5 << 9)
              << (QVector<int>() << 5 << 11 << 4)
              << (QVector<int>() << 11 << 10 << 2)
              << (QVector<int>() << 10 << 7 << 6)
              << (QVector<int>() << 7 << 1 << 8)
              << (QVector<int>() << 3 << 9 << 4)
              << (QVector<int>() << 3 << 4 << 2)
              << (QVector<int>() << 3 << 2 << 6)
              << (QVector<int>() << 3 << 6 << 8)
              << (QVector<int>() << 3 << 8 << 9)
              << (QVector<int>() << 4 << 9 << 5)
              << (QVector<int>() << 2 << 4 << 11)
              << (QVector<int>() << 6 << 2 << 10)
              << (QVector<int>() << 8 << 6 << 7)
              << (QVector<int>() << 9 << 8 << 1);

    for (const auto& v : baseVerts) {
        Vertex vert;
        vert.position = v * radius;
        vert.normal = v.normalized();
        mesh.vertices.append(vert);
    }

    QVector<QPair<int, int>> edges;
    QVector<QVector<int>> faces = baseFaces;

    for (int sub = 0; sub < subdivisions; ++sub) {
        QVector<QVector<int>> newFaces;
        QVector<QPair<int, int>> newEdges;

        for (const auto& face : faces) {
            QVector<QVector3D> pts = {
                mesh.vertices[face[0]].position,
                mesh.vertices[face[1]].position,
                mesh.vertices[face[2]].position
            };

            auto getMidpoint = [&](int i1, int i2) -> int {
                QPair<int, int> edge = (i1 < i2) ? QPair<int, int>(i1, i2) : QPair<int, int>(i2, i1);
                if (edges.contains(edge)) {
                    return edges.indexOf(edge) + baseVerts.size();
                }

                QVector3D mid = (pts[i1 < face[0] ? 0 : (i1 - baseVerts.size()) / 2] +
                               pts[i2 < baseVerts.size() ? 0 : (i2 - baseVerts.size()) / 2]).normalized() * radius;

                Vertex v;
                v.position = mid;
                v.normal = mid.normalized();
                mesh.vertices.append(v);

                edges.append(edge);
                return mesh.vertices.size() - 1;
            };

            int a = getMidpoint(face[0], face[1]);
            int b = getMidpoint(face[1], face[2]);
            int c = getMidpoint(face[2], face[0]);

            newFaces.append({face[0], a, c});
            newFaces.append({face[1], b, a});
            newFaces.append({face[2], c, b});
            newFaces.append({a, b, c});
        }

        faces = newFaces;
    }

    for (const auto& face : faces) {
        if (face.size() >= 3) {
            mesh.faces.append({face[0], face[1], face[2]});
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::extrude(const MeshData& input, const QVector3D& direction, float distance, bool individualFaces) {
    MeshData mesh = input;

    if (individualFaces) {
        int faceCount = mesh.faces.size();
        for (int i = 0; i < faceCount; ++i) {
            const auto& face = mesh.faces[i];
            QVector3D normal = QVector3D::crossProduct(
                mesh.vertices[face[1]].position - mesh.vertices[face[0]].position,
                mesh.vertices[face[2]].position - mesh.vertices[face[0]].position
            ).normalized();

            QVector3D offset = direction * distance;

            int baseIdx = mesh.vertices.size();
            for (int j = 0; j < face.indices.size(); ++j) {
                Vertex v = mesh.vertices[face[j]];
                v.position += offset;
                mesh.vertices.append(v);
            }

            Face extrudedFace;
            for (int j = 0; j < face.indices.size(); ++j) {
                extrudedFace.indices.append(baseIdx + j);
            }
            mesh.faces.append(extrudedFace);
        }
    } else {
        int baseIdx = mesh.vertices.size();
        for (const auto& v : mesh.vertices) {
            Vertex nv = v;
            nv.position += direction * distance;
            mesh.vertices.append(nv);
        }

        QMap<QPair<int, int>, QVector<int>> edgeMap;
        for (int i = 0; i < mesh.faces.size(); ++i) {
            const auto& face = mesh.faces[i];
            for (int j = 0; j < face.indices.size(); ++j) {
                int v1 = face.indices[j];
                int v2 = face.indices[(j + 1) % face.indices.size()];
                QPair<int, int> edge = (v1 < v2) ? QPair<int, int>(v1, v2) : QPair<int, int>(v2, v1);
                edgeMap[edge].append(i);
            }
        }

        for (auto it = edgeMap.constBegin(); it != edgeMap.constEnd(); ++it) {
            if (it.value().size() == 1) {
                int v1 = it.key().first;
                int v2 = it.key().second;
                mesh.faces.append({v1, v2, v2 + baseIdx, v1 + baseIdx});
            }
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::extrudeFaces(const MeshData& input, const QVector<QVector3D>& directions) {
    MeshData mesh = input;
    int faceCount = mesh.faces.size();
    int dirCount = directions.size();

    for (int i = 0; i < faceCount; ++i) {
        const auto& face = mesh.faces[i];
        QVector3D dir = (i < dirCount) ? directions[i] : QVector3D(0, 0, 0);
        if (dir.lengthSquared() < 0.0001f) continue;

        int baseIdx = mesh.vertices.size();
        for (int j = 0; j < face.indices.size(); ++j) {
            Vertex v = mesh.vertices[face[j]];
            v.position += dir;
            mesh.vertices.append(v);
        }

        Face extrudedFace;
        for (int j = 0; j < face.indices.size(); ++j) {
            extrudedFace.indices.append(baseIdx + j);
        }
        mesh.faces.append(extrudedFace);
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

// ── Boolean Operations (CGAL-backed if available) ──

// ── Conversion helpers between ks::MeshData and ks::geometry::GeoMeshData ──

geometry::GeoMeshData MeshOperations::toGeoMesh(const MeshData& src) {
    geometry::GeoMeshData out;
    out.vertices.reserve(src.vertices.size());
    for (const auto& v : src.vertices)
        out.vertices.emplace_back(v.position.x(), v.position.y(), v.position.z());
    out.faces.reserve(src.faces.size());
    for (const auto& f : src.faces) {
        if (f.indices.size() >= 3)
            out.faces.emplace_back(f.indices[0], f.indices[1], f.indices[2]);
    }
    return out;
}

MeshData MeshOperations::fromGeoMesh(const geometry::GeoMeshData& src) {
    MeshData out;
    out.vertices.reserve(src.vertices.size());
    for (const auto& v : src.vertices) {
        Vertex kv;
        kv.position = QVector3D(v.x, v.y, v.z);
        out.vertices.append(kv);
    }
    out.faces.reserve(src.faces.size());
    for (const auto& f : src.faces) {
        Face kf;
        kf.indices = { (int)f.v0, (int)f.v1, (int)f.v2 };
        out.faces.append(kf);
    }
    out.computeNormals();
    out.computeBoundingBox();
    return out;
}

// ── Ray-triangle helpers for boolean ops (fallback) ──
namespace {
struct Tri {
    QVector3D v0, v1, v2, normal;
    float area;
};
static QVector<Tri> meshToTris(const MeshData& m) {
    QVector<Tri> out;
    for (const auto& f : m.faces) {
        QVector<int> idx = f.indices;
        if (idx.size() < 3) continue;
        for (int i = 1; i + 1 < idx.size(); ++i) {
            Tri t;
            t.v0 = m.vertices[idx[0]].position;
            t.v1 = m.vertices[idx[i]].position;
            t.v2 = m.vertices[idx[i + 1]].position;
            t.normal = QVector3D::crossProduct(t.v1 - t.v0, t.v2 - t.v0);
            t.area = t.normal.length();
            if (t.area > 1e-12f) t.normal /= t.area; else t.normal = QVector3D(0,1,0);
            out.append(t);
        }
    }
    return out;
}
static bool rayTriIntersect(const QVector3D& orig, const QVector3D& dir,
                            const QVector3D& v0, const QVector3D& v1, const QVector3D& v2,
                            float& t) {
    const float EPS = 1e-8f;
    QVector3D e1 = v1 - v0, e2 = v2 - v0;
    QVector3D pv = QVector3D::crossProduct(dir, e2);
    float det = QVector3D::dotProduct(e1, pv);
    if (qAbs(det) < EPS) return false;
    float invDet = 1.0f / det;
    QVector3D tv = orig - v0;
    float u = QVector3D::dotProduct(tv, pv) * invDet;
    if (u < 0 || u > 1) return false;
    QVector3D qv = QVector3D::crossProduct(tv, e1);
    float v = QVector3D::dotProduct(dir, qv) * invDet;
    if (v < 0 || u + v > 1) return false;
    t = QVector3D::dotProduct(e2, qv) * invDet;
    return t > EPS;
}
static bool pointInMesh(const QVector3D& p, const QVector<Tri>& tris) {
    QVector3D dir(0, 0, 1);
    int hits = 0;
    for (const auto& t : tris) {
        float dummy;
        if (rayTriIntersect(p, dir, t.v0, t.v1, t.v2, dummy)) hits++;
    }
    return (hits % 2) == 1;
}
} // anonymous namespace

MeshData MeshOperations::booleanUnion(const MeshData& a, const MeshData& b) {
#if HAS_CGAL
    if (geometry::BooleanOperations::canPerform()) {
        auto geomA = toGeoMesh(a);
        auto geomB = toGeoMesh(b);
        auto result = geometry::BooleanOperations::performOperation(geomA, geomB, geometry::BooleanOperations::Union);
        if (result.isSuccess()) return fromGeoMesh(result.result);
    }
#endif
    QVector<Tri> trisA = meshToTris(a), trisB = meshToTris(b);
    if (trisA.isEmpty() || trisB.isEmpty()) { MeshData r = a; mergeMeshes(r, b); return r; }
    uint32_t offA = (uint32_t)a.vertices.size();
    MeshData r;
    for (const auto& v : a.vertices) r.vertices.append(v);
    for (const auto& v : b.vertices) r.vertices.append(v);
    auto centroid = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return (v0 + v1 + v2) / 3.0f;
    };
    for (int fi = 0; fi < a.faces.size(); ++fi) {
        const auto& face = a.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(a.vertices[face[0]].position, a.vertices[face[1]].position, a.vertices[face[2]].position);
        if (!pointInMesh(c, trisB)) {
            Face nf = face;
            for (int& idx : nf.indices) { /* keep offset 0 */ }
            r.faces.append(nf);
        }
    }
    for (int fi = 0; fi < b.faces.size(); ++fi) {
        const auto& face = b.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(b.vertices[face[0]].position, b.vertices[face[1]].position, b.vertices[face[2]].position);
        if (!pointInMesh(c, trisA)) {
            Face nf = face;
            for (int& idx : nf.indices) idx += offA;
            r.faces.append(nf);
        }
    }
    r.computeNormals(); r.computeBoundingBox();
    return r;
}

MeshData MeshOperations::booleanDifference(const MeshData& a, const MeshData& b) {
#if HAS_CGAL
    if (geometry::BooleanOperations::canPerform()) {
        auto geomA = toGeoMesh(a);
        auto geomB = toGeoMesh(b);
        auto result = geometry::BooleanOperations::performOperation(geomA, geomB, geometry::BooleanOperations::Difference);
        if (result.isSuccess()) return fromGeoMesh(result.result);
    }
#endif
    QVector<Tri> trisA = meshToTris(a), trisB = meshToTris(b);
    if (trisA.isEmpty()) return a;
    if (trisB.isEmpty()) { MeshData r = a; return r; }
    uint32_t offA = (uint32_t)a.vertices.size();
    MeshData r;
    for (const auto& v : a.vertices) r.vertices.append(v);
    for (const auto& v : b.vertices) r.vertices.append(v);
    auto centroid = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return (v0 + v1 + v2) / 3.0f;
    };
    for (int fi = 0; fi < a.faces.size(); ++fi) {
        const auto& face = a.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(a.vertices[face[0]].position, a.vertices[face[1]].position, a.vertices[face[2]].position);
        if (!pointInMesh(c, trisB)) {
            Face nf = face;
            r.faces.append(nf);
        }
    }
    for (int fi = 0; fi < b.faces.size(); ++fi) {
        const auto& face = b.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(b.vertices[face[0]].position, b.vertices[face[1]].position, b.vertices[face[2]].position);
        if (pointInMesh(c, trisA)) {
            Face nf = face;
            for (int& idx : nf.indices) idx += offA;
            qSwap(nf.indices[0], nf.indices[1]); // flip for outward normals
            r.faces.append(nf);
        }
    }
    r.computeNormals(); r.computeBoundingBox();
    return r;
}

MeshData MeshOperations::booleanIntersection(const MeshData& a, const MeshData& b) {
#if HAS_CGAL
    if (geometry::BooleanOperations::canPerform()) {
        auto geomA = toGeoMesh(a);
        auto geomB = toGeoMesh(b);
        auto result = geometry::BooleanOperations::performOperation(geomA, geomB, geometry::BooleanOperations::Intersection);
        if (result.isSuccess()) return fromGeoMesh(result.result);
    }
#endif
    QVector<Tri> trisA = meshToTris(a), trisB = meshToTris(b);
    if (trisA.isEmpty() || trisB.isEmpty()) return MeshData();
    uint32_t offA = (uint32_t)a.vertices.size();
    MeshData r;
    for (const auto& v : a.vertices) r.vertices.append(v);
    for (const auto& v : b.vertices) r.vertices.append(v);
    auto centroid = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return (v0 + v1 + v2) / 3.0f;
    };
    for (int fi = 0; fi < a.faces.size(); ++fi) {
        const auto& face = a.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(a.vertices[face[0]].position, a.vertices[face[1]].position, a.vertices[face[2]].position);
        if (pointInMesh(c, trisB)) {
            Face nf = face;
            r.faces.append(nf);
        }
    }
    for (int fi = 0; fi < b.faces.size(); ++fi) {
        const auto& face = b.faces[fi];
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(b.vertices[face[0]].position, b.vertices[face[1]].position, b.vertices[face[2]].position);
        if (pointInMesh(c, trisA)) {
            Face nf = face;
            for (int& idx : nf.indices) idx += offA;
            r.faces.append(nf);
        }
    }
    r.computeNormals(); r.computeBoundingBox();
    return r;
}

MeshData MeshOperations::booleanXor(const MeshData& a, const MeshData& b) {
#if HAS_CGAL
    if (geometry::BooleanOperations::canPerform()) {
        auto geomA = toGeoMesh(a);
        auto geomB = toGeoMesh(b);
        auto result = geometry::BooleanOperations::performOperation(geomA, geomB, geometry::BooleanOperations::SymmetricDiff);
        if (result.isSuccess()) return fromGeoMesh(result.result);
    }
#endif
    MeshData u = booleanUnion(a, b);
    MeshData i = booleanIntersection(a, b);
    QVector<Tri> trisI = meshToTris(i);
    if (trisI.isEmpty()) return u;
    MeshData r;
    for (const auto& v : u.vertices) r.vertices.append(v);
    auto centroid = [](const QVector3D& v0, const QVector3D& v1, const QVector3D& v2) {
        return (v0 + v1 + v2) / 3.0f;
    };
    for (const auto& face : u.faces) {
        if (face.indices.size() < 3) continue;
        QVector3D c = centroid(u.vertices[face[0]].position, u.vertices[face[1]].position, u.vertices[face[2]].position);
        if (!pointInMesh(c, trisI)) {
            r.faces.append(face);
        }
    }
    r.computeNormals(); r.computeBoundingBox();
    return r;
}

MeshData MeshOperations::bevelEdges(const MeshData& input, float distance, int segments, float angleLimit) {
    if (distance <= 0.0f || input.faces.isEmpty()) return input;
    MeshData mesh = input;
    ensureEdgeList(mesh);

    QMap<QPair<int,int>, QVector<int>> edgeToFaces;
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const Face& f = mesh.faces[fi];
        int n = f.indices.size();
        for (int i = 0; i < n; ++i) {
            int a = f.indices[i], b = f.indices[(i + 1) % n];
            if (a > b) qSwap(a, b);
            edgeToFaces[qMakePair(a, b)].append(fi);
        }
    }

    QVector<QPair<int,int>> bevelEdges;
    for (auto it = edgeToFaces.constBegin(); it != edgeToFaces.constEnd(); ++it) {
        if (it.value().size() == 2) {
            int fi0 = it.value()[0], fi1 = it.value()[1];
            if (fi0 < mesh.faces.size() && fi1 < mesh.faces.size()) {
                QVector3D n0 = computeFaceNormal(mesh, fi0);
                QVector3D n1 = computeFaceNormal(mesh, fi1);
                float angle = qAcos(qBound(-1.0f, QVector3D::dotProduct(n0, n1), 1.0f));
                if (angle < angleLimit) {
                    bevelEdges.append(it.key());
                }
            }
        }
    }

    for (const auto& edge : bevelEdges) {
        int v1 = edge.first, v2 = edge.second;
        if (v1 >= mesh.vertices.size() || v2 >= mesh.vertices.size()) continue;

        QVector3D midPos = (mesh.vertices[v1].position + mesh.vertices[v2].position) / 2.0f;
        QVector3D edgeDir = (mesh.vertices[v2].position - mesh.vertices[v1].position).normalized();
        QVector3D pushDir = QVector3D(0, 1, 0);

        for (int fi : edgeToFaces.value(edge)) {
            if (fi < mesh.faces.size()) {
                QVector3D n = computeFaceNormal(mesh, fi);
                QVector3D proj = n - QVector3D::dotProduct(n, edgeDir) * edgeDir;
                if (proj.lengthSquared() > 0.0001f) {
                    pushDir = proj.normalized();
                    break;
                }
            }
        }

        int newV1 = mesh.vertices.size();
        Vertex nv1 = mesh.vertices[v1];
        nv1.position = mesh.vertices[v1].position + pushDir * distance;
        mesh.vertices.append(nv1);

        int newV2 = mesh.vertices.size();
        Vertex nv2 = mesh.vertices[v2];
        nv2.position = mesh.vertices[v2].position + pushDir * distance;
        mesh.vertices.append(nv2);

        for (int fi : edgeToFaces.value(edge)) {
            if (fi >= mesh.faces.size()) continue;
            Face& f = mesh.faces[fi];
            for (int i = 0; i < f.indices.size(); ++i) {
                int next = (i + 1) % f.indices.size();
                int a = f.indices[i], b = f.indices[next];
                if ((a == v1 && b == v2) || (a == v2 && b == v1)) {
                    f.indices.insert(next, (a == v1) ? newV2 : newV1);
                    f.indices.insert(i + 1, (a == v1) ? newV1 : newV2);
                    break;
                }
            }
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::bevelVertices(const MeshData& mesh, float distance) {
    if (distance <= 0.0f || mesh.faces.isEmpty()) return mesh;
    MeshData result = mesh;

    QMap<int, QVector<int>> vertToFaces;
    for (int fi = 0; fi < result.faces.size(); ++fi) {
        for (int idx : result.faces[fi].indices) {
            vertToFaces[idx].append(fi);
        }
    }

    for (auto it = vertToFaces.constBegin(); it != vertToFaces.constEnd(); ++it) {
        int vi = it.key();
        if (vi >= result.vertices.size()) continue;
        const QVector<int>& adjFaces = it.value();
        if (adjFaces.size() < 3) continue;

        QVector3D avgNormal(0, 0, 0);
        for (int fi : adjFaces) {
            avgNormal += computeFaceNormal(result, fi);
        }
        avgNormal.normalize();
        if (avgNormal.lengthSquared() < 0.0001f) continue;

        QVector3D oldPos = result.vertices[vi].position;
        QVector3D newPos = oldPos + avgNormal * distance;

        for (int fi : adjFaces) {
            if (fi >= result.faces.size()) continue;
            Face& f = result.faces[fi];
            for (int i = 0; i < f.indices.size(); ++i) {
                if (f.indices[i] == vi) {
                    int newIdx = result.vertices.size();
                    Vertex nv = result.vertices[vi];
                    nv.position = newPos;
                    result.vertices.append(nv);
                    f.indices[i] = newIdx;
                    break;
                }
            }
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::insetFaces(const MeshData& mesh, float distance, float depth) {
    if (mesh.faces.isEmpty() || distance <= 0.0f) return mesh;
    MeshData result;
    for (const auto& v : mesh.vertices) result.vertices.append(v);

    // Build per-face vertex groups for the old vertices
    // For each face: create an inset face (scaled inward along face normal),
    // then bridge edges with side quad
    for (int fi = 0; fi < mesh.faces.size(); ++fi) {
        const Face& face = mesh.faces[fi];
        int n = face.indices.size();
        if (n < 3) continue;

        // Compute face center and normal
        QVector3D center;
        for (int i = 0; i < n; ++i)
            center += mesh.vertices[face[i]].position;
        center /= n;

        QVector3D normal = MeshOperations::computeFaceNormal(mesh, fi);

        // Create inset vertices (move toward center along normal plane)
        // and side vertices (original pushed out by depth along normal)
        QVector<int> insetIdx(n), sideOuter(n), sideInner(n);
        for (int i = 0; i < n; ++i) {
            const Vertex& ov = mesh.vertices[face[i]];
            // Inset: move vertex toward center by distance, projected onto face plane
            QVector3D dir = (center - ov.position);
            float len = dir.length();
            QVector3D insetPos;
            if (len > 0.001f) {
                dir /= len;
                float d = qMin(distance, len * 0.45f);
                insetPos = ov.position + dir * d;
            } else {
                insetPos = ov.position;
            }

            Vertex iv; iv.position = insetPos; iv.normal = normal;
            result.vertices.append(iv); insetIdx[i] = result.vertices.size() - 1;

            // Side vertices (extruded along depth)
            if (depth != 0.0f) {
                Vertex so = ov; so.position += normal * depth;
                result.vertices.append(so); sideOuter[i] = result.vertices.size() - 1;

                Vertex si; si.position = insetPos + normal * depth; si.normal = normal;
                result.vertices.append(si); sideInner[i] = result.vertices.size() - 1;
            }
        }

        // Emit inset face (reversed winding for proper normal)
        Face inFace;
        inFace.materialId = face.materialId;
        for (int i = n - 1; i >= 0; --i)
            inFace.indices.append(insetIdx[i]);
        result.faces.append(inFace);

        // Emit side quads (bridging original edges to inset edges)
        for (int i = 0; i < n; ++i) {
            int i_next = (i + 1) % n;
            if (depth != 0.0f) {
                Face s1; s1.materialId = face.materialId;
                s1.indices = { face[i], face[i_next], sideOuter[i_next], sideOuter[i] };
                result.faces.append(s1);
                Face s2; s2.materialId = face.materialId;
                s2.indices = { sideOuter[i], sideOuter[i_next], sideInner[i_next], sideInner[i] };
                result.faces.append(s2);
                Face s3; s3.materialId = face.materialId;
                s3.indices = { sideInner[i], sideInner[i_next], insetIdx[i_next], insetIdx[i] };
                result.faces.append(s3);
            } else {
                Face s; s.materialId = face.materialId;
                s.indices = { face[i], face[i_next], insetIdx[i_next], insetIdx[i] };
                result.faces.append(s);
            }
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::weldVertices(const MeshData& mesh, float threshold) {
    if (mesh.vertices.isEmpty() || threshold <= 0.0f) return mesh;
    MeshData result;

    int removed = 0;
    QVector<int> remap(mesh.vertices.size());
    for (int i = 0; i < mesh.vertices.size(); ++i) remap[i] = i;

    for (int i = 0; i < mesh.vertices.size(); ++i) {
        if (remap[i] != i) continue;
        for (int j = i + 1; j < mesh.vertices.size(); ++j) {
            if (remap[j] != j) continue;
            QVector3D d = mesh.vertices[i].position - mesh.vertices[j].position;
            if (d.lengthSquared() <= threshold * threshold) {
                remap[j] = i;
                ++removed;
            }
        }
    }

    if (removed == 0) return mesh;

    QVector<int> newIdx(mesh.vertices.size(), -1);
    for (int i = 0; i < mesh.vertices.size(); ++i) {
        if (remap[i] == i) {
            newIdx[i] = result.vertices.size();
            result.vertices.append(mesh.vertices[i]);
        }
    }

    for (const auto& face : mesh.faces) {
        Face nf;
        nf.materialId = face.materialId;
        QSet<int> seen;
        for (int idx : face.indices) {
            int r = newIdx[remap[idx]];
            if (r >= 0 && !seen.contains(r)) {
                seen.insert(r);
                nf.indices.append(r);
            }
        }
        if (nf.indices.size() >= 3)
            result.faces.append(nf);
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::knifeCut(const MeshData& input, const QVector3D& cutStart, const QVector3D& cutEnd) {
    MeshData result = input;
    QVector3D cutDir = (cutEnd - cutStart).normalized();
    float cutLen = (cutEnd - cutStart).length();
    if (cutLen < 0.001f) return result;

    QMap<QPair<int,int>, float> splitEdges;
    for (int fi = 0; fi < result.faces.size(); ++fi) {
        const Face& f = result.faces[fi];
        for (int i = 0; i < f.indices.size(); ++i) {
            int v0 = f.indices[i], v1 = f.indices[(i + 1) % f.indices.size()];
            if (v0 > v1) qSwap(v0, v1);
            if (splitEdges.contains({v0, v1})) continue;

            const QVector3D& p0 = result.vertices[v0].position;
            const QVector3D& p1 = result.vertices[v1].position;
            QVector3D edgeDir = p1 - p0;
            float edgeLen = edgeDir.length();
            if (edgeLen < 0.001f) continue;
            edgeDir /= edgeLen;

            // Closest point between two line segments (cut line vs edge)
            QVector3D w0 = p0 - cutStart;
            float a = QVector3D::dotProduct(cutDir, cutDir);
            float b = QVector3D::dotProduct(cutDir, edgeDir);
            float c = QVector3D::dotProduct(edgeDir, edgeDir);
            float d = QVector3D::dotProduct(cutDir, w0);
            float e = QVector3D::dotProduct(edgeDir, w0);
            float denom = a * c - b * b;
            if (qAbs(denom) < 1e-6f) continue;

            float t_cut = (b * e - c * d) / denom;
            float t_edge = (a * e - b * d) / denom;

            if (t_cut >= 0.0f && t_cut <= 1.0f && t_edge >= 0.0f && t_edge <= 1.0f) {
                QVector3D ptOnCut = cutStart + t_cut * cutDir;
                QVector3D ptOnEdge = p0 + t_edge * edgeDir;
                if ((ptOnCut - ptOnEdge).length() < 0.001f)
                    splitEdges[{v0, v1}] = t_edge;
            }
        }
    }

    if (splitEdges.isEmpty()) return result;

    QMap<int, int> oldToNew;
    for (auto it = splitEdges.begin(); it != splitEdges.end(); ++it) {
        int v0 = it.key().first, v1 = it.key().second;
        float t = it.value();
        Vertex nv;
        nv.position = (1.0f - t) * result.vertices[v0].position + t * result.vertices[v1].position;
        nv.normal = ((1.0f - t) * result.vertices[v0].normal + t * result.vertices[v1].normal).normalized();
        nv.uv = (1.0f - t) * result.vertices[v0].uv + t * result.vertices[v1].uv;
        nv.color = result.vertices[v0].color;
        result.vertices.append(nv);
        int newIdx = result.vertices.size() - 1;
        oldToNew[v0 * 1000000 + v1] = newIdx;
    }

    QVector<Face> newFaces;
    for (const Face& f : result.faces) {
        QVector<int> splitVerts;
        for (int i = 0; i < f.indices.size(); ++i) {
            int v0 = f.indices[i], v1 = f.indices[(i + 1) % f.indices.size()];
            int a = qMin(v0, v1), b = qMax(v0, v1);
            if (oldToNew.contains(a * 1000000 + b))
                splitVerts.append(i);
        }

        if (splitVerts.size() < 2) { newFaces.append(f); continue; }

        for (int si = 0; si + 1 < splitVerts.size(); ++si) {
            int i0 = splitVerts[si], i1 = splitVerts[si + 1];
            int a0 = qMin(f.indices[i0], f.indices[(i0 + 1) % f.indices.size()]);
            int b0 = qMax(f.indices[i0], f.indices[(i0 + 1) % f.indices.size()]);
            int a1 = qMin(f.indices[i1], f.indices[(i1 + 1) % f.indices.size()]);
            int b1 = qMax(f.indices[i1], f.indices[(i1 + 1) % f.indices.size()]);
            int ni0 = oldToNew.value(a0 * 1000000 + b0, -1);
            int ni1 = oldToNew.value(a1 * 1000000 + b1, -1);
            if (ni0 < 0 || ni1 < 0) { newFaces.append(f); continue; }

            Face nf;
            nf.materialId = f.materialId;
            nf.indices = f.indices.mid(i0, i1 - i0 + 1);
            nf.indices.append(ni1);
            nf.indices.append(ni0);
            newFaces.append(nf);
        }
    }

    if (!newFaces.isEmpty()) {
        result.faces = newFaces;
        result.triangulate();
    }
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::dissolveEdges(const MeshData& mesh, const QVector<int>& edgeIndices) {
    MeshData result = mesh;
    ensureEdgeList(result);
    if (edgeIndices.isEmpty()) return result;

    // Build edge-to-face mapping
    QMap<QPair<int,int>, QVector<int>> edgeToFaces;
    for (int fi = 0; fi < result.faces.size(); ++fi) {
        const Face& f = result.faces[fi];
        int n = f.indices.size();
        for (int i = 0; i < n; ++i) {
            int a = f.indices[i], b = f.indices[(i+1)%n];
            if (a > b) qSwap(a, b);
            edgeToFaces[qMakePair(a, b)].append(fi);
        }
    }

    QSet<int> facesToRemove;
    QVector<Face> newFaces;

    for (int ei : edgeIndices) {
        if (ei < 0 || ei >= result.edges.size()) continue;
        int v1 = result.edges[ei].v1;
        int v2 = result.edges[ei].v2;
        if (v1 == v2) continue;
        QPair<int,int> key = (v1 < v2) ? qMakePair(v1, v2) : qMakePair(v2, v1);

        auto it = edgeToFaces.constFind(key);
        if (it == edgeToFaces.constEnd()) continue;
        const QVector<int>& faceList = it.value();

        if (faceList.size() == 2) {
            int fA = faceList[0], fB = faceList[1];
            if (facesToRemove.contains(fA) || facesToRemove.contains(fB)) continue;

            const Face& faceA = result.faces[fA];
            const Face& faceB = result.faces[fB];

            // Find positions of v1, v2 in each face
            int posA1 = -1, posA2 = -1, posB1 = -1, posB2 = -1;
            for (int i = 0; i < faceA.indices.size(); ++i) {
                if (faceA.indices[i] == v1) posA1 = i;
                if (faceA.indices[i] == v2) posA2 = i;
            }
            for (int i = 0; i < faceB.indices.size(); ++i) {
                if (faceB.indices[i] == v1) posB1 = i;
                if (faceB.indices[i] == v2) posB2 = i;
            }
            if (posA1 < 0 || posA2 < 0 || posB1 < 0 || posB2 < 0) continue;

            // Collect vertices from A: from v2 forward to v1 (exclusive)
            QVector<int> merged;
            merged.append(v1);
            // B's vertices from v1 forward to v2 (exclusive)
            for (int i = (posB1 + 1) % faceB.indices.size(); i != posB2; i = (i + 1) % faceB.indices.size()) {
                merged.append(faceB.indices[i]);
            }
            merged.append(v2);
            // A's vertices from v2 forward to v1 (exclusive)
            for (int i = (posA2 + 1) % faceA.indices.size(); i != posA1; i = (i + 1) % faceA.indices.size()) {
                merged.append(faceA.indices[i]);
            }

            if (merged.size() >= 3) {
                Face newFace;
                newFace.indices = merged;
                newFace.materialId = faceA.materialId;
                newFaces.append(newFace);
                facesToRemove.insert(fA);
                facesToRemove.insert(fB);
            }
        } else if (faceList.size() == 1) {
            // Boundary edge: just remove it from the face
            int fi = faceList[0];
            if (facesToRemove.contains(fi)) continue;
            const Face& old = result.faces[fi];
            QVector<int> newIndices;
            for (int i = 0; i < old.indices.size(); ++i) {
                int idx = old.indices[i];
                if (idx != v1 && idx != v2) newIndices.append(idx);
            }
            if (newIndices.size() >= 3) {
                Face newFace;
                newFace.indices = newIndices;
                newFace.materialId = old.materialId;
                newFaces.append(newFace);
                facesToRemove.insert(fi);
            }
        }
    }

    // Build final face list: keep non-removed faces + new merged faces
    QVector<Face> finalFaces;
    for (int i = 0; i < result.faces.size(); ++i) {
        if (!facesToRemove.contains(i)) {
            finalFaces.append(result.faces[i]);
        }
    }
    finalFaces += newFaces;
    result.faces = finalFaces;

    // Rebuild edge list
    result.edges.clear();
    ensureEdgeList(result);
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::dissolveFaces(const MeshData& mesh, const QVector<int>& faceIndices) {
    MeshData result = mesh;
    QSet<int> removeSet(faceIndices.begin(), faceIndices.end());
    QVector<Face> newFaces;
    for (int i = 0; i < mesh.faces.size(); ++i) {
        if (!removeSet.contains(i)) newFaces.append(mesh.faces[i]);
    }
    result.faces = newFaces;
    return result;
}

MeshData MeshOperations::dissolveVertices(const MeshData& mesh, const QVector<int>& vertexIndices) {
    MeshData result = mesh;
    if (vertexIndices.isEmpty()) return result;

    QSet<int> vertSet(vertexIndices.begin(), vertexIndices.end());
    QSet<int> facesToRemove;
    QVector<Face> keptFaces;

    // Remove the vertex from all faces that contain it
    for (const Face& f : result.faces) {
        QVector<int> kept;
        bool hasAny = false;
        for (int idx : f.indices) {
            if (!vertSet.contains(idx)) {
                kept.append(idx);
            } else {
                hasAny = true;
            }
        }
        if (!hasAny) {
            keptFaces.append(f);
        } else if (kept.size() >= 3) {
            // Face still valid after removing vertices
            Face nf;
            nf.indices = kept;
            nf.materialId = f.materialId;
            keptFaces.append(nf);
        }
        // If kept.size() < 3, face is degenerate → dropped
    }

    result.faces = keptFaces;

    // Remove orphaned vertices (no longer referenced by any face)
    QSet<int> usedVerts;
    for (const Face& f : result.faces) {
        for (int idx : f.indices) {
            usedVerts.insert(idx);
        }
    }
    // Build remap for surviving vertices
    QMap<int, int> vertRemap;
    QVector<Vertex> newVerts;
    for (int i = 0; i < result.vertices.size(); ++i) {
        if (usedVerts.contains(i)) {
            vertRemap[i] = newVerts.size();
            newVerts.append(result.vertices[i]);
        }
    }
    // Remap face indices
    for (Face& f : result.faces) {
        for (int& idx : f.indices) {
            idx = vertRemap.value(idx, 0);
        }
    }
    result.vertices = newVerts;

    // Clean up other vertex-indexed arrays
    if (!result.normals.isEmpty()) {
        QVector<QVector3D> newNormals;
        for (int i = 0; i < result.normals.size(); ++i) {
            if (usedVerts.contains(i)) newNormals.append(result.normals[i]);
        }
        result.normals = newNormals;
    }
    if (!result.uvs.isEmpty()) {
        QVector<QVector2D> newUVs;
        for (int i = 0; i < result.uvs.size(); ++i) {
            if (usedVerts.contains(i)) newUVs.append(result.uvs[i]);
        }
        result.uvs = newUVs;
    }
    if (!result.uv2s.isEmpty()) {
        QVector<QVector2D> newUV2s;
        for (int i = 0; i < result.uv2s.size(); ++i) {
            if (usedVerts.contains(i)) newUV2s.append(result.uv2s[i]);
        }
        result.uv2s = newUV2s;
    }

    result.edges.clear();
    ensureEdgeList(result);
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::loft(const QVector<MeshData>& profiles, bool close) {
    MeshData result;

    if (profiles.isEmpty()) return result;

    int maxVerts = 0;
    for (const auto& p : profiles) {
        maxVerts = qMax(maxVerts, p.getVertexCount());
    }

    for (int i = 0; i < profiles.size(); ++i) {
        const MeshData& profile = profiles[i];
        float t = float(i) / (profiles.size() - 1);

        int nextIdx = (i < profiles.size() - 1) ? i + 1 : i;
        const MeshData& nextProfile = profiles[nextIdx];

        for (int v = 0; v < maxVerts; ++v) {
            const Vertex& v1 = profile.vertices.value(v, profile.vertices.first());
            const Vertex& v2 = nextProfile.vertices.value(v, nextProfile.vertices.first());

            Vertex nv;
            nv.position = v1.position + (v2.position - v1.position) * t;
            nv.normal = QVector3D::crossProduct(v1.normal, v2.normal).normalized();
            nv.uv = v1.uv + (v2.uv - v1.uv) * t;

            result.vertices.append(nv);
        }

        if (i > 0) {
            int prevOffset = (i - 1) * maxVerts;
            int currOffset = i * maxVerts;
            for (int v = 0; v < maxVerts - 1; ++v) {
                result.faces.append({prevOffset + v, currOffset + v, currOffset + v + 1, prevOffset + v + 1});
            }
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::sweep(const MeshData& profile, const QVector<QMatrix4x4>& transforms, bool close) {
    MeshData result;

    for (const auto& transform : transforms) {
        for (const auto& v : profile.vertices) {
            Vertex nv;
            nv.position = transform.map(v.position);
            nv.normal = transform.mapVector(v.normal).normalized();
            nv.uv = v.uv;
            result.vertices.append(nv);
        }
    }

    int profileSize = profile.vertices.size();
    for (int i = 0; i < transforms.size() - 1; ++i) {
        for (int v = 0; v < profileSize - 1; ++v) {
            int curr = i * profileSize + v;
            result.faces.append({curr, curr + profileSize, curr + profileSize + 1, curr + 1});
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::spin(const MeshData& profile, const QVector3D& axis, float angle, int steps) {
    MeshData result;

    QVector<QMatrix4x4> transforms;
    for (int i = 0; i <= steps; ++i) {
        float t = float(i) / steps;
        float currentAngle = angle * t;

        QMatrix4x4 rot;
        rot.setToIdentity();

        QVector3D normalizedAxis = axis.normalized();
        QQuaternion q = QQuaternion::fromAxisAndAngle(normalizedAxis, currentAngle * 180.0f / M_PI);
        rot.rotate(q);

        transforms.append(rot);
    }

    return sweep(profile, transforms, false);
}

MeshData MeshOperations::subdivide(const MeshData& input, int levels) {
    MeshData mesh = input;

    for (int l = 0; l < levels; ++l) {
        MeshData newMesh;

        QMap<QPair<int, int>, int> edgeMidpoints;

        auto getEdgeMidpoint = [&](int v1, int v2) -> int {
            QPair<int, int> edge = (v1 < v2) ? QPair<int, int>(v1, v2) : QPair<int, int>(v2, v1);
            if (edgeMidpoints.contains(edge)) {
                return edgeMidpoints[edge];
            }

            const Vertex& a = mesh.vertices[v1];
            const Vertex& b = mesh.vertices[v2];

            Vertex mid;
            mid.position = (a.position + b.position) / 2.0f;
            mid.normal = (a.normal + b.normal).normalized();
            mid.uv = (a.uv + b.uv) / 2.0f;

            int idx = newMesh.vertices.size();
            newMesh.vertices.append(mid);
            edgeMidpoints[edge] = idx;
            return idx;
        };

        for (const auto& v : mesh.vertices) {
            newMesh.vertices.append(v);
        }

        for (const auto& face : mesh.faces) {
            if (face.indices.size() != 3) continue;

            int v0 = face[0], v1 = face[1], v2 = face[2];
            int m01 = getEdgeMidpoint(v0, v1);
            int m12 = getEdgeMidpoint(v1, v2);
            int m20 = getEdgeMidpoint(v2, v0);

            newMesh.faces.append({v0, m01, m20});
            newMesh.faces.append({m01, v1, m12});
            newMesh.faces.append({m20, m12, v2});
            newMesh.faces.append({m01, m12, m20});
        }

        mesh = newMesh;
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::unsubdivide(const MeshData& mesh, float detail) {
    if (mesh.vertices.isEmpty() || detail <= 0.0f) return mesh;
    // Un-subdivide: weld vertices within detail threshold to reduce complexity
    return weldVertices(mesh, detail);
}

MeshData MeshOperations::triangulate(const MeshData& mesh) {
    MeshData result = mesh;
    result.triangulate();
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::quadrangulate(const MeshData& mesh) {
    MeshData result = mesh;
    if (result.faces.size() < 2) return result;

    // Ensure triangulated input
    result.triangulate();

    // Build face adjacency: faces that share an edge (2 shared vertices)
    QVector<QVector<int>> adj(result.faces.size());
    QMap<QPair<int,int>, int> edgeToFace;
    for (int fi = 0; fi < result.faces.size(); ++fi) {
        const Face& f = result.faces[fi];
        if (f.indices.size() < 3) continue;
        for (int i = 0; i < 3; ++i) {
            int a = f.indices[i], b = f.indices[(i+1)%3];
            if (a > b) qSwap(a, b);
            auto key = qMakePair(a, b);
            if (edgeToFace.contains(key)) {
                int other = edgeToFace[key];
                adj[fi].append(other);
                adj[other].append(fi);
            } else {
                edgeToFace[key] = fi;
            }
        }
    }

    // Merge adjacent face pairs into quads
    QVector<bool> used(result.faces.size(), false);
    QVector<Face> merged;
    for (int fi = 0; fi < result.faces.size(); ++fi) {
        if (used[fi]) continue;
        int bestMatch = -1;
        for (int nb : adj[fi]) {
            if (!used[nb]) { bestMatch = nb; break; }
        }
        if (bestMatch >= 0) {
            const Face& f1 = result.faces[fi];
            const Face& f2 = result.faces[bestMatch];

            // Find the shared edge (two common vertices)
            QVector<int> verts;
            for (int i = 0; i < 3; ++i) {
                bool shared = false;
                for (int j = 0; j < 3; ++j) {
                    if (f1.indices[i] == f2.indices[j]) { shared = true; break; }
                }
                if (!shared) verts.append(f1.indices[i]);
            }
            for (int i = 0; i < 3; ++i) {
                bool inF1 = false;
                for (int j = 0; j < 3; ++j) {
                    if (f2.indices[i] == f1.indices[j]) { inF1 = true; break; }
                }
                if (!inF1) verts.append(f2.indices[i]);
            }

            if (verts.size() == 4) {
                Face q; q.materialId = f1.materialId;
                q.indices = verts;
                merged.append(q);
                used[fi] = used[bestMatch] = true;
            } else {
                merged.append(f1);
                used[fi] = true;
            }
        } else {
            merged.append(result.faces[fi]);
            used[fi] = true;
        }
    }

    result.faces = merged;
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::mirror(const MeshData& input, const QVector3D& axis, float offset) {
    MeshData mesh = input;

    for (auto& v : mesh.vertices) {
        QVector3D pos = v.position;
        QVector3D normal = axis.normalized();
        float dist = QVector3D::dotProduct(pos, normal) - offset;
        v.position = pos - normal * 2.0f * dist;
        v.normal = QVector3D::dotProduct(v.normal, normal) > 0 ? -v.normal : v.normal;
    }

    for (auto& face : mesh.faces) {
        if (face.indices.size() >= 3) {
            int tmp = face.indices[0];
            face.indices[0] = face.indices[1];
            face.indices[1] = tmp;
        }
    }

    mesh.computeNormals();
    mesh.computeBoundingBox();
    return mesh;
}

MeshData MeshOperations::array(const MeshData& input, int count, const QVector3D& offset) {
    MeshData result;

    for (int i = 0; i < count; ++i) {
        MeshData copy = input;
        for (auto& v : copy.vertices) {
            v.position += offset * i;
        }

        int baseIdx = result.vertices.size();
        for (const auto& v : copy.vertices) {
            result.vertices.append(v);
        }
        for (const auto& f : copy.faces) {
            Face nf;
            for (int idx : f.indices) {
                nf.indices.append(baseIdx + idx);
            }
            result.faces.append(nf);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::radialArray(const MeshData& input, int count, const QVector3D& axis, float angle) {
    MeshData result;

    QQuaternion baseRot = QQuaternion::fromAxisAndAngle(axis.normalized(), 0);

    for (int i = 0; i < count; ++i) {
        float currentAngle = angle * i / count;
        QQuaternion rot = QQuaternion::fromAxisAndAngle(axis.normalized(), currentAngle * 180.0f / M_PI);
        QMatrix4x4 transform;
        transform.setToIdentity();
        transform.rotate(rot);

        int baseIdx = result.vertices.size();
        for (const auto& v : input.vertices) {
            Vertex nv;
            nv.position = transform.map(v.position);
            nv.normal = transform.mapVector(v.normal).normalized();
            nv.uv = v.uv;
            result.vertices.append(nv);
        }

        for (const auto& f : input.faces) {
            Face nf;
            for (int idx : f.indices) {
                nf.indices.append(baseIdx + idx);
            }
            result.faces.append(nf);
        }
    }

    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::shrinkwrap(const MeshData& mesh, const MeshData& target, const QVector3D& direction) {
    MeshData result = mesh;
    if (result.vertices.isEmpty() || target.vertices.isEmpty() || direction.isNull()) return result;

    // Build triangle list from target mesh
    QVector<Tri> tris = meshToTris(target);
    if (tris.isEmpty()) return result;

    // For each vertex, cast ray along -direction, find closest hit
    for (auto& v : result.vertices) {
        QVector3D origin = v.position - direction * 1e3f; // start far behind
        float closestT = 1e10f;
        for (const auto& t : tris) {
            float tHit;
            if (rayTriIntersect(origin, direction, t.v0, t.v1, t.v2, tHit)) {
                if (tHit > 1e-6f && tHit < closestT)
                    closestT = tHit;
            }
        }
        if (closestT < 1e9f)
            v.position = origin + direction * closestT;
    }
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

MeshData MeshOperations::displace(const MeshData& mesh, const QImage& heightmap, float strength) {
    MeshData result = mesh;
    if (result.vertices.isEmpty() || heightmap.isNull()) return result;

    int w = heightmap.width(), h = heightmap.height();
    if (w < 1 || h < 1) return result;

    for (int i = 0; i < result.vertices.size(); ++i) {
        float u = result.normals.isEmpty() ? 0.0f : result.uvs.isEmpty() ? 0.0f : result.uvs[i].x();
        float v = result.normals.isEmpty() ? 0.0f : result.uvs.isEmpty() ? 0.0f : result.uvs[i].y();

        // Sample heightmap at UV coordinate
        int px = qBound(0, (int)(u * w), w - 1);
        int py = qBound(0, (int)((1.0f - v) * h), h - 1);
        QColor col = heightmap.pixelColor(px, py);
        float height = col.valueF(); // 0.0 – 1.0 grayscale

        // Displace along normal
        QVector3D normal = result.normals.isEmpty() ? QVector3D(0, 1, 0) : result.normals[i];
        if (normal.isNull()) normal = QVector3D(0, 1, 0);
        result.vertices[i].position += normal * height * strength;
    }
    result.computeNormals();
    result.computeBoundingBox();
    return result;
}

void MeshOperations::mergeMeshes(MeshData& target, const MeshData& source) {
    int baseIdx = target.vertices.size();
    for (const auto& v : source.vertices) {
        target.vertices.append(v);
    }
    for (const auto& f : source.faces) {
        Face nf;
        for (int idx : f.indices) {
            nf.indices.append(baseIdx + idx);
        }
        nf.materialId = f.materialId;
        target.faces.append(nf);
    }
    target.computeBoundingBox();
}

void MeshOperations::splitMeshes(const MeshData& mesh, QVector<MeshData>& result) {
    result.clear();
    int faceCount = mesh.faces.size();
    if (faceCount == 0) return;

    // Build vertex-to-face adjacency
    QMap<int, QVector<int>> vertToFaces;
    for (int fi = 0; fi < faceCount; ++fi) {
        for (int idx : mesh.faces[fi].indices) {
            vertToFaces[idx].append(fi);
        }
    }

    // Flood-fill to find connected components (faces that share a vertex)
    QVector<int> component(faceCount, -1);
    int compCount = 0;
    for (int fi = 0; fi < faceCount; ++fi) {
        if (component[fi] >= 0) continue;
        QQueue<int> queue;
        queue.enqueue(fi);
        component[fi] = compCount;
        while (!queue.isEmpty()) {
            int cur = queue.dequeue();
            const Face& f = mesh.faces[cur];
            QSet<int> neighbors;
            for (int idx : f.indices) {
                for (int nf : vertToFaces.value(idx)) {
                    if (component[nf] < 0) neighbors.insert(nf);
                }
            }
            for (int nf : neighbors) {
                component[nf] = compCount;
                queue.enqueue(nf);
            }
        }
        ++compCount;
    }

    if (compCount <= 1) {
        result.append(mesh);
        return;
    }

    // Build component face lists
    QVector<QVector<int>> compFaces(compCount);
    for (int fi = 0; fi < faceCount; ++fi) {
        compFaces[component[fi]].append(fi);
    }

    for (int ci = 0; ci < compCount; ++ci) {
        MeshData part;
        part.name = mesh.name + QString("_part%1").arg(ci);

        // Collect unique vertices used by this component
        QMap<int, int> vertRemap;
        const auto& faces = compFaces[ci];
        for (int fi : faces) {
            const Face& f = mesh.faces[fi];
            for (int idx : f.indices) {
                if (!vertRemap.contains(idx)) {
                    vertRemap[idx] = part.vertices.size();
                    part.vertices.append(mesh.vertices[idx]);
                }
            }
        }

        // Copy faces with remapped indices
        for (int fi : faces) {
            const Face& f = mesh.faces[fi];
            Face nf;
            for (int idx : f.indices) {
                nf.indices.append(vertRemap.value(idx));
            }
            nf.materialId = f.materialId;
            nf.normal = f.normal;
            part.faces.append(nf);
        }

        part.computeNormals();
        part.computeBoundingBox();
        result.append(part);
    }
}

QVector<MeshUVIsland> MeshOperations::findUVIslands(const MeshData& mesh) {
    QVector<MeshUVIsland> islands;
    int faceCount = mesh.faces.size();
    if (faceCount == 0) return islands;

    QVector<int> faceToIsland(faceCount, -1);

    // Build adjacency: faces connected by shared vertex
    QMap<int, QVector<int>> vertexToFaces;
    for (int i = 0; i < faceCount; ++i)
        for (int idx : mesh.faces[i].indices)
            vertexToFaces[idx].append(i);

    // Flood-fill through face adjacency
    for (int start = 0; start < faceCount; ++start) {
        if (faceToIsland[start] != -1) continue;

        int islandIdx = islands.size();
        MeshUVIsland island;
        island.minUV = QVector2D(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        island.maxUV = QVector2D(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        QVector<int> stack;
        stack.append(start);
        faceToIsland[start] = islandIdx;

        while (!stack.isEmpty()) {
            int fi = stack.takeLast();
            island.faceIndices.append(fi);

            // Expand UV bounds
            const auto& face = mesh.faces[fi];
            for (int j = 0; j < face.indices.size(); ++j) {
                int uvIdx = (j < face.uvIndices.size()) ? face.uvIndices[j] : face.indices[j];
                if (uvIdx >= 0 && uvIdx < mesh.uvs.size()) {
                    const QVector2D& uv = mesh.uvs[uvIdx];
                    island.minUV.setX(qMin(island.minUV.x(), uv.x()));
                    island.minUV.setY(qMin(island.minUV.y(), uv.y()));
                    island.maxUV.setX(qMax(island.maxUV.x(), uv.x()));
                    island.maxUV.setY(qMax(island.maxUV.y(), uv.y()));
                }
            }

            // Find neighbor faces sharing a vertex
            QSet<int> neighbors;
            for (int idx : face.indices) {
                for (int nf : vertexToFaces.value(idx)) {
                    if (faceToIsland[nf] == -1)
                        neighbors.insert(nf);
                }
            }
            for (int nf : neighbors) {
                faceToIsland[nf] = islandIdx;
                stack.append(nf);
            }
        }

        if (island.minUV.x() == std::numeric_limits<float>::max()) island.minUV = QVector2D(0, 0);
        if (island.maxUV.x() == -std::numeric_limits<float>::max()) island.maxUV = QVector2D(0, 0);
        islands.append(island);
    }
    return islands;
}

QVector<int> MeshOperations::findEdge(const MeshData& mesh, int v1, int v2) {
    QVector<int> result;
    for (int i = 0; i < mesh.faces.size(); ++i) {
        const auto& face = mesh.faces[i];
        bool hasV1 = face.indices.contains(v1);
        bool hasV2 = face.indices.contains(v2);
        if (hasV1 && hasV2)
            result.append(i);
    }
    return result;
}

bool MeshOperations::isEdge(const MeshData& mesh, int v1, int v2) {
    for (const auto& face : mesh.faces) {
        bool hasV1 = face.indices.contains(v1);
        bool hasV2 = face.indices.contains(v2);
        if (hasV1 && hasV2)
            return true;
    }
    return false;
}

QVector3D MeshOperations::computeFaceNormal(const MeshData& mesh, int faceIndex) {
    if (faceIndex >= mesh.faces.size()) return QVector3D(0, 0, 1);
    const auto& face = mesh.faces[faceIndex];
    if (face.indices.size() < 3) return QVector3D(0, 0, 1);

    const QVector3D& a = mesh.vertices[face[0]].position;
    const QVector3D& b = mesh.vertices[face[1]].position;
    const QVector3D& c = mesh.vertices[face[2]].position;

    return QVector3D::crossProduct(b - a, c - a).normalized();
}

void MeshOperations::ensureEdgeList(MeshData& mesh) {
    if (!mesh.edges.isEmpty()) return;
    QSet<QPair<int,int>> seen;
    for (const auto& face : mesh.faces) {
        int n = face.indices.size();
        for (int i = 0; i < n; ++i) {
            int a = face.indices[i];
            int b = face.indices[(i + 1) % n];
            if (a > b) qSwap(a, b);
            QPair<int,int> key(a, b);
            if (!seen.contains(key)) {
                seen.insert(key);
                mesh.edges.append(Edge(a, b));
            }
        }
    }
}

}