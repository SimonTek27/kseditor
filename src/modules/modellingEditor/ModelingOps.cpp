#include "ModelingOps.h"
#include <cmath>
#include <QMap>
#include <QPair>
#include <QImage>
#include <QUuid>
#include <functional>

namespace ks {
namespace geometry {

Mesh3D* Modeling3D::createPrimitive(PrimitiveType type) {
    switch (type) {
        case Cube: return createCube();
        case Sphere: return createSphere();
        case Cylinder: return createCylinder();
        case Cone: return createCone();
        case Torus: return createTorus();
        case Plane: return createPlane();
        case Circle: return createCircle();
        default: return nullptr;
    }
}

Mesh3D* Modeling3D::createCube(float width, float height, float depth) {
    Mesh3D* mesh = new Mesh3D();
    float w = width / 2, h = height / 2, d = depth / 2;

    mesh->setVertices({
        {-w,-h,d}, {w,-h,d}, {w,h,d}, {-w,h,d},
        {-w,-h,-d}, {w,-h,-d}, {w,h,-d}, {-w,h,-d}
    });

    mesh->setIndices({0,1,2, 0,2,3, 4,6,5, 4,7,6, 0,4,5, 0,5,1,
                     2,6,7, 2,7,3, 0,3,7, 0,7,4, 1,5,6, 1,6,2});
    mesh->computeNormals();
    return mesh;
}

Mesh3D* Modeling3D::createSphere(float radius, int segments, int rings) {
    Mesh3D* mesh = new Mesh3D();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    for (int i = 0; i <= rings; ++i) {
        float phi = M_PI * i / rings;
        for (int j = 0; j <= segments; ++j) {
            float theta = 2.0f * M_PI * j / segments;
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);
            verts.append({x, y, z});
        }
    }

    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < segments; ++j) {
            indices.append(i * (segments + 1) + j);
            indices.append((i + 1) * (segments + 1) + j);
            indices.append((i + 1) * (segments + 1) + j + 1);
            indices.append(i * (segments + 1) + j);
            indices.append((i + 1) * (segments + 1) + j + 1);
            indices.append(i * (segments + 1) + j + 1);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    return mesh;
}

Mesh3D* Modeling3D::createCylinder(float radius, float height, int segments) {
    Mesh3D* mesh = new Mesh3D();
    float h = height / 2;

    QVector<QVector3D> verts;
    verts.append({0, h, 0});
    verts.append({0, -h, 0});

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        verts.append({radius * cosf(angle), h, radius * sinf(angle)});
        verts.append({radius * cosf(angle), -h, radius * sinf(angle)});
    }

    mesh->setVertices(verts);
    return mesh;
}

Mesh3D* Modeling3D::createCone(float radius, float height, int segments) {
    Mesh3D* mesh = new Mesh3D();
    float h = height;

    QVector<QVector3D> verts;
    verts.append({0, h, 0});

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        verts.append({radius * cosf(angle), 0, radius * sinf(angle)});
    }

    mesh->setVertices(verts);
    return mesh;
}

