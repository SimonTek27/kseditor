#include "3DModeling.h"
#include "3DModeling_io.h"
#include "3DModeling_utils.h"
#include "3DModeling_panels.h"
#include "core/mesh/ModifierSystem.h"
#include "AdditionalModifiers.h"
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>
#include <QPainter>
#include <QMap>
#include <cmath>
#include <algorithm>
#include <functional>
#include <QUuid>

namespace ks {

// ============================================================================
// ModelerContext Implementation
// ============================================================================

ModelerContext* ModelerContext::s_instance = nullptr;

ModelerContext::ModelerContext(QObject* parent)
    : QObject(parent)
{
    m_toolsByType[TypeCar] = {"Select", "Move", "Rotate", "Scale", "Paint", "Animate"};
    m_toolsByType[TypeTrack] = {"Select", "Move", "Terrain", "Road", "Vegetation"};
    m_toolsByType[TypeCharacter] = {"Select", "Pose", "Cloth", "Skin"};

    m_toolsByMode[ModeSelect] = {"Select", "BoxSelect", "Lasso"};
    m_toolsByMode[ModeEdit] = {"Move", "Rotate", "Scale", "Extrude", "LoopCut"};
    m_toolsByMode[ModePaint] = {"Paint", "Erase", "Smudge", "Clone"};
    m_toolsByMode[ModeAnimate] = {"Keyframe", "Timeline", "Graph"};
}

ModelerContext* ModelerContext::instance()
{
    if (!s_instance) {
        s_instance = new ModelerContext();
    }
    return s_instance;
}

QStringList ModelerContext::getToolsForType(EditorType type) const
{
    return m_toolsByType.value(type);
}

QStringList ModelerContext::getToolsForMode(EditMode mode) const
{
    return m_toolsByMode.value(mode);
}

bool ModelerContext::isToolValid(const QString& tool) const
{
    for (const auto& list : m_toolsByType.values()) {
        if (list.contains(tool)) return true;
    }
    for (const auto& list : m_toolsByMode.values()) {
        if (list.contains(tool)) return true;
    }
    return false;
}

// ============================================================================
// Geometry3D Implementation
// ============================================================================

namespace geometry {

void Mesh3D::computeNormals() {
    m_normals.clear();
    m_normals.resize(m_vertices.size(), QVector3D(0, 0, 0));

    for (int i = 0; i < m_indices.size(); i += 3) {
        QVector3D v0 = m_vertices[m_indices[i]];
        QVector3D v1 = m_vertices[m_indices[i + 1]];
        QVector3D v2 = m_vertices[m_indices[i + 2]];

        QVector3D n = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();

        m_normals[m_indices[i]] += n;
        m_normals[m_indices[i + 1]] += n;
        m_normals[m_indices[i + 2]] += n;
    }

    for (auto& n : m_normals) n = n.normalized();
}

void Mesh3D::subdivide(int levels) {
    for (int l = 0; l < levels; ++l) {
        int origCount = m_vertices.size();
        QVector<QVector3D> newVertices;
        for (int i = 0; i < m_indices.size(); i += 3) {
            int i0 = m_indices[i], i1 = m_indices[i + 1], i2 = m_indices[i + 2];
            QVector3D m01 = (m_vertices[i0] + m_vertices[i1]) * 0.5f;
            QVector3D m12 = (m_vertices[i1] + m_vertices[i2]) * 0.5f;
            QVector3D m20 = (m_vertices[i2] + m_vertices[i0]) * 0.5f;
            newVertices.append(m01);
            newVertices.append(m12);
            newVertices.append(m20);
        }
        m_vertices.append(newVertices);
    }
}

void Mesh3D::triangulate() {
    if (m_indices.isEmpty()) return;

    QVector<quint32> triIndices;
    for (int i = 0; i + 2 < m_indices.size(); i += 3) {
        triIndices.append(m_indices[i]);
        triIndices.append(m_indices[i + 1]);
        triIndices.append(m_indices[i + 2]);
    }

    m_indices = triIndices;
    computeNormals();
}

QVector<float> Mesh3D::toFloatArray() const {
    QVector<float> result;
    for (const auto& v : m_vertices) {
        result << v.x() << v.y() << v.z();
    }
    return result;
}

QJsonObject Material3D::toJson() const {
    QJsonObject json;
    json["name"] = m_name;
    json["diffuseR"] = m_diffuse.x();
    json["diffuseG"] = m_diffuse.y();
    json["diffuseB"] = m_diffuse.z();
    json["opacity"] = m_opacity;
    json["roughness"] = m_roughness;
    json["metallic"] = m_metallic;
    return json;
}

void Material3D::fromJson(const QJsonObject& json) {
    m_name = json["name"].toString();
    m_diffuse.setX(json["diffuseR"].toDouble());
    m_diffuse.setY(json["diffuseG"].toDouble());
    m_diffuse.setZ(json["diffuseB"].toDouble());
    m_opacity = json["opacity"].toDouble();
    m_roughness = json["roughness"].toDouble();
    m_metallic = json["metallic"].toDouble();
}

QString Scene3D::addObject(const QString& name, Mesh3D* mesh) {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Object3D* obj = new Object3D();
    obj->id = id;
    obj->name = name;
    obj->mesh = mesh;
    m_objects[id] = obj;
    emit objectAdded(id);
    return id;
}

void Scene3D::removeObject(const QString& objId) {
    if (m_objects.contains(objId)) {
        delete m_objects.take(objId);
        emit objectRemoved(objId);
    }
}

Scene3D::Object3D* Scene3D::getObject(const QString& objId) const {
    return m_objects.value(objId);
}

void Scene3D::setObjectTransform(const QString& objId, const QMatrix4x4& matrix) {
    if (m_objects.contains(objId)) {
        m_objects[objId]->transform = matrix;
        emit objectModified(objId);
    }
}

void Scene3D::setObjectPosition(const QString& objId, const QVector3D& pos) {
    if (m_objects.contains(objId)) {
        QMatrix4x4& t = m_objects[objId]->transform;
        t(0, 3) = pos.x();
        t(1, 3) = pos.y();
        t(2, 3) = pos.z();
        emit objectModified(objId);
    }
}

void Scene3D::setObjectRotation(const QString& objId, const QVector3D& rot) {
    if (m_objects.contains(objId)) {
        QMatrix4x4& t = m_objects[objId]->transform;
        QVector3D pos(t(0, 3), t(1, 3), t(2, 3));
        QVector3D scale(QVector3D(t(0, 0), t(1, 0), t(2, 0)).length(),
                        QVector3D(t(0, 1), t(1, 1), t(2, 1)).length(),
                        QVector3D(t(0, 2), t(1, 2), t(2, 2)).length());
        QMatrix4x4 r;
        r.rotate(QQuaternion::fromEulerAngles(rot));
        t = r;
        t(0, 3) = pos.x(); t(1, 3) = pos.y(); t(2, 3) = pos.z();
        t.scale(scale);
        emit objectModified(objId);
    }
}

void Scene3D::setObjectScale(const QString& objId, const QVector3D& scale) {
    if (m_objects.contains(objId)) {
        QMatrix4x4& t = m_objects[objId]->transform;
        QVector3D pos(t(0, 3), t(1, 3), t(2, 3));
        QVector3D s(QVector3D(t(0, 0), t(1, 0), t(2, 0)).length(),
                    QVector3D(t(0, 1), t(1, 1), t(2, 1)).length(),
                    QVector3D(t(0, 2), t(1, 2), t(2, 2)).length());
        float sx = s.x() > 0 ? scale.x() / s.x() : 1.0f;
        float sy = s.y() > 0 ? scale.y() / s.y() : 1.0f;
        float sz = s.z() > 0 ? scale.z() / s.z() : 1.0f;
        t.scale(sx, sy, sz);
        emit objectModified(objId);
    }
}

void Scene3D::selectObject(const QString& objId, bool select) {
    for (auto* obj : m_objects.values()) {
        obj->selected = (obj->id == objId && select);
    }
    emit selectionChanged();
}

} // namespace geometry

// ============================================================================
// Modeling3D Implementation
// ============================================================================

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

// ============================================================================
// Skeleton3D Implementation
// ============================================================================

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

// ============================================================================
// Rendering3D Implementation
// ============================================================================

namespace rendering {

QMatrix4x4 Camera3D::viewMatrix() const {
    QMatrix4x4 view;
    view.lookAt(m_position, m_target, m_up);
    return view;
}

QMatrix4x4 Camera3D::projectionMatrix() const {
    QMatrix4x4 proj;
    proj.perspective(m_fov, 16.0f/9.0f, m_near, m_far);
    return proj;
}

void Camera3D::orbit(const QVector3D& center, float azimuth, float elevation) {
    m_target = center;
    QVector3D dir = m_position - center;
    float dist = dir.length();
    if (dist < 0.001f) dist = 1.0f;

    float theta = atan2f(dir.x(), dir.z()) + azimuth;
    float phi = asinf(qBound(-1.0f, dir.y() / dist, 1.0f)) + elevation;
    phi = qBound(-1.57f, phi, 1.57f);

    m_position = center + QVector3D(
        dist * sinf(theta) * cosf(phi),
        dist * sinf(phi),
        dist * cosf(theta) * cosf(phi)
    );
    emit cameraModified();
}

void Camera3D::pan(float dx, float dy) {
    QVector3D forward = (m_target - m_position).normalized();
    QVector3D right = QVector3D::crossProduct(forward, m_up).normalized();
    QVector3D up = QVector3D::crossProduct(right, forward).normalized();

    float speed = m_position.distanceToPoint(m_target) * 0.005f;
    m_position += right * (-dx * speed) + up * (dy * speed);
    m_target += right * (-dx * speed) + up * (dy * speed);
    emit cameraModified();
}

void Camera3D::zoom(float delta) {
    QVector3D dir = (m_target - m_position);
    float dist = dir.length();
    float newDist = dist * (1.0f - delta * 0.05f);
    newDist = qBound(0.1f, newDist, 1000.0f);
    m_position = m_target - dir.normalized() * newDist;
    emit cameraModified();
}

void RenderEngine::render(int width, int height) {
    emit renderStarted(width, height);
    m_width = width;
    m_height = height;
    m_result = QImage(width, height, QImage::Format_ARGB32);
    m_result.fill(QColor::fromRgbF(m_background.x(), m_background.y(), m_background.z()));

    if (!m_scene || !m_camera) {
        emit renderComplete();
        return;
    }

    QVector<float> depthBuffer(width * height, 1.0f);
    int totalObjects = m_scene->allObjects().size();
    int processed = 0;

    auto toScreen = [&](const QVector4D& clip) -> QPointF {
        return QPointF((clip.x() * 0.5f + 0.5f) * m_width,
                       (-clip.y() * 0.5f + 0.5f) * m_height);
    };

    auto edgeFunc = [](const QPointF& a, const QPointF& b, const QPointF& c) -> float {
        return (c.x() - a.x()) * (b.y() - a.y()) - (c.y() - a.y()) * (b.x() - a.x());
    };

    for (auto* obj : m_scene->allObjects()) {
        if (!obj->mesh || !obj->visible) {
            processed++;
            continue;
        }

        auto verts = obj->mesh->vertices();
        auto norms = obj->mesh->normals();
        auto indices = obj->mesh->indices();
        QMatrix4x4 mvp = m_camera->projectionMatrix() * m_camera->viewMatrix() * obj->transform;

        for (int i = 0; i + 2 < indices.size(); i += 3) {
            QVector<QVector4D> clip(3);
            QVector<QPointF> screen(3);
            bool clipped = false;
            for (int j = 0; j < 3; ++j) {
                QVector4D v(verts[indices[i + j]], 1.0f);
                clip[j] = mvp * v;
                if (clip[j].w() == 0) { clipped = true; break; }
                clip[j] /= clip[j].w();
                screen[j] = toScreen(clip[j]);
            }
            if (clipped) continue;

            // Back-face culling
            float area = edgeFunc(screen[0], screen[1], screen[2]);
            if (area <= 0) continue;

            // Bounding box
            int minX = qMax(0, (int)std::min({screen[0].x(), screen[1].x(), screen[2].x()}));
            int maxX = qMin(width - 1, (int)std::max({screen[0].x(), screen[1].x(), screen[2].x()}));
            int minY = qMax(0, (int)std::min({screen[0].y(), screen[1].y(), screen[2].y()}));
            int maxY = qMin(height - 1, (int)std::max({screen[0].y(), screen[1].y(), screen[2].y()}));

            float invArea = 1.0f / area;
            QVector3D lightDir(0.0f, 0.5f, 1.0f);
            lightDir.normalize();

            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    QPointF p(x + 0.5f, y + 0.5f);
                    float w0 = edgeFunc(screen[1], screen[2], p);
                    float w1 = edgeFunc(screen[2], screen[0], p);
                    float w2 = edgeFunc(screen[0], screen[1], p);
                    if (w0 < 0 || w1 < 0 || w2 < 0) continue;

                    float depth = clip[0].z() * w0 + clip[1].z() * w1 + clip[2].z() * w2;
                    depth *= invArea;
                    int idx = y * width + x;
                    if (depth >= depthBuffer[idx]) continue;
                    depthBuffer[idx] = depth;

                    float b0 = w0 * invArea, b1 = w1 * invArea, b2 = w2 * invArea;
                    QVector3D norm = norms[indices[i]] * b0 +
                                     norms[indices[i + 1]] * b1 +
                                     norms[indices[i + 2]] * b2;
                    norm.normalize();
                    float ndotl = qMax(0.15f, QVector3D::dotProduct(norm, lightDir));

                    QVector4D col(1.0f, 1.0f, 1.0f, 1.0f);
                    col.setX(qMin(1.0f, col.x() * ndotl));
                    col.setY(qMin(1.0f, col.y() * ndotl));
                    col.setZ(qMin(1.0f, col.z() * ndotl));

                    m_result.setPixelColor(x, y, QColor::fromRgbF(col.x(), col.y(), col.z(), col.w()));
                }
            }
        }

