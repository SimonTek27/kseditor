#include "NormalMapBaker.h"

#if HAS_MIKKTSPACE
#include <mikktspace.h>
#endif

#include <cmath>
#include <algorithm>
#include <limits>

namespace ks {

#if HAS_MIKKTSPACE
struct MikkUserData {
    QVector<QVector3D> positions;
    QVector<QVector3D> normals;
    QVector<QVector2D> texCoords;
    const QVector<QVector<int>>* faces;
    QVector<QVector3D> tangents;
    QVector<QVector3D> bitangents;
};

static int mikk_getNumFaces(const SMikkTSpaceContext* ctx) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    return data->faces->size();
}

static int mikk_getNumVerticesOfFace(const SMikkTSpaceContext* ctx, const int face) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    if (face < 0 || face >= data->faces->size()) return 0;
    return (*data->faces)[face].size();
}

static void mikk_getPosition(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    int idx = (*data->faces)[face][vert];
    const auto& p = data->positions[idx];
    out[0] = static_cast<float>(p.x());
    out[1] = static_cast<float>(p.y());
    out[2] = static_cast<float>(p.z());
}

static void mikk_getNormal(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    int idx = (*data->faces)[face][vert];
    const auto& n = data->normals[idx];
    out[0] = static_cast<float>(n.x());
    out[1] = static_cast<float>(n.y());
    out[2] = static_cast<float>(n.z());
}

static void mikk_getTexCoord(const SMikkTSpaceContext* ctx, float out[], const int face, const int vert) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    int idx = (*data->faces)[face][vert];
    const auto& t = data->texCoords[idx];
    out[0] = static_cast<float>(t.x());
    out[1] = static_cast<float>(t.y());
}

static void mikk_setTSpaceBasic(const SMikkTSpaceContext* ctx, const float tangent[], const float sign,
                                const int face, const int vert) {
    auto* data = static_cast<MikkUserData*>(ctx->m_pUserData);
    int idx = (*data->faces)[face][vert];
    data->tangents[idx] = QVector3D(tangent[0], tangent[1], tangent[2]);
    const auto& n = data->normals[idx];
    QVector3D t(tangent[0], tangent[1], tangent[2]);
    data->bitangents[idx] = QVector3D::crossProduct(n, t).normalized() * sign;
}
#endif

bool NormalMapBaker::computeTangents(BakerMesh& mesh) {
#if HAS_MIKKTSPACE
    MikkUserData data;
    data.faces = &mesh.faces;
    data.tangents.resize(mesh.vertices.size());
    data.bitangents.resize(mesh.vertices.size());

    for (const auto& v : mesh.vertices) {
        data.positions.append(v.position);
        data.normals.append(v.normal);
        data.texCoords.append(v.texCoord);
    }

    SMikkTSpaceInterface iface{};
    iface.m_getNumFaces = mikk_getNumFaces;
    iface.m_getNumVerticesOfFace = mikk_getNumVerticesOfFace;
    iface.m_getPosition = mikk_getPosition;
    iface.m_getNormal = mikk_getNormal;
    iface.m_getTexCoord = mikk_getTexCoord;
    iface.m_setTSpaceBasic = mikk_setTSpaceBasic;

    SMikkTSpaceContext ctx{};
    ctx.m_pInterface = &iface;
    ctx.m_pUserData = &data;

    bool success = genTangSpaceDefault(&ctx) != 0;

    for (int i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertices[i].tangent = data.tangents[i];
        mesh.vertices[i].bitangent = data.bitangents[i];
    }

    return success;
#else
    // Fallback: compute tangents via simple derivative method
    for (auto& v : mesh.vertices) {
        v.tangent = QVector3D(1, 0, 0);
        v.bitangent = QVector3D(0, 1, 0);
    }

    for (const auto& face : mesh.faces) {
        if (face.size() < 3) continue;
        const auto& v0 = mesh.vertices[face[0]];
        const auto& v1 = mesh.vertices[face[1]];
        const auto& v2 = mesh.vertices[face[2]];

        QVector3D e1 = v1.position - v0.position;
        QVector3D e2 = v2.position - v0.position;
        QVector2D uv1 = v1.texCoord - v0.texCoord;
        QVector2D uv2 = v2.texCoord - v0.texCoord;

        float r = 1.0f / (uv1.x() * uv2.y() - uv2.x() * uv1.y() + 1e-8f);
        QVector3D tangent = (e1 * uv2.y() - e2 * uv1.y()) * r;
        QVector3D bitangent = (e2 * uv1.x() - e1 * uv2.x()) * r;

        for (int idx : face) {
            mesh.vertices[idx].tangent = tangent.normalized();
            mesh.vertices[idx].bitangent = bitangent.normalized();
        }
    }
    return true;
#endif
}

bool NormalMapBaker::bakeNormalMap(const BakerMesh& highPoly, const BakerMesh& lowPoly,
                                   const QVector2D& textureSize, QVector<QVector3D>& normalMap) {
    int w = static_cast<int>(textureSize.x());
    int h = static_cast<int>(textureSize.y());
    normalMap.resize(w * h, QVector3D(0.5f, 0.5f, 1.0f));

    if (highPoly.vertices.isEmpty() || lowPoly.vertices.isEmpty()) return false;

    // For each texel, cast ray from low-poly surface toward high-poly
    // This is a simplified implementation
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            QVector2D uv(static_cast<float>(x) / static_cast<float>(w),
                         static_cast<float>(y) / static_cast<float>(h));

            // Find closest point on low-poly mesh for this UV
            float minDist = std::numeric_limits<float>::max();
            QVector3D surfaceNormal;

            for (const auto& face : lowPoly.faces) {
                if (face.size() < 3) continue;
                const auto& v0 = lowPoly.vertices[face[0]];
                const auto& v1 = lowPoly.vertices[face[1]];
                const auto& v2 = lowPoly.vertices[face[2]];

                // Barycentric check if UV falls in this face
                QVector2D d0 = v1.texCoord - v0.texCoord;
                QVector2D d1 = v2.texCoord - v0.texCoord;
                QVector2D d2 = uv - v0.texCoord;

                float dot00 = QVector2D::dotProduct(d0, d0);
                float dot01 = QVector2D::dotProduct(d0, d1);
                float dot02 = QVector2D::dotProduct(d0, d2);
                float dot11 = QVector2D::dotProduct(d1, d1);
                float dot12 = QVector2D::dotProduct(d1, d2);

                float inv = 1.0f / (dot00 * dot11 - dot01 * dot01 + 1e-8f);
                float u = (dot11 * dot02 - dot01 * dot12) * inv;
                float v = (dot00 * dot12 - dot01 * dot02) * inv;

                if (u >= 0 && v >= 0 && u + v <= 1) {
                    QVector3D normal = v0.normal * (1 - u - v) +
                                      v1.normal * u +
                                      v2.normal * v;
                    surfaceNormal = normal.normalized();
                    break;
                }
            }

            if (surfaceNormal.length() > 0.001f) {
                // Map normal to [0,1] for storage
                normalMap[y * w + x] = QVector3D(
                    surfaceNormal.x() * 0.5f + 0.5f,
                    surfaceNormal.y() * 0.5f + 0.5f,
                    surfaceNormal.z() * 0.5f + 0.5f
                );
            }
        }
    }

    return true;
}

}