Mesh3D* Modeling3D::createTorus(float majorRadius, float minorRadius, int majorSegs, int minorSegs) {
    Mesh3D* mesh = new Mesh3D();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    for (int i = 0; i <= majorSegs; ++i) {
        float u = 2.0f * M_PI * i / majorSegs;
        for (int j = 0; j <= minorSegs; ++j) {
            float v = 2.0f * M_PI * j / minorSegs;
            float x = (majorRadius + minorRadius * cosf(v)) * cosf(u);
            float y = minorRadius * sinf(v);
            float z = (majorRadius + minorRadius * cosf(v)) * sinf(u);
            verts.append({x, y, z});
        }
    }

    for (int i = 0; i < majorSegs; ++i) {
        for (int j = 0; j < minorSegs; ++j) {
            indices.append(i * (minorSegs + 1) + j);
            indices.append((i + 1) * (minorSegs + 1) + j);
            indices.append((i + 1) * (minorSegs + 1) + j + 1);
            indices.append(i * (minorSegs + 1) + j);
            indices.append((i + 1) * (minorSegs + 1) + j + 1);
            indices.append(i * (minorSegs + 1) + j + 1);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
    return mesh;
}

Mesh3D* Modeling3D::createPlane(float width, float height, int subdivisions) {
    Mesh3D* mesh = new Mesh3D();
    float w = width / 2, h = height / 2;

    QVector<QVector3D> verts;
    for (int y = 0; y <= subdivisions; ++y) {
        for (int x = 0; x <= subdivisions; ++x) {
            verts.append({w * 2.0f * x / subdivisions - w, 0, h * 2.0f * y / subdivisions - h});
        }
    }

    QVector<quint32> indices;
    for (int y = 0; y < subdivisions; ++y) {
        for (int x = 0; x < subdivisions; ++x) {
            indices.append(y * (subdivisions + 1) + x);
            indices.append((y + 1) * (subdivisions + 1) + x);
            indices.append((y + 1) * (subdivisions + 1) + x + 1);
            indices.append(y * (subdivisions + 1) + x);
            indices.append((y + 1) * (subdivisions + 1) + x + 1);
            indices.append(y * (subdivisions + 1) + x + 1);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    return mesh;
}

Mesh3D* Modeling3D::createCircle(float radius, int segments) {
    Mesh3D* mesh = new Mesh3D();
    QVector<QVector3D> verts;
    verts.append({0, 0, 0});

    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        verts.append({radius * cosf(angle), 0, radius * sinf(angle)});
    }

    mesh->setVertices(verts);
    return mesh;
}

void Modeling3D::subdivide(Mesh3D* mesh, int levels) {
    if (!mesh) return;
    mesh->subdivide(levels);
}

void Modeling3D::triangulate(Mesh3D* mesh) {
    if (!mesh) return;
    mesh->triangulate();
}

Mesh3D* Modeling3D::createFromHeightmap(const QImage& image, float heightScale) {
    Mesh3D* mesh = new Mesh3D();
    if (image.isNull()) return mesh;

    int w = image.width();
    int h = image.height();
    QVector<QVector3D> verts;
    QVector<quint32> indices;

    for (int y = 0; y <= h; ++y) {
        for (int x = 0; x <= w; ++x) {
            int px = qMin(x, w - 1);
            int py = qMin(y, h - 1);
            float height = qGray(image.pixel(px, py)) / 255.0f * heightScale;
            verts.append(QVector3D((float)x - w/2.0f, height, (float)y - h/2.0f));
        }
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int a = y * (w + 1) + x;
            int b = a + 1;
            int c = (y + 1) * (w + 1) + x;
            int d = c + 1;
            indices.append(a); indices.append(c); indices.append(b);
            indices.append(b); indices.append(c); indices.append(d);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
    return mesh;
}

void Modeling3D::extrude(Mesh3D* mesh, const QVector3D& direction, float distance) {
    if (!mesh) return;
    QVector3D offset = direction.normalized() * distance;
    int origCount = mesh->vertices().size();
    QVector<QVector3D> verts = mesh->vertices();
    QVector<quint32> indices = mesh->indices();

    for (const auto& v : verts) {
        verts.append(v + offset);
    }

    int newVertCount = origCount;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        int a = indices[i], b = indices[i + 1], c = indices[i + 2];
        int na = a + newVertCount, nb = b + newVertCount, nc = c + newVertCount;
        indices.append(a); indices.append(b); indices.append(na);
        indices.append(b); indices.append(nb); indices.append(na);
        indices.append(b); indices.append(c); indices.append(nb);
        indices.append(c); indices.append(nc); indices.append(nb);
        indices.append(c); indices.append(a); indices.append(nc);
        indices.append(a); indices.append(na); indices.append(nc);
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
}

void Modeling3D::bevel(Mesh3D* mesh, float distance, int segments) {
    if (!mesh || segments < 1 || distance <= 0) return;

    QVector<QVector3D> verts = mesh->vertices();
    QVector<quint32> indices = mesh->indices();
    int origCount = verts.size();

    QMap<QPair<int,int>, QVector<int>> edgeFaces;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        int a = indices[i], b = indices[i + 1], c = indices[i + 2];
        edgeFaces[{qMin(a,b), qMax(a,b)}].append(i);
        edgeFaces[{qMin(b,c), qMax(b,c)}].append(i);
        edgeFaces[{qMin(c,a), qMax(c,a)}].append(i);
    }

    QMap<int, QVector3D> bevelVerts;
    for (auto it = edgeFaces.constBegin(); it != edgeFaces.constEnd(); ++it) {
        if (it.value().size() == 1) continue;
        int v1 = it.key().first, v2 = it.key().second;
        QVector3D edge = (verts[v2] - verts[v1]).normalized();
        QVector3D normal(0, 1, 0);
        if (verts[v1].length() > 0.001f) normal = verts[v1].normalized();
        QVector3D bevelDir = QVector3D::crossProduct(edge, normal).normalized();
        for (int seg = 0; seg <= segments; ++seg) {
            float t = (float)seg / segments;
            float offset = distance * sinf(t * 3.14159f);
            int vtxIdx = verts.size();
            verts.append(verts[v1] + (verts[v2] - verts[v1]) * t + bevelDir * offset);
            if (seg > 0) {
                int prev = vtxIdx - 1;
                indices.append(v1); indices.append(prev); indices.append(vtxIdx);
                indices.append(v2); indices.append(vtxIdx); indices.append(prev);
            }
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
}

void Modeling3D::inset(Mesh3D* mesh, float distance) {
    if (!mesh || distance <= 0) return;

    QVector<QVector3D> verts = mesh->vertices();
    QVector<quint32> indices;
    for (int i = 0; i + 2 < mesh->indices().size(); i += 3) {
        int a = mesh->indices()[i];
        int b = mesh->indices()[i + 1];
        int c = mesh->indices()[i + 2];

        QVector3D center = (verts[a] + verts[b] + verts[c]) / 3.0f;
        QVector3D dir = center.normalized();
        QVector3D insetA = verts[a] + (center - verts[a]).normalized() * distance;
        QVector3D insetB = verts[b] + (center - verts[b]).normalized() * distance;
        QVector3D insetC = verts[c] + (center - verts[c]).normalized() * distance;

        int ia = verts.size();
        int ib = verts.size() + 1;
        int ic = verts.size() + 2;
        verts.append(insetA);
        verts.append(insetB);
        verts.append(insetC);

        indices.append(a); indices.append(b); indices.append(ia);
        indices.append(b); indices.append(ib); indices.append(ia);
        indices.append(b); indices.append(c); indices.append(ib);
        indices.append(c); indices.append(ic); indices.append(ib);
        indices.append(c); indices.append(a); indices.append(ic);
        indices.append(a); indices.append(ia); indices.append(ic);
        indices.append(ia); indices.append(ib); indices.append(ic);
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
}

void Modeling3D::decimate(Mesh3D* mesh, float ratio) {
    if (!mesh || ratio <= 0.0f || ratio >= 1.0f) return;

    QVector<QVector3D> verts = mesh->vertices();
    QVector<quint32> indices = mesh->indices();
    int targetTriCount = qMax(4, (int)(indices.size() / 3 * ratio));

    if (targetTriCount >= indices.size() / 3) return;

    QVector<float> edgeCosts;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a = indices[i + j];
            int b = indices[i + (j + 1) % 3];
            float cost = (verts[a] - verts[b]).lengthSquared();
            edgeCosts.append(cost);
        }
    }

    QVector<bool> removed(indices.size(), false);
    int removedCount = 0;
    while (removedCount < indices.size() / 3 - targetTriCount) {
        int bestIdx = -1;
        float bestCost = 1e30f;
        for (int i = 0; i + 2 < indices.size(); i += 3) {
            if (removed[i]) continue;
            for (int j = 0; j < 3; ++j) {
                int ecIdx = (i / 3) * 3 + j;
                if (ecIdx < edgeCosts.size() && edgeCosts[ecIdx] < bestCost) {
                    bestCost = edgeCosts[ecIdx];
                    bestIdx = i;
                }
            }
        }
        if (bestIdx < 0) break;
        removed[bestIdx] = true;
        removedCount++;
    }

    QVector<quint32> newIndices;
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        if (!removed[i]) {
            newIndices.append(indices[i]);
            newIndices.append(indices[i + 1]);
            newIndices.append(indices[i + 2]);
        }
    }

    mesh->setIndices(newIndices);
    mesh->computeNormals();
}

void Modeling3D::mirror(Mesh3D* mesh, const QVector3D& axis, float pivot) {
    if (!mesh) return;

    QVector<QVector3D> verts = mesh->vertices();
    int origCount = verts.size();

    for (int i = 0; i < origCount; ++i) {
        QVector3D v = verts[i];
        QVector3D mirrored(
            axis.x() != 0 ? 2 * pivot - v.x() : v.x(),
            axis.y() != 0 ? 2 * pivot - v.y() : v.y(),
            axis.z() != 0 ? 2 * pivot - v.z() : v.z()
        );
        verts.append(mirrored);
    }

    QVector<quint32> indices = mesh->indices();
    int idxCount = indices.size();
    for (int i = 0; i + 2 < idxCount; i += 3) {
        indices.append(indices[i + 2] + origCount);
        indices.append(indices[i + 1] + origCount);
        indices.append(indices[i] + origCount);
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
}

void Modeling3D::array(Mesh3D* mesh, int count, const QVector3D& offset) {
    if (!mesh || count <= 1) return;

    QVector<QVector3D> verts = mesh->vertices();
    QVector<quint32> indices = mesh->indices();
    int origCount = verts.size();
    int idxCount = indices.size();

    for (int i = 1; i < count; ++i) {
        for (int j = 0; j < origCount; ++j) {
            verts.append(mesh->vertices()[j] + offset * i);
        }
        for (int j = 0; j < idxCount; ++j) {
            indices.append(indices[j] + origCount * i);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
}

void Modeling3D::screw(Mesh3D* mesh, int steps, float angle, float height) {
    if (!mesh || steps < 1) return;

    QVector<QVector3D> verts = mesh->vertices();
    int origCount = verts.size();
    QVector<quint32> indices = mesh->indices();
    float angleStep = angle / steps;
    float heightStep = height / steps;

    for (int i = 1; i <= steps; ++i) {
        float a = i * angleStep;
        float h = i * heightStep;
        float ca = cosf(a), sa = sinf(a);

        for (int j = 0; j < origCount; ++j) {
            QVector3D v = mesh->vertices()[j];
            verts.append(QVector3D(v.x() * ca - v.z() * sa, v.y() + h, v.x() * sa + v.z() * ca));
        }
    }

    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j + 2 < indices.size(); j += 3) {
            int base = i * origCount;
            int nextBase = (i + 1) * origCount;
            indices.append(indices[j] + base); indices.append(indices[j + 1] + base); indices.append(indices[j] + nextBase);
            indices.append(indices[j] + nextBase); indices.append(indices[j + 1] + base); indices.append(indices[j + 1] + nextBase);
            indices.append(indices[j + 1] + base); indices.append(indices[j + 2] + base); indices.append(indices[j + 1] + nextBase);
            indices.append(indices[j + 1] + nextBase); indices.append(indices[j + 2] + base); indices.append(indices[j + 2] + nextBase);
        }
    }

    mesh->setVertices(verts);
    mesh->setIndices(indices);
    mesh->computeNormals();
}

void Modeling3D::generateUVs(Mesh3D* mesh, UVProjection projection) {
    if (!mesh) return;
    auto verts = mesh->vertices();
    if (verts.isEmpty()) return;

    QVector<QVector2D> uvs(verts.size());

    QVector3D min = verts[0], max = verts[0];
    for (const auto& v : verts) {
        min.setX(qMin(min.x(), v.x()));
        min.setY(qMin(min.y(), v.y()));
        min.setZ(qMin(min.z(), v.z()));
        max.setX(qMax(max.x(), v.x()));
        max.setY(qMax(max.y(), v.y()));
        max.setZ(qMax(max.z(), v.z()));
    }
    QVector3D size = max - min;
    if (size.x() == 0) size.setX(1);
    if (size.y() == 0) size.setY(1);
    if (size.z() == 0) size.setZ(1);

    for (int i = 0; i < verts.size(); ++i) {
        switch (projection) {
            case Spherical: {
                QVector3D dir = (verts[i] - (min + size * 0.5f)).normalized();
                uvs[i] = QVector2D(0.5f + atan2f(dir.z(), dir.x()) / (2.0f * M_PI),
                                    0.5f - asinf(qBound(-1.0f, dir.y(), 1.0f)) / M_PI);
                break;
            }
            case Cylindrical: {
                QVector3D dir = QVector3D(verts[i].x(), 0, verts[i].z());
                float len = dir.length();
                if (len > 0) dir /= len;
                uvs[i] = QVector2D(0.5f + atan2f(dir.z(), dir.x()) / (2.0f * M_PI),
                                    (verts[i].y() - min.y()) / size.y());
                break;
            }
            default:
                uvs[i] = QVector2D((verts[i].x() - min.x()) / size.x(),
                                    (verts[i].z() - min.z()) / size.z());
                break;
        }
    }
    mesh->setUVs(uvs);
}

// -----------------------------------------------------------------------
// Skeleton3D Implementation
// -----------------------------------------------------------------------

QString Skeleton3D::addBone(const QString& name, const QString& parentId) {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Bone bone;
    bone.id = id;
    bone.name = name.isEmpty() ? "Bone_" + QString::number(m_bones.size()) : name;
    bone.parentId = parentId;
    bone.head = QVector3D(0, 0, 0);
    bone.tail = QVector3D(0, 1, 0);
    bone.rotation = QQuaternion();
    bone.length = 1.0f;
    m_bones[id] = bone;
    emit boneAdded(id);
    return id;
}

void Skeleton3D::removeBone(const QString& boneId) {
    m_bones.remove(boneId);
    for (auto& b : m_bones) {
        if (b.parentId == boneId) b.parentId.clear();
    }
    emit boneRemoved(boneId);
}

Skeleton3D::Bone Skeleton3D::getBone(const QString& boneId) const {
    return m_bones.value(boneId);
}

void Skeleton3D::setBonePosition(const QString& boneId, const QVector3D& pos) {
    if (m_bones.contains(boneId)) {
        Bone& b = m_bones[boneId];
        QVector3D diff = pos - b.head;
        b.head = pos;
        b.tail += diff;
        b.length = b.head.distanceToPoint(b.tail);
    }
}

void Skeleton3D::setBoneRotation(const QString& boneId, const QQuaternion& rot) {
    if (m_bones.contains(boneId)) {
        m_bones[boneId].rotation = rot;
    }
}

void Skeleton3D::calculateFK() {
    QVector<QString> rootBones;
    for (const auto& b : m_bones) {
        if (b.parentId.isEmpty()) rootBones.append(b.id);
    }

    std::function<void(const QString&, const QMatrix4x4&)> traverseFK =
        [&](const QString& boneId, const QMatrix4x4& parentMatrix) {
        if (!m_bones.contains(boneId)) return;
        Bone& b = m_bones[boneId];

        QMatrix4x4 local;
        local.translate(b.head);
        local.rotate(b.rotation);
        b.worldMatrix = parentMatrix * local;

        for (auto& child : m_bones) {
            if (child.parentId == boneId) {
                traverseFK(child.id, b.worldMatrix);
            }
        }
    };

    for (const auto& rootId : rootBones) {
        traverseFK(rootId, QMatrix4x4());
    }
    emit fkUpdated();
}

QMatrix4x4 Skeleton3D::getBoneWorldMatrix(const QString& boneId) const {
    return m_bones.value(boneId).worldMatrix;
}

void Skeleton3D::calculateIK(const QString& targetBoneId, const QVector3D& targetPos) {
    if (!m_bones.contains(targetBoneId)) return;

    QVector<QString*> chain;
    QString* id = new QString(targetBoneId);
    while (id && !id->isEmpty() && m_bones.contains(*id)) {
        chain.prepend(id);
        id = new QString(m_bones[*id].parentId);
    }

    const int maxIterations = 20;
    const float tolerance = 0.01f;
    calculateFK();

    for (int iter = 0; iter < maxIterations; ++iter) {
        Bone& endBone = m_bones[targetBoneId];
        QVector3D endPos = endBone.tail;
        if ((targetPos - endPos).length() < tolerance) { delete id; break; }

        for (int i = chain.size() - 1; i >= 0; --i) {
            Bone& bone = m_bones[*chain[i]];
            QVector3D bonePos = bone.head;
            QVector3D toEnd = (endPos - bonePos).normalized();
            QVector3D toTarget = (targetPos - bonePos).normalized();

            float dot = QVector3D::dotProduct(toEnd, toTarget);
            if (dot < 0.9999f) {
                QVector3D axis = QVector3D::crossProduct(toEnd, toTarget).normalized();
                float angle = acosf(qBound(-1.0f, dot, 1.0f));
                QQuaternion rot = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
                bone.rotation = rot * bone.rotation;
                QVector3D tailOffset = bone.tail - bone.head;
                bone.tail = bone.head + rot.rotatedVector(tailOffset);
            }
        }
        calculateFK();
        endPos = m_bones[targetBoneId].tail;
    }
    delete id;
    emit fkUpdated();
}

} // namespace geometry
} // namespace ks