        processed++;
        emit renderProgress(processed * 100 / totalObjects);
    }

    emit renderComplete();
}

} // namespace rendering

QVector3D MeshObject::getCenter() const
{
    if (vertices.isEmpty()) return QVector3D(0, 0, 0);

    QVector3D sum(0, 0, 0);
    for (const auto& v : vertices) {
        sum += v.position;
    }
    return sum / vertices.size();
}

void MeshObject::applyTransform()
{
    for (auto& v : vertices) {
        QVector4D pos(v.position, 1.0f);
        pos = transform * pos;
        v.position = pos.toVector3D();

        QVector4D norm(v.normal, 0.0f);
        norm = transform * norm;
        v.normal = norm.toVector3D().normalized();
    }
}

// ---------------------------------------------------------------------------
// MeshModifier Implementation
// ---------------------------------------------------------------------------
MeshModifier* MeshModifier::s_instance = nullptr;

MeshModifier::MeshModifier(QObject* parent) : QObject(parent) {}
MeshModifier::~MeshModifier() {
    for (auto* m : m_meshes) delete m;
    m_meshes.clear();
}

MeshModifier* MeshModifier::instance() {
    if (!s_instance) s_instance = new MeshModifier();
    return s_instance;
}

int MeshModifier::findMeshIndex(int meshId) {
    return m_meshes.contains(meshId) ? meshId : -1;
}

