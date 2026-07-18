#include "SceneMesh.h"
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>
#include <limits>

namespace ks {
namespace graphics {

SceneMesh::SceneMesh(QObject* parent) : QObject(parent)
{
}

SceneMesh::SceneMesh(const SceneMeshGeometry& geometry, QObject* parent) : QObject(parent)
{
    m_geometry = geometry;
}

SceneMesh::~SceneMesh()
{
}

void SceneMesh::setGeometry(const SceneMeshGeometry& geometry)
{
    m_geometry = geometry;
    emit geometryChanged();
}

bool SceneMesh::createBuffers(VkDevice device, VkPhysicalDevice physDev, VkQueue queue, VkCommandPool pool)
{
    Q_UNUSED(device); Q_UNUSED(physDev); Q_UNUSED(queue); Q_UNUSED(pool);
    if (m_geometry.vertices.isEmpty() || m_geometry.indices.isEmpty()) return false;
    m_buffersValid = true;
    return true;
}

void SceneMesh::destroyBuffers(VkDevice device)
{
    Q_UNUSED(device);
    if (m_vertexBuffer) {
        vkDestroyBuffer(device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_indexBuffer) {
        vkDestroyBuffer(device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexMemory) {
        vkFreeMemory(device, m_vertexMemory, nullptr);
        m_vertexMemory = VK_NULL_HANDLE;
    }
    if (m_indexMemory) {
        vkFreeMemory(device, m_indexMemory, nullptr);
        m_indexMemory = VK_NULL_HANDLE;
    }
    m_buffersValid = false;
}

void SceneMesh::setMorphWeight(int targetIndex, float weight)
{
    if (targetIndex < 0 || targetIndex >= m_geometry.morphPositionDeltas.size()) return;
    if (targetIndex >= m_morphWeights.size())
        m_morphWeights.resize(targetIndex + 1);
    m_morphWeights[targetIndex] = qBound(0.0f, weight, 1.0f);
    emit geometryChanged();
}

SceneSubMesh* SceneMesh::getSubMesh(const QString& name)
{
    for (auto& sub : m_geometry.subMeshes) {
        if (sub.name == name) return &sub;
    }
    return nullptr;
}

QJsonObject SceneMesh::toJson() const
{
    QJsonObject obj;
    obj["name"] = m_geometry.name;
    obj["vertexCount"] = m_geometry.vertices.size();
    obj["indexCount"] = m_geometry.indices.size();
    return obj;
}

SceneMesh* SceneMesh::fromJson(const QJsonObject& obj)
{
    Q_UNUSED(obj);
    return new SceneMesh();
}

void SceneMeshGeometry::computeBounds()
{
    if (vertices.empty()) {
        boundsMin = boundsMax = QVector3D();
        boundsRadius = 0.0f;
        return;
    }
    
    boundsMin = boundsMax = vertices[0].position;
    for (const auto& v : vertices) {
        boundsMin.setX(qMin(boundsMin.x(), v.position.x()));
        boundsMin.setY(qMin(boundsMin.y(), v.position.y()));
        boundsMin.setZ(qMin(boundsMin.z(), v.position.z()));
        boundsMax.setX(qMax(boundsMax.x(), v.position.x()));
        boundsMax.setY(qMax(boundsMax.y(), v.position.y()));
        boundsMax.setZ(qMax(boundsMax.z(), v.position.z()));
    }
    boundsRadius = (boundsMax - boundsMin).length() * 0.5f;
}

void SceneMeshGeometry::computeTangents()
{
    if (vertices.empty() || indices.empty()) return;

    // Initialize tangent arrays
    QVector<QVector3D> tan1(vertices.size(), QVector3D(0, 0, 0));
    QVector<QVector3D> tan2(vertices.size(), QVector3D(0, 0, 0));

    // Iterate triangles
    for (int i = 0; i + 2 < indices.size(); i += 3) {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) continue;

        const QVector3D& p0 = vertices[i0].position;
        const QVector3D& p1 = vertices[i1].position;
        const QVector3D& p2 = vertices[i2].position;

        const QVector2D& uv0 = vertices[i0].uv;
        const QVector2D& uv1 = vertices[i1].uv;
        const QVector2D& uv2 = vertices[i2].uv;

        QVector3D edge1 = p1 - p0;
        QVector3D edge2 = p2 - p0;
        QVector2D duv1 = uv1 - uv0;
        QVector2D duv2 = uv2 - uv0;

        float denom = duv1.x() * duv2.y() - duv1.y() * duv2.x();
        if (std::abs(denom) < 1e-8f) continue;

        float f = 1.0f / denom;

        QVector3D t = (edge1 * duv2.y() - edge2 * duv1.y()) * f;
        QVector3D b = (edge2 * duv1.x() - edge1 * duv2.x()) * f;

        tan1[i0] += t; tan1[i1] += t; tan1[i2] += t;
        tan2[i0] += b; tan2[i1] += b; tan2[i2] += b;
    }

    // Orthogonalize and compute final tangent
    for (int i = 0; i < vertices.size(); ++i) {
        const QVector3D& n = vertices[i].normal;
        const QVector3D& t = tan1[i];

        if (t.lengthSquared() < 1e-8f) {
            vertices[i].tangent = QVector3D(1, 0, 0);
            continue;
        }

        // Gram-Schmidt orthogonalize: tangent = normalize(t - n * dot(n, t))
        QVector3D tangent = (t - n * QVector3D::dotProduct(n, t)).normalized();

        // Compute handedness for bitangent
        float w = (QVector3D::dotProduct(QVector3D::crossProduct(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
        Q_UNUSED(w);

        vertices[i].tangent = tangent;
    }
}

void SceneMeshGeometry::optimizeVertexCache()
{
    if (indices.size() < 6) return;

    // Simple vertex cache optimization using Forsyth algorithm approximation
    // Group triangles into cache-friendly batches
    const int CACHE_SIZE = 24;

    QVector<uint32_t> optimizedIndices;
    optimizedIndices.reserve(indices.size());

    QVector<bool> usedTriangle(indices.size() / 3, false);
    QVector<int> vertexScore(vertices.size(), 0);

    // Score vertices based on cache proximity
    auto scoreVertex = [&](int vi) -> int {
        return vertexScore[vi];
    };

    // Find best starting triangle
    int bestTri = 0;
    int bestScore = -1;
    for (int t = 0; t + 2 < indices.size(); t += 3) {
        int score = scoreVertex(indices[t]) + scoreVertex(indices[t + 1]) + scoreVertex(indices[t + 2]);
        if (score > bestScore) {
            bestScore = score;
            bestTri = t;
        }
    }

    // Process triangles in cache-friendly order
    QVector<int> cache;
    int currentTri = bestTri;

    for (int pass = 0; pass < indices.size() / 3; ++pass) {
        if (currentTri >= 0 && currentTri + 2 < indices.size()) {
            if (!usedTriangle[currentTri / 3]) {
                usedTriangle[currentTri / 3] = true;

                for (int v = 0; v < 3; ++v) {
                    int vi = indices[currentTri + v];
                    optimizedIndices.append(vi);

                    // Update cache
                    cache.removeAll(vi);
                    cache.prepend(vi);
                    if (cache.size() > CACHE_SIZE) cache.removeLast();

                    // Update vertex scores
                    vertexScore[vi] = CACHE_SIZE - cache.indexOf(vi);
                }
            }
        }

        // Find next best triangle (adjacent to last added vertices)
        currentTri = -1;
        int bestNewScore = -1;
        for (int t = 0; t + 2 < indices.size(); t += 3) {
            if (usedTriangle[t / 3]) continue;

            int score = 0;
            for (int v = 0; v < 3; ++v) {
                score += vertexScore[indices[t + v]];
            }

            // Bonus for cache hits
            for (int v = 0; v < 3; ++v) {
                if (cache.contains(indices[t + v])) score += 10;
            }

            if (score > bestNewScore) {
                bestNewScore = score;
                currentTri = t;
            }
        }
    }

    indices = optimizedIndices;
}

} // namespace graphics
} // namespace ks