#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QVector4D>

namespace ks {

struct MeshVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D uv;
    QVector2D texCoord;
    QVector4D color = {1, 1, 1, 1};
    QVector<int> boneIndices;
    QVector<float> boneWeights;
};

class MeshRenderer : public QObject {
    Q_OBJECT
public:
    explicit MeshRenderer(QObject* parent = nullptr) : QObject(parent) {}
    ~MeshRenderer() override = default;

    bool loadFromOBJ(const QString& filePath) { Q_UNUSED(filePath); return false; }
    bool loadFromKN5(const QString& filePath) { Q_UNUSED(filePath); return false; }
    bool loadFromGLTF(const QString& filePath) { Q_UNUSED(filePath); return false; }

    const QVector<MeshVertex>& vertices() const { return m_vertices; }
    const QVector<MeshVertex>& getVertices() const { return m_vertices; }
    const QVector<uint32_t>& indices() const { return m_indices; }
    const QVector<uint32_t>& getIndices() const { return m_indices; }

    int vertexCount() const { return m_vertices.size(); }
    int indexCount() const { return m_indices.size(); }

    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    void setVertices(const QVector<MeshVertex>& v) { m_vertices = v; }
    void setIndices(const QVector<uint32_t>& i) { m_indices = i; }

signals:
    void meshLoaded(const QString& name, int vertexCount, int indexCount);
    void loadError(const QString& error);

private:
    QVector<MeshVertex> m_vertices;
    QVector<uint32_t> m_indices;
    QString m_name;
};

} // namespace ks