QVector<int> MeshModifier::findMeshesByName(const QString& infix) {
    QVector<int> results;
    for (auto it = m_meshes.constBegin(); it != m_meshes.constEnd(); ++it) {
        if (it.value()->name.contains(infix, Qt::CaseInsensitive))
            results.append(it.key());
    }
    return results;
}

int MeshModifier::findMeshByName(const QString& name) {
    for (auto it = m_meshes.constBegin(); it != m_meshes.constEnd(); ++it) {
        if (it.value()->name == name) return it.key();
    }
    return -1;
}

MeshObject* MeshModifier::getMesh(int meshId) {
    return m_meshes.value(meshId);
}

void MeshModifier::setMesh(int meshId, MeshObject* mesh) {
    if (m_meshes.contains(meshId)) {
        delete m_meshes[meshId];
        m_meshes[meshId] = mesh;
    }
}

int MeshModifier::addMesh(const QString& name, const QString& type) {
    auto* mesh = new MeshObject();
    mesh->id = QString::number(m_nextMeshId);
    mesh->name = name.isEmpty() ? "Mesh_" + QString::number(m_nextMeshId) : name;
    mesh->meshType = type.isEmpty() ? "TriMesh" : type;
    int id = m_nextMeshId++;
    m_meshes[id] = mesh;
    emit meshAdded(id);
    return id;
}

void MeshModifier::removeMesh(int meshId) {
    if (m_meshes.contains(meshId)) {
        delete m_meshes.take(meshId);
        emit meshRemoved(meshId);
    }
}

bool MeshModifier::createVertexGroup(int meshId, const QString& groupName, const QVector<int>& vertexIndices) {
    if (!m_meshes.contains(meshId) || groupName.isEmpty()) return false;
    VertexGroup vg;
    vg.name = groupName;
    vg.vertexIndices = vertexIndices;
    m_meshes[meshId]->vertexGroups.append(vg);
    return true;
}

bool MeshModifier::removeVertexGroup(int meshId, const QString& groupName) {
    if (!m_meshes.contains(meshId)) return false;
    auto& groups = m_meshes[meshId]->vertexGroups;
    for (int i = 0; i < groups.size(); ++i) {
        if (groups[i].name == groupName) {
            groups.removeAt(i);
            return true;
        }
    }
    return false;
}

QVector<int> MeshModifier::getVerticesInGroup(int meshId, const QString& groupName) {
    if (!m_meshes.contains(meshId)) return {};
    for (const auto& vg : m_meshes[meshId]->vertexGroups) {
        if (vg.name == groupName) return vg.vertexIndices;
    }
    return {};
}

QVector<int> MeshModifier::getVerticesInRadius(int meshId, const QVector3D& center, float radius) {
    QVector<int> result;
    if (!m_meshes.contains(meshId)) return result;
    float r2 = radius * radius;
    const auto& verts = m_meshes[meshId]->vertices;
    for (int i = 0; i < verts.size(); ++i) {
        if ((verts[i].position - center).lengthSquared() <= r2)
            result.append(i);
    }
    return result;
}

bool MeshModifier::createBones(int meshId, int boneCount, float radius) {
    if (!m_meshes.contains(meshId) || boneCount < 1) return false;
    auto& verts = m_meshes[meshId]->vertices;
    if (verts.isEmpty()) return false;

    QVector3D center = calculateMeshCenter(meshId);
    QVector3D bounds = calculateMeshBounds(meshId);
    float maxDim = qMax(bounds.x(), qMax(bounds.y(), bounds.z()));

    for (int i = 0; i < boneCount; ++i) {
        float angle = 2.0f * 3.14159f * i / boneCount;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);
        QVector3D bonePos = center + QVector3D(x, maxDim * 0.5f, z);

        for (auto& v : verts) {
            float dist = v.position.distanceToPoint(bonePos);
            if (dist < maxDim) {
                float weight = 1.0f - dist / maxDim;
                if (weight > 0.01f) {
                    v.boneIndices.append(i);
                    v.boneWeights.append(weight);
                }
            }
        }
    }

    return true;
}

bool MeshModifier::addSkinModifier(int meshId) {
    if (!m_meshes.contains(meshId)) return false;
    ModifierData md;
    md.type = "Skin";
    md.params["bones"] = QJsonValue(4);
    m_meshes[meshId]->modifiers.append(md);
    return true;
}

void MeshModifier::translateVertices(int meshId, const QVector<int>& vertices, const QVector3D& delta) {
    if (!m_meshes.contains(meshId)) return;
    auto& verts = m_meshes[meshId]->vertices;
    for (int idx : vertices) {
        if (idx >= 0 && idx < verts.size())
            verts[idx].position += delta;
    }
    emit meshModified(meshId);
}

