#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QQuaternion>
#include <QImage>

namespace ks {
namespace geometry {

class Mesh3D : public QObject
{
    Q_OBJECT
public:
    explicit Mesh3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Mesh3D() {}

    void setVertices(const QVector<QVector3D>& verts) { m_vertices = verts; }
    QVector<QVector3D> vertices() const { return m_vertices; }

    void setNormals(const QVector<QVector3D>& norms) { m_normals = norms; }
    QVector<QVector3D> normals() const { return m_normals; }

    void setIndices(const QVector<quint32>& idx) { m_indices = idx; }
    QVector<quint32> indices() const { return m_indices; }

    void setUVs(const QVector<QVector2D>& uvs) { m_uvs = uvs; }
    QVector<QVector2D> uvs() const { return m_uvs; }

    void setMaterialId(const QString& id) { m_materialId = id; }
    QString materialId() const { return m_materialId; }

    void computeNormals();
    void subdivide(int levels);
    void triangulate();

    QVector<float> toFloatArray() const;

private:
    QVector<QVector3D> m_vertices;
    QVector<QVector3D> m_normals;
    QVector<quint32> m_indices;
    QVector<QVector2D> m_uvs;
    QString m_materialId;
};

class Material3D : public QObject
{
    Q_OBJECT
public:
    explicit Material3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Material3D() {}

    void setName(const QString& name) { m_name = name; }
    QString name() const { return m_name; }

    void setDiffuse(const QVector3D& color) { m_diffuse = color; }
    QVector3D diffuse() const { return m_diffuse; }

    void setSpecular(const QVector3D& color) { m_specular = color; }
    QVector3D specular() const { return m_specular; }

    void setAmbient(const QVector3D& color) { m_ambient = color; }
    QVector3D ambient() const { return m_ambient; }

    void setEmissive(const QVector3D& color) { m_emissive = color; }
    QVector3D emissive() const { return m_emissive; }

    void setOpacity(float opacity) { m_opacity = qBound(0.0f, opacity, 1.0f); }
    float opacity() const { return m_opacity; }

    void setRoughness(float roughness) { m_roughness = qBound(0.0f, roughness, 1.0f); }
    float roughness() const { return m_roughness; }

    void setMetallic(float metallic) { m_metallic = qBound(0.0f, metallic, 1.0f); }
    float metallic() const { return m_metallic; }

    void setTexture(const QString& map, const QString& path) { m_textures[map] = path; }
    QString texture(const QString& map) const { return m_textures.value(map); }

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

 signals:
    void changed();

private:
    QString m_name;
    QVector3D m_diffuse = QVector3D(0.8f, 0.8f, 0.8f);
    QVector3D m_specular = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D m_ambient = QVector3D(0.2f, 0.2f, 0.2f);
    QVector3D m_emissive = QVector3D(0, 0, 0);
    float m_opacity = 1.0f;
    float m_roughness = 0.5f;
    float m_metallic = 0.0f;
    QMap<QString, QString> m_textures;
};

class Scene3D : public QObject
{
    Q_OBJECT
public:
    explicit Scene3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Scene3D() {}

    struct Object3D {
        QString id;
        QString name;
        Mesh3D* mesh = nullptr;
        Material3D* material = nullptr;
        QMatrix4x4 transform;
        bool visible = true;
        bool selected = false;
    };

    QString addObject(const QString& name, Mesh3D* mesh);
    void removeObject(const QString& objId);
    Object3D* getObject(const QString& objId) const;
    QVector<Object3D*> allObjects() const { return m_objects.values(); }

    void setObjectTransform(const QString& objId, const QMatrix4x4& matrix);
    void setObjectPosition(const QString& objId, const QVector3D& pos);
    void setObjectRotation(const QString& objId, const QVector3D& rot);
    void setObjectScale(const QString& objId, const QVector3D& scale);

    void selectObject(const QString& objId, bool select);

    void setBackgroundColor(const QVector3D& color) { m_background = color; }
    QVector3D backgroundColor() const { return m_background; }

 signals:
    void objectAdded(const QString& objId);
    void objectRemoved(const QString& objId);
    void objectModified(const QString& objId);
    void selectionChanged();

private:
    QMap<QString, Object3D*> m_objects;
    QVector3D m_background = QVector3D(0.1f, 0.1f, 0.1f);
};

} // namespace geometry
} // namespace ks
