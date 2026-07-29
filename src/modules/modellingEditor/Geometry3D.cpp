#include "Geometry3D.h"
#include <cmath>
#include <QJsonObject>
#include <QUuid>

namespace ks {
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
} // namespace ks