void MeshModifier::rotateVertices(int meshId, const QVector<int>& vertices, const QVector3D& center, const QVector3D& rotation) {
    if (!m_meshes.contains(meshId)) return;
    auto& verts = m_meshes[meshId]->vertices;
    QMatrix4x4 rotMat;
    rotMat.translate(center);
    rotMat.rotate(QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, rotation.x()));
    rotMat.rotate(QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, rotation.y()));
    rotMat.rotate(QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 1.0f, rotation.z()));
    rotMat.translate(-center);

    for (int idx : vertices) {
        if (idx >= 0 && idx < verts.size()) {
            QVector4D p(verts[idx].position, 1.0f);
            p = rotMat * p;
            verts[idx].position = p.toVector3D();
        }
    }
    emit meshModified(meshId);
}

void MeshModifier::scaleVertices(int meshId, const QVector<int>& vertices, const QVector3D& center, const QVector3D& scale) {
    if (!m_meshes.contains(meshId)) return;
    auto& verts = m_meshes[meshId]->vertices;
    for (int idx : vertices) {
        if (idx >= 0 && idx < verts.size())
            verts[idx].position = center + (verts[idx].position - center) * scale;
    }
    emit meshModified(meshId);
}

void MeshModifier::mirrorAlongAxis(int meshId, int axis, float threshold) {
    if (!m_meshes.contains(meshId)) return;
    auto& verts = m_meshes[meshId]->vertices;
    int count = verts.size();

    for (int i = 0; i < count; ++i) {
        MeshVertex v = verts[i];
        float val = (axis == 0) ? v.position.x() : (axis == 1) ? v.position.y() : v.position.z();
        if (qAbs(val) < threshold) continue;
        MeshVertex mv = v;
        if (axis == 0) { mv.position.setX(-v.position.x()); mv.normal.setX(-v.normal.x()); }
        else if (axis == 1) { mv.position.setY(-v.position.y()); mv.normal.setY(-v.normal.y()); }
        else { mv.position.setZ(-v.position.z()); mv.normal.setZ(-v.normal.z()); }
        verts.append(mv);
    }

    int newCount = verts.size();
    int oldFaceCount = m_meshes[meshId]->faces.size();
    for (int i = 0; i < oldFaceCount; ++i) {
        MeshFace f = m_meshes[meshId]->faces[i];
        MeshFace mf;
        mf.v1 = f.v3 + (count);
        mf.v2 = f.v2 + (count);
        mf.v3 = f.v1 + (count);
        mf.materialId = f.materialId;
        m_meshes[meshId]->faces.append(mf);
    }

    emit meshModified(meshId);
}

void MeshModifier::addModifier(int meshId, const QString& modifierType, const QJsonObject& params) {
    if (!m_meshes.contains(meshId)) return;
    ModifierData md;
    md.type = modifierType;
    md.params = params;
    m_meshes[meshId]->modifiers.append(md);
}

bool MeshModifier::removeModifier(int meshId, const QString& modifierType) {
    if (!m_meshes.contains(meshId)) return false;
    auto& mods = m_meshes[meshId]->modifiers;
    for (int i = 0; i < mods.size(); ++i) {
        if (mods[i].type == modifierType) {
            mods.removeAt(i);
            return true;
        }
    }
    return false;
}

QVector3D MeshModifier::calculateMeshCenter(int meshId) {
    if (!m_meshes.contains(meshId)) return QVector3D();
    return m_meshes[meshId]->getCenter();
}

QVector3D MeshModifier::calculateMeshBounds(int meshId) {
    if (!m_meshes.contains(meshId)) return QVector3D();
    const auto& verts = m_meshes[meshId]->vertices;
    if (verts.isEmpty()) return QVector3D();

    QVector3D min = verts[0].position, max = verts[0].position;
    for (const auto& v : verts) {
        min.setX(qMin(min.x(), v.position.x()));
        min.setY(qMin(min.y(), v.position.y()));
        min.setZ(qMin(min.z(), v.position.z()));
        max.setX(qMax(max.x(), v.position.x()));
        max.setY(qMax(max.y(), v.position.y()));
        max.setZ(qMax(max.z(), v.position.z()));
    }
    return max - min;
}

// ── MeshObject ↔ MeshData conversion helpers ──
namespace {
static MeshData meshObjectToMeshData(const MeshObject& obj) {
    MeshData md;
    md.vertices.reserve(obj.vertices.size());
    for (const auto& mv : obj.vertices) {
        Vertex v;
        v.position = mv.position;
        v.normal = mv.normal;
        v.uv = mv.uv;
        md.vertices.append(v);
    }
    md.faces.reserve(obj.faces.size());
    for (const auto& mf : obj.faces) {
        Face f;
        f.indices = { mf.v1, mf.v2, mf.v3 };
        f.materialId = mf.materialId;
        md.faces.append(f);
    }
    return md;
}
static MeshObject meshDataToMeshObject(const MeshData& md, const MeshObject& templateObj) {
    MeshObject obj = templateObj;
    obj.vertices.clear();
    obj.vertices.reserve(md.vertices.size());
    for (const auto& v : md.vertices) {
        MeshVertex mv;
        mv.position = v.position;
        mv.normal = v.normal;
        mv.uv = v.uv;
        obj.vertices.append(mv);
    }
    obj.faces.clear();
    obj.faces.reserve(md.faces.size());
    for (const auto& f : md.faces) {
        if (f.indices.size() >= 3) {
            MeshFace mf;
            mf.v1 = f.indices[0];
            mf.v2 = f.indices[1];
            mf.v3 = f.indices[2];
            mf.materialId = f.materialId;
            obj.faces.append(mf);
        }
    }
    return obj;
}
static ModifierPtr createModifierFromType(const QString& type) {
    if (type == "Mirror")    return QSharedPointer<MirrorModifier>::create();
    if (type == "Array")     return QSharedPointer<ArrayModifier>::create();
    if (type == "Bevel")     return QSharedPointer<BevelModifier>::create();
    if (type == "Solidify")  return QSharedPointer<SolidifyModifier>::create();
    if (type == "Subdivision") return QSharedPointer<SubdivisionModifier>::create();
    if (type == "Decimate")  return QSharedPointer<DecimateModifier>::create();
    if (type == "Displace")  return QSharedPointer<DisplaceModifier>::create();
    if (type == "Smooth")    return QSharedPointer<SmoothModifier>::create();
    if (type == "Cast")      return QSharedPointer<CastModifier>::create();
    if (type == "Triangulate") return QSharedPointer<TriangulateModifier>::create();
    if (type == "Wireframe") return QSharedPointer<WireframeModifier>::create();
    if (type == "Remesh")    return QSharedPointer<RemeshModifier>::create();
    if (type == "Skin")      return QSharedPointer<SkinModifier>::create();
    if (type == "Shrinkwrap") return QSharedPointer<ShrinkwrapModifier>::create();
    if (type == "CageDeform") return QSharedPointer<CageDeformModifier>::create();
    if (type == "LatticeEx")  return QSharedPointer<LatticeExModifier>::create();
    if (type == "SimpleDeform") return QSharedPointer<SimpleDeformModifier>::create();
    if (type == "Curve")      return QSharedPointer<CurveModifier>::create();
    if (type == "CorrectiveSmooth") return QSharedPointer<CorrectiveSmoothModifier>::create();
    return nullptr;
}
} // anonymous namespace

