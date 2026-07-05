#pragma once

#include <QString>
#include <QVector>
#include <QColor>
#include <QVector3D>
#include <QJsonObject>
#include <functional>

#include "Math/MathCore.h"
#include "SceneMesh.h"

namespace ks {

class SceneObject
{

public:
    enum class Type {
        Node,
        Mesh,
        Light,
        Camera,
        Bone,
        Unknown
    };

    SceneObject(int id, const QString& name, Type type = Type::Node);
    ~SceneObject();

    int id() const { return m_id; }

    const QString& name() const { return m_name; }
    void setName(const QString& n) { m_name = n; }

    Type type() const { return m_type; }
    void setType(Type t) { m_type = t; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }

    bool isSelected() const { return m_selected; }
    void setSelected(bool s) { m_selected = s; }

    // Hierarchy
    SceneObject* parent() const { return m_parent; }
    const QVector<SceneObject*>& children() const { return m_children; }

    void addChild(SceneObject* child);
    void removeChild(SceneObject* child);
    int childCount() const { return m_children.size(); }
    SceneObject* childAt(int index) const { return (index >= 0 && index < m_children.size()) ? m_children[index] : nullptr; }
    int indexOfChild(SceneObject* child) const { return m_children.indexOf(child); }

    // Transform (local)
    const Matrix4& localTransform() const { return m_transform; }
    void setLocalTransform(const Matrix4& m) { m_transform = m; m_dirty = true; }
    const Matrix4& transform() const { return m_transform; }
    void setTransform(const Matrix4& m) { m_transform = m; m_dirty = true; }

    // Translation convenience
    QVector3D translation() const {
        return QVector3D(m_transform.translation_.x, m_transform.translation_.y, m_transform.translation_.z);
    }
    void setTranslation(const QVector3D& t) {
        m_transform.setTranslation(Vec3(t.x(), t.y(), t.z()));
        m_dirty = true;
    }

    QVector3D rotationEuler() const;
    void setRotationEuler(const QVector3D& euler);

    QVector3D scale() const;
    void setScale(const QVector3D& s);

    // World transform (computed from parent chain)
    const Matrix4& worldTransform() const { return m_worldTransform; }
    void updateWorldTransform(bool force = false) const;

    // Mesh
    bool hasMesh() const { return m_mesh != nullptr; }
    void setMesh(class SceneMesh* mesh) { m_mesh = mesh; }
    class SceneMesh* mesh() const { return m_mesh; }

    // Material properties
    QColor baseColor() const { return m_baseColor; }
    void setBaseColor(const QColor& c) { m_baseColor = c; }

    float metallic() const { return m_metallic; }
    void setMetallic(float v) { m_metallic = v; }

    float roughness() const { return m_roughness; }
    void setRoughness(float v) { m_roughness = v; }

    float opacity() const { return m_opacity; }
    void setOpacity(float v) { m_opacity = v; }

    // Traversal
    SceneObject* findByName(const QString& name, bool recursive = true) const;
    SceneObject* findById(int id) const;
    QVector<SceneObject*> findByType(Type type, bool recursive = true) const;
    void traverse(const std::function<void(SceneObject*)>& visitor) const;
    int descendantCount() const;

    // Serialization
    QJsonObject serialize() const;
    void deserialize(const QJsonObject& json);
    static SceneObject* fromJson(const QJsonObject& json, int& nextId);

private:
    int m_id;
    QString m_name;
    Type m_type;
    bool m_visible = true;
    bool m_selected = false;

    mutable SceneObject* m_parent = nullptr;
    mutable QVector<SceneObject*> m_children;

    Matrix4 m_transform;
    mutable Matrix4 m_worldTransform;
    mutable bool m_dirty = true;

    class SceneMesh* m_mesh = nullptr;

    QColor m_baseColor = QColor(180, 180, 200);
    float m_metallic = 0.0f;
    float m_roughness = 0.5f;
    float m_opacity = 1.0f;
};

} // namespace ks
