#pragma once

#include <QVector>
#include <QString>
#include "Math/MathCore.h"

namespace ks {

// Extended vertex with position, color, normal, and UV
struct SceneVertex {
    Vec3 position;
    Vec3 color;
    Vec3 normal;
    Vec2 uv;
};

// CPU-side mesh used by SceneObject
class SceneMesh {
public:
    SceneMesh() = default;
    ~SceneMesh();

    // CPU-side data
    QVector<SceneVertex>& vertices() { return m_vertices; }
    const QVector<SceneVertex>& vertices() const { return m_vertices; }

    QVector<uint32_t>& indices() { return m_indices; }
    const QVector<uint32_t>& indices() const { return m_indices; }

    // Utility methods
    void clear() { m_vertices.clear(); m_indices.clear(); m_boundsMin = m_boundsMax = m_boundsCenter = Vec3(); }
    void reserveVertices(int n) { m_vertices.reserve(n); }
    void reserveIndices(int n) { m_indices.reserve(n); }

    // Add a triangle (counter-clockwise winding)
    void addTriangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& color = Vec3(0.8f, 0.8f, 0.8f)) {
        uint32_t base = m_vertices.size();
        m_vertices.push_back({a, color, Vec3(), Vec2()});
        m_vertices.push_back({b, color, Vec3(), Vec2()});
        m_vertices.push_back({c, color, Vec3(), Vec2()});
        m_indices.push_back(base);
        m_indices.push_back(base + 1);
        m_indices.push_back(base + 2);
    }

    // Add a quad (split into two triangles)
    void addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color = Vec3(0.8f, 0.8f, 0.8f)) {
        addTriangle(a, b, c, color);
        addTriangle(c, d, a, color);
    }

    // Compute flat face normals for all triangles
    void computeNormals() {
        // First zero all normals
        for (auto& v : m_vertices) v.normal = Vec3();

        for (size_t i = 0; i + 2 < m_indices.size(); i += 3) {
            uint32_t i0 = m_indices[i];
            uint32_t i1 = m_indices[i+1];
            uint32_t i2 = m_indices[i+2];
            if (i0 >= m_vertices.size() || i1 >= m_vertices.size() || i2 >= m_vertices.size()) continue;
            Vec3 edge1 = m_vertices[i1].position - m_vertices[i0].position;
            Vec3 edge2 = m_vertices[i2].position - m_vertices[i0].position;
            Vec3 n = Vec3::cross(edge1, edge2);
            if (n.normalized().x != 0 || n.normalized().y != 0 || n.normalized().z != 0)
                n = n.normalized();
            m_vertices[i0].normal = n;
            m_vertices[i1].normal = n;
            m_vertices[i2].normal = n;
        }
    }

    // Recompute bounds
    void update() {
        if (m_vertices.isEmpty()) { m_boundsMin = m_boundsMax = m_boundsCenter = Vec3(); return; }
        m_boundsMin = m_boundsMax = m_vertices[0].position;
        for (int i = 1; i < m_vertices.size(); ++i) {
            const Vec3& p = m_vertices[i].position;
            m_boundsMin.x = qMin(m_boundsMin.x, p.x);
            m_boundsMin.y = qMin(m_boundsMin.y, p.y);
            m_boundsMin.z = qMin(m_boundsMin.z, p.z);
            m_boundsMax.x = qMax(m_boundsMax.x, p.x);
            m_boundsMax.y = qMax(m_boundsMax.y, p.y);
            m_boundsMax.z = qMax(m_boundsMax.z, p.z);
        }
        m_boundsCenter = Vec3(
            (m_boundsMin.x + m_boundsMax.x) * 0.5f,
            (m_boundsMin.y + m_boundsMax.y) * 0.5f,
            (m_boundsMin.z + m_boundsMax.z) * 0.5f
        );
    }

    const Vec3& boundsMin() const { return m_boundsMin; }
    const Vec3& boundsMax() const { return m_boundsMax; }
    const Vec3& boundsCenter() const { return m_boundsCenter; }

    // Material name
    QString materialName;
    QString shaderName;

    // Vertex/face counts
    size_t vertexCount() const { return m_vertices.size(); }
    size_t triangleCount() const { return m_indices.size() / 3; }

private:
    QVector<SceneVertex> m_vertices;
    QVector<uint32_t> m_indices;
    Vec3 m_boundsMin{};
    Vec3 m_boundsMax{};
    Vec3 m_boundsCenter{};
};

// Helper to create a simple colored triangle mesh
SceneMesh* createTestTriangleMesh();

} // namespace ks