bool MeshModifier::applyModifiers(int meshId) {
    if (!m_meshes.contains(meshId)) return false;
    MeshObject* obj = m_meshes[meshId];
    if (obj->vertices.isEmpty()) return true;

    MeshData data = meshObjectToMeshData(*obj);

    for (const auto& md : obj->modifiers) {
        ModifierPtr mod = createModifierFromType(md.type);
        if (!mod) continue;

        // Read parameters from JSON
        QMap<QString, QVariant> params;
        for (auto it = md.params.begin(); it != md.params.end(); ++it)
            params[it.key()] = it.value().toVariant();
        mod->readParameters(params);

        if (mod->canApply(data))
            data = mod->apply(data);
    }

    *obj = meshDataToMeshObject(data, *obj);
    emit meshModified(meshId);
    return true;
}

// ---------------------------------------------------------------------------
// Shader3D Implementation
// ---------------------------------------------------------------------------
namespace rendering {

void Shader3D::addNode(const QString& nodeId, const QString& nodeType) {
    m_nodes[nodeId] = nodeType;
    emit shaderModified();
}

void Shader3D::removeNode(const QString& nodeId) {
    m_nodes.remove(nodeId);
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const Connection& c) {
                return c.fromNode == nodeId || c.toNode == nodeId;
            }),
        m_connections.end());
    emit shaderModified();
}

void Shader3D::connectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket) {
    Connection conn;
    conn.fromNode = fromNode;
    conn.toNode = toNode;
    conn.fromSocket = fromSocket;
    conn.toSocket = toSocket;
    m_connections.append(conn);
    emit shaderModified();
}

void Shader3D::disconnectNodes(const QString& fromNode, const QString& toNode, const QString& fromSocket, const QString& toSocket) {
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [&](const Connection& c) {
                return c.fromNode == fromNode && c.toNode == toNode &&
                       c.fromSocket == fromSocket && c.toSocket == toSocket;
            }),
        m_connections.end());
    emit shaderModified();
}

QString Shader3D::compile() const {
    // Generate GLSL shader code from node graph
    QString glsl;
    glsl += "#version 450\n\n";

    if (m_type == Shader3D::PBR) {
        glsl += "layout(location = 0) in vec3 inPosition;\n";
        glsl += "layout(location = 1) in vec3 inNormal;\n";
        glsl += "layout(location = 2) in vec2 inUV;\n";
        glsl += "layout(location = 0) out vec4 outColor;\n\n";
        glsl += "void main() {\n";
        glsl += "    vec3 baseColor = vec3(" +
            QString::number(m_bsdf.baseColor[0]) + ", " +
            QString::number(m_bsdf.baseColor[1]) + ", " +
            QString::number(m_bsdf.baseColor[2]) + ");\n";
        glsl += "    float metallic = " + QString::number(m_bsdf.metallic) + ";\n";
        glsl += "    float roughness = " + QString::number(m_bsdf.roughness) + ";\n";
        glsl += "    float ao = 1.0;\n";
        glsl += "    outColor = vec4(baseColor, " + QString::number(m_bsdf.alpha) + ");\n";
        glsl += "}\n";
    } else {
        glsl += "void main() {\n";
        glsl += "    outColor = vec4(1.0);\n";
        glsl += "}\n";
    }

    return glsl;
}

} // namespace rendering

// ---------------------------------------------------------------------------
// ProceduralTextureGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralTextureGenerator::ProceduralTextureGenerator(QObject* parent) : QObject(parent) {}
ProceduralTextureGenerator::~ProceduralTextureGenerator() {}

float ProceduralTextureGenerator::fbm(float x, float y, int octaves, int seed) {
    float value = 0.0f, amplitude = 0.5f, frequency = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        float nx = x * frequency + seed * 0.01f;
        float ny = y * frequency + seed * 0.01f;
        value += amplitude * (sinf(nx * 12.9898f + ny * 78.233f) * 43758.5453f);
        value -= floorf(value);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return value;
}

float ProceduralTextureGenerator::turbulence(float x, float y, int octaves, int seed) {
    float value = 0.0f, scale = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        float nx = x * scale + seed * 3.14159f;
        float ny = y * scale + seed * 2.71828f;
        value += fabsf(sinf(nx * 12.9898f + ny * 78.233f));
        scale *= 2.0f;
    }
    return value;
}

QVector3D ProceduralTextureGenerator::marblePattern(float x, float y, int seed, const TextureParams& params) {
    float t = turbulence(x * params.scale, y * params.scale, 6, seed);
    float sine = sinf((x * params.scale + t) * 3.14159f * 4.0f);
    float mix = sine * 0.5f + 0.5f;
    mix = qBound(0.0f, mix, 1.0f);
    return params.color1 * mix + params.color2 * (1.0f - mix);
}

QVector3D ProceduralTextureGenerator::woodPattern(float x, float y, int seed) {
    float rings = sinf(sqrtf(x * x + y * y) * 8.0f + seed * 0.1f) * 0.5f + 0.5f;
    return QVector3D(0.6f + rings * 0.3f, 0.3f + rings * 0.2f, 0.1f + rings * 0.1f);
}

QVector3D ProceduralTextureGenerator::concretePattern(float x, float y, int seed) {
    float n = fbm(x * 4.0f, y * 4.0f, 4, seed);
    float base = 0.5f + n * 0.1f;
    return QVector3D(base, base, base);
}

QVector3D ProceduralTextureGenerator::asphaltPattern(float x, float y, int seed) {
    float n = fbm(x * 8.0f, y * 8.0f, 5, seed);
    float base = 0.25f + n * 0.05f;
    float grain = fmodf(x * 100.0f + y * 100.0f + seed, 1.0f) * 0.02f;
    return QVector3D(base + grain, base + grain, base + grain);
}

QImage ProceduralTextureGenerator::generateTexture(const TextureParams& params) {
    QImage image(params.width, params.height, QImage::Format_RGB32);
    int seed = params.seed != 0 ? params.seed : 12345;

    for (int y = 0; y < params.height; ++y) {
        for (int x = 0; x < params.width; ++x) {
            float fx = (float)x / params.width;
            float fy = (float)y / params.height;
            QVector3D color;

            switch (params.type) {
                case Type_Marble:
                    color = marblePattern(fx, fy, seed, params);
                    break;
                case Type_Wood:
                    color = woodPattern(fx, fy, seed);
                    break;
                case Type_Concrete:
                    color = concretePattern(fx, fy, seed);
                    break;
                case Type_Asphalt:
                    color = asphaltPattern(fx, fy, seed);
                    break;
                case Type_Grass:
                    color = QVector3D(0.2f + fbm(fx * 6, fy * 6, 3, seed) * 0.3f, 0.5f + fbm(fx * 4, fy * 4, 3, seed + 1) * 0.3f, 0.1f);
                    break;
                case Type_Metal:
                    color = QVector3D(0.6f + fbm(fx * 10, fy * 10, 2, seed) * 0.2f, 0.6f + fbm(fx * 10, fy * 10, 2, seed + 1) * 0.2f, 0.6f + fbm(fx * 10, fy * 10, 2, seed + 2) * 0.2f);
                    break;
                case Type_Carbon:
                    color = QVector3D(0.1f + fmodf(fx * 50 + fy * 50 + seed, 1.0f) * 0.05f, 0.1f, 0.1f);
                    break;
                case Type_Plastic:
                    color = QVector3D(0.3f, 0.3f, 0.35f);
                    break;
                case Type_Rust:
                    color = QVector3D(0.5f + turbulence(fx, fy, 3, seed) * 0.3f, 0.2f + turbulence(fx, fy, 3, seed + 1) * 0.15f, 0.1f);
                    break;
                case Type_Grunge: {
                    float n = turbulence(fx, fy, 4, seed);
                    color = QVector3D(n, n, n) * 0.6f;
                    break;
                }
            }

            color = color * params.contrast + QVector3D(params.brightness, params.brightness, params.brightness);
            color.setX(qBound(0.0f, color.x(), 1.0f));
            color.setY(qBound(0.0f, color.y(), 1.0f));
            color.setZ(qBound(0.0f, color.z(), 1.0f));

            image.setPixelColor(x, y, QColor::fromRgbF(color.x(), color.y(), color.z()));
        }
    }

    emit generationComplete(image);
    return image;
}

QImage ProceduralTextureGenerator::generateNormalMap(const QImage& diffuse) {
    QImage normalMap(diffuse.size(), QImage::Format_RGB32);
    float strength = 1.0f;

    for (int y = 0; y < diffuse.height(); ++y) {
        for (int x = 0; x < diffuse.width(); ++x) {
            int left = qMax(0, x - 1);
            int right = qMin(diffuse.width() - 1, x + 1);
            int top = qMax(0, y - 1);
            int bottom = qMin(diffuse.height() - 1, y + 1);

            float hL = qGray(diffuse.pixel(left, y)) / 255.0f;
            float hR = qGray(diffuse.pixel(right, y)) / 255.0f;
            float hT = qGray(diffuse.pixel(x, top)) / 255.0f;
            float hB = qGray(diffuse.pixel(x, bottom)) / 255.0f;

            float dx = (hR - hL) * strength;
            float dy = (hT - hB) * strength;
            float dz = 1.0f / sqrtf(1.0f + dx * dx + dy * dy);

            normalMap.setPixelColor(x, y, QColor::fromRgbF(
                dx * 0.5f + 0.5f,
                dy * 0.5f + 0.5f,
                dz * 0.5f + 0.5f
            ));
        }
    }

    emit generationComplete(normalMap);
    return normalMap;
}

QString ProceduralTextureGenerator::textureTypeToString(TextureType type) {
    switch (type) {
        case Type_Marble: return "Marble";
        case Type_Wood: return "Wood";
        case Type_Concrete: return "Concrete";
        case Type_Asphalt: return "Asphalt";
        case Type_Grass: return "Grass";
        case Type_Metal: return "Metal";
        case Type_Carbon: return "Carbon";
        case Type_Plastic: return "Plastic";
        case Type_Rust: return "Rust";
        case Type_Grunge: return "Grunge";
        default: return "Unknown";
    }
}

ProceduralTextureGenerator::TextureType ProceduralTextureGenerator::stringToTextureType(const QString& str) {
    if (str == "Marble") return Type_Marble;
    if (str == "Wood") return Type_Wood;
    if (str == "Concrete") return Type_Concrete;
    if (str == "Asphalt") return Type_Asphalt;
    if (str == "Grass") return Type_Grass;
    if (str == "Metal") return Type_Metal;
    if (str == "Carbon") return Type_Carbon;
    if (str == "Plastic") return Type_Plastic;
    if (str == "Rust") return Type_Rust;
    if (str == "Grunge") return Type_Grunge;
    return Type_Marble;
}

// ---------------------------------------------------------------------------
// ProceduralMeshGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralMeshGenerator::ProceduralMeshGenerator(QObject* parent) : QObject(parent) {}
ProceduralMeshGenerator::~ProceduralMeshGenerator() {}

void ProceduralMeshGenerator::generateBox(MeshData& mesh, const MeshParams& params) {
    float w = params.width / 2, h = params.height / 2, d = params.depth / 2;
    mesh.vertices = {
        {-w,-h,d}, {w,-h,d}, {w,h,d}, {-w,h,d},
        {-w,-h,-d}, {w,-h,-d}, {w,h,-d}, {-w,h,-d}
    };
    mesh.indices = {
        QVector3D(0,1,2), QVector3D(0,2,3), QVector3D(4,6,5), QVector3D(4,7,6),
        QVector3D(0,4,5), QVector3D(0,5,1), QVector3D(2,6,7), QVector3D(2,7,3),
        QVector3D(0,3,7), QVector3D(0,7,4), QVector3D(1,5,6), QVector3D(1,6,2)
    };
}

void ProceduralMeshGenerator::generateSphere(MeshData& mesh, const MeshParams& params) {
    float r = params.width / 2;
    int segs = qMax(3, params.segments);
    int rngs = qMax(3, params.rings);
    for (int i = 0; i <= rngs; ++i) {
        float phi = 3.14159f * i / rngs;
        for (int j = 0; j <= segs; ++j) {
            float theta = 2.0f * 3.14159f * j / segs;
            mesh.vertices.append(QVector3D(
                r * sinf(phi) * cosf(theta),
                r * cosf(phi),
                r * sinf(phi) * sinf(theta)
            ));
        }
    }
    for (int i = 0; i < rngs; ++i) {
        for (int j = 0; j < segs; ++j) {
            int a = i * (segs + 1) + j;
            int b = a + segs + 1;
            mesh.indices.append(QVector3D(a, b, b + 1));
            mesh.indices.append(QVector3D(a, b + 1, a + 1));
        }
    }
}

void ProceduralMeshGenerator::generateCylinder(MeshData& mesh, const MeshParams& params) {
    float r = params.width / 2, h = params.height / 2;
    int segs = qMax(3, params.segments);

    mesh.vertices.append(QVector3D(0, h, 0));
    mesh.vertices.append(QVector3D(0, -h, 0));

    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * 3.14159f * i / segs;
        float ca = cosf(a), sa = sinf(a);
        mesh.vertices.append(QVector3D(r * ca, h, r * sa));
        mesh.vertices.append(QVector3D(r * ca, -h, r * sa));
    }

    for (int i = 0; i < segs; ++i) {
        int b1 = 2 + i * 2, b2 = 2 + ((i + 1) % segs) * 2;
        mesh.indices.append(QVector3D(0, b1 + 0, b2 + 0));
        mesh.indices.append(QVector3D(1, b2 + 1, b1 + 1));
        mesh.indices.append(QVector3D(b1 + 0, b1 + 1, b2 + 1));
        mesh.indices.append(QVector3D(b1 + 0, b2 + 1, b2 + 0));
    }
}

void ProceduralMeshGenerator::generateTorus(MeshData& mesh, const MeshParams& params) {
    float majR = params.width / 2, minR = params.height / 4;
    int majS = qMax(3, params.segments), minS = qMax(3, params.rings);

    for (int i = 0; i <= majS; ++i) {
        float u = 2.0f * 3.14159f * i / majS;
        for (int j = 0; j <= minS; ++j) {
            float v = 2.0f * 3.14159f * j / minS;
            mesh.vertices.append(QVector3D(
                (majR + minR * cosf(v)) * cosf(u),
                minR * sinf(v),
                (majR + minR * cosf(v)) * sinf(u)
            ));
        }
    }

    for (int i = 0; i < majS; ++i) {
        for (int j = 0; j < minS; ++j) {
            int a = i * (minS + 1) + j;
            int b = a + minS + 1;
            mesh.indices.append(QVector3D(a, b, b + 1));
            mesh.indices.append(QVector3D(a, b + 1, a + 1));
        }
    }
}

float ProceduralMeshGenerator::heightMap(float x, float z, int seed) {
    float val = sinf(x * 0.1f + seed) * cosf(z * 0.15f + seed * 2);
    val += sinf(x * 0.05f + z * 0.08f + seed * 3) * 0.5f;
    val += sinf(x * 0.02f - z * 0.03f + seed * 5) * 0.25f;
    return val;
}

ProceduralMeshGenerator::MeshData ProceduralMeshGenerator::generateMesh(const MeshParams& params) {
    MeshData mesh;
    switch (params.primitive) {
        case MeshParams::Box: generateBox(mesh, params); break;
        case MeshParams::Sphere: generateSphere(mesh, params); break;
        case MeshParams::Cylinder: generateCylinder(mesh, params); break;
        case MeshParams::Torus: generateTorus(mesh, params); break;
        default: generateBox(mesh, params); break;
    }

    mesh.normals.resize(mesh.vertices.size());
    for (int i = 0; i + 2 < mesh.indices.size(); i += 3) {
        auto f = mesh.indices[i];
        QVector3D v0 = mesh.vertices[(int)f.x()];
        QVector3D v1 = mesh.vertices[(int)f.y()];
        QVector3D v2 = mesh.vertices[(int)f.z()];
        QVector3D n = QVector3D::crossProduct(v1 - v0, v2 - v0).normalized();
        mesh.normals[(int)f.x()] += n;
        mesh.normals[(int)f.y()] += n;
        mesh.normals[(int)f.z()] += n;
    }
    for (auto& n : mesh.normals) n = n.normalized();

    emit generationComplete(mesh);
    return mesh;
}

ProceduralMeshGenerator::MeshData ProceduralMeshGenerator::generateTerrain(int width, int height, float scale, int seed) {
    MeshData mesh;
    for (int z = 0; z <= height; ++z) {
        for (int x = 0; x <= width; ++x) {
            float y = heightMap(x * scale, z * scale, seed) * scale;
            mesh.vertices.append(QVector3D(x - width/2.0f, y, z - height/2.0f));
        }
    }

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            int a = z * (width + 1) + x;
            int b = a + 1;
            int c = (z + 1) * (width + 1) + x;
            int d = c + 1;
            mesh.indices.append(QVector3D(a, c, b));
            mesh.indices.append(QVector3D(b, c, d));
        }
    }

    emit generationComplete(mesh);
    return mesh;
}

// ---------------------------------------------------------------------------
// ProceduralTrackGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralTrackGenerator::ProceduralTrackGenerator(QObject* parent) : QObject(parent) {}
ProceduralTrackGenerator::~ProceduralTrackGenerator() {}

ProceduralTrackGenerator::TrackPoint ProceduralTrackGenerator::calculateTrackPoint(const QVector<QVector2D>& spline, float t) {
    int n = spline.size();
    float idx = t * (n - 1);
    int i0 = qMax(1, qMin(n - 2, (int)floorf(idx)));
    float frac = idx - i0;

    QVector2D pm = spline[i0 - 1];
    QVector2D p0 = spline[i0];
    QVector2D p1 = spline[qMin(n - 1, i0 + 1)];
    QVector2D p2 = spline[qMin(n - 1, i0 + 2)];

    // Catmull-Rom interpolation
    float t2 = frac * frac, t3 = t2 * frac;
    QVector2D pt = 0.5f * ((2.0f * p0) + (-pm + p1) * frac
        + (2.0f * pm - 5.0f * p0 + 4.0f * p1 - p2) * t2
        + (-pm + 3.0f * p0 - 3.0f * p1 + p2) * t3);

    QVector2D dir = (p1 - pm).normalized();
    float cross = (p0 - pm).x() * (p1 - p0).y() - (p0 - pm).y() * (p1 - p0).x();

    TrackPoint tp;
    tp.position = QVector3D(pt.x(), 0, pt.y());
    tp.tangent = QVector3D(dir.x(), 0, dir.y()).normalized();
    tp.normal = QVector3D::crossProduct(tp.tangent, QVector3D(0, 1, 0)).normalized();
    tp.curvature = cross;
    tp.width = m_trackWidth;
    return tp;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::generateTrack(const TrackParams& params) {
    TrackData data;
    int numPts = qMax(4, params.numPoints);
    m_trackWidth = params.width;

    QVector<QVector2D> splinePts;
    float angleStep = 2.0f * 3.14159f / numPts;
    for (int i = 0; i < numPts; ++i) {
        float angle = i * angleStep;
        float radius = params.minRadius + fabsf(sinf(angle * 2 + params.seed * 0.1f)) * (params.maxRadius - params.minRadius);
        splinePts.append(QVector2D(radius * cosf(angle), radius * sinf(angle)));
    }
    if (params.closed) splinePts.append(splinePts.first());

    int numTrackPts = 200;
    for (int i = 0; i < numTrackPts; ++i) {
        float t = (float)i / (numTrackPts - 1);
        TrackPoint tp = calculateTrackPoint(splinePts, t);
        data.points.append(tp);
        data.centerLine.append(tp.position);
        data.leftEdge.append(tp.position + tp.normal * tp.width * 0.5f);
        data.rightEdge.append(tp.position - tp.normal * tp.width * 0.5f);
    }

    if (params.includePitLane) {
        // Add a pit lane offset from the start/finish straight
        int pitStart = numTrackPts / 4;
        int pitEnd = numTrackPts * 3 / 4;
        float pitWidth = qMax(8.0f, params.width * 0.6f);
        for (int i = pitStart; i <= pitEnd; ++i) {
            if (i < data.points.size()) {
                QVector3D offset = data.points[i].normal * params.width * 1.2f;
                data.centerLine[i] += offset;
                data.leftEdge[i] += offset;
                data.rightEdge[i] += offset;
                data.points[i].position += offset;
                data.points[i].width = pitWidth;
            }
        }
    }

    data.totalLength = 0;
    for (int i = 1; i < data.centerLine.size(); ++i)
        data.totalLength += data.centerLine[i].distanceToPoint(data.centerLine[i - 1]);

    emit generationComplete(data);
    return data;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::addChicanes(const TrackData& baseTrack, int count, float intensity) {
    TrackData data = baseTrack;
    if (count < 1 || data.points.size() < 10) return data;

    int step = data.points.size() / (count + 1);
    for (int c = 0; c < count; ++c) {
        int idx = step * (c + 1);
        if (idx >= data.points.size()) break;
        float offset = (c % 2 == 0) ? intensity : -intensity;
        for (int i = qMax(0, idx - 3); i <= qMin(data.points.size() - 1, idx + 3); ++i) {
            float fade = 1.0f - fabsf(i - idx) / 3.0f;
            data.points[i].position += data.points[i].normal * offset * fade;
            data.centerLine[i] = data.points[i].position;
            data.leftEdge[i] = data.points[i].position + data.points[i].normal * data.points[i].width * 0.5f;
            data.rightEdge[i] = data.points[i].position - data.points[i].normal * data.points[i].width * 0.5f;
        }
    }

    emit generationComplete(data);
    return data;
}

ProceduralTrackGenerator::TrackData ProceduralTrackGenerator::addCrest(const TrackData& baseTrack, float position, float height) {
    TrackData data = baseTrack;
    if (data.points.isEmpty()) return data;

    int idx = qBound(0, (int)(position * data.points.size()), data.points.size() - 1);
    for (int i = qMax(0, idx - 10); i <= qMin(data.points.size() - 1, idx + 10); ++i) {
        float fade = 1.0f - fabsf(i - idx) / 10.0f;
        float h = height * (1.0f - fade * fade);
        data.points[i].position.setY(h);
        data.centerLine[i].setY(h);
        data.leftEdge[i].setY(h);
        data.rightEdge[i].setY(h);
    }

    emit generationComplete(data);
    return data;
}

// ---------------------------------------------------------------------------
// ProceduralCarGenerator Implementation
// ---------------------------------------------------------------------------
ProceduralCarGenerator::ProceduralCarGenerator(QObject* parent) : QObject(parent) {}
ProceduralCarGenerator::~ProceduralCarGenerator() {}

ProceduralCarGenerator::CarModel ProceduralCarGenerator::generateCar(const CarParams& params) {
    CarModel model;
    ProceduralMeshGenerator gen;
    ProceduralMeshGenerator::MeshParams mp;
    mp.width = params.length;
    mp.height = params.height;
    mp.depth = params.width;
    mp.segments = 16 + params.detailLevel * 8;
    mp.rings = 8 + params.detailLevel * 4;

    switch (params.bodyStyle) {
        case CarParams::Sedan: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::Coupe: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::SUV: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::Formula: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
        case CarParams::GT: mp.primitive = ProceduralMeshGenerator::MeshParams::Box; break;
    }

    model.body = gen.generateMesh(mp);

    for (int w = 0; w < 4; ++w) {
        ProceduralMeshGenerator::MeshData wheel;
        float wr = params.wheelRadius;
        float wt = wr * 0.4f;
        int segs = 12 + params.detailLevel * 4;
        for (int i = 0; i <= segs; ++i) {
            float a = 2.0f * 3.14159f * i / segs;
            wheel.vertices.append(QVector3D(0, wr * cosf(a), wr * sinf(a)));
            wheel.vertices.append(QVector3D(wt, wr * cosf(a), wr * sinf(a)));
        }
        model.wheels.append(wheel);
    }

    emit generationComplete(model);
    return model;
}

// ---------------------------------------------------------------------------
// DecalGenerator Implementation
// ---------------------------------------------------------------------------
DecalGenerator::DecalGenerator(QObject* parent) : QObject(parent) {}
DecalGenerator::~DecalGenerator() {}

QImage DecalGenerator::generateDecal(const DecalParams& params) {
    int w = qMax(64, (int)(params.width * 256));
    int h = qMax(64, (int)(params.height * 256));
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);

    switch (params.type) {
        case DecalParams::Number:
        case DecalParams::Text: {
            if (!params.text.isEmpty()) {
                QFont font(params.fontFamily, params.fontSize);
                p.setFont(font);
                p.setPen(QPen(params.outlineColor, params.outlineWidth));
                QFontMetrics fm(font);
                QRect textRect = fm.boundingRect(params.text);
                QPoint textPos((w - textRect.width()) / 2, (h + textRect.height()) / 2);
                p.drawText(textPos, params.text);
                p.setPen(QPen(params.color, 1));
                textPos -= QPoint(params.outlineWidth, 0);
                p.drawText(QPoint(textPos.x() + (int)params.outlineWidth, textPos.y()), params.text);
            }
            break;
        }
        case DecalParams::Circle: {
            p.setBrush(params.color);
            p.setPen(QPen(params.outlineColor, params.outlineWidth));
            p.drawEllipse(QPointF(w/2, h/2), w/2 - 10, h/2 - 10);
            break;
        }
        case DecalParams::Rectangle: {
            p.setBrush(params.color);
            p.setPen(QPen(params.outlineColor, params.outlineWidth));
            p.drawRect(10, 10, w - 20, h - 20);
            break;
        }
        case DecalParams::Stripe: {
            p.fillRect(0, h/3, w, h/3, params.color);
            break;
        }
        case DecalParams::Logo: {
            p.setBrush(params.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(w/2, h/2), w/3, h/3);
            p.setBrush(params.outlineColor);
            QFont font(params.fontFamily, params.fontSize * 2);
            p.setFont(font);
            p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, params.text.left(3));
            break;
        }
    }

    p.end();
    emit generationComplete(image);
    return image;
}

QImage DecalGenerator::generateNumberPlate(const QString& number, const QString& fontFamily) {
    DecalParams params;
    params.type = DecalParams::Text;
    params.text = number;
    params.fontFamily = fontFamily;
    params.fontSize = 72;
    params.width = 2.0f;
    params.height = 0.5f;
    params.color = QColor(30, 30, 30);
    params.outlineColor = Qt::white;
    params.outlineWidth = 0;
    return generateDecal(params);
}

} // namespace ks
