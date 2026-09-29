#include "TerrainMesh.h"

#include <fstream>
#include <cmath>
#include <algorithm>

namespace ks::engine::terrain {

namespace {

float sampleH(const std::vector<float>& heights, int gridW, int gridH, int x, int z) {
    x = std::clamp(x, 0, gridW - 1);
    z = std::clamp(z, 0, gridH - 1);
    return heights[static_cast<size_t>(z) * static_cast<size_t>(gridW) + static_cast<size_t>(x)];
}

// Central-difference surface normal at grid cell (x,z). dx/dz are the
// world-space distance between adjacent grid samples (so the normal comes
// out correct regardless of grid resolution vs. world size).
void computeNormal(const std::vector<float>& heights, int gridW, int gridH,
                   int x, int z, float dx, float dz, float& nx, float& ny, float& nz) {
    float hL = sampleH(heights, gridW, gridH, x - 1, z);
    float hR = sampleH(heights, gridW, gridH, x + 1, z);
    float hD = sampleH(heights, gridW, gridH, x, z - 1);
    float hU = sampleH(heights, gridW, gridH, x, z + 1);

    // Tangent along +X: tX = (2*dx, hR-hL, 0). Tangent along +Z:
    // tZ = (0, hU-hD, 2*dz). Surface normal = tZ x tX (this ordering, not
    // tX x tZ, is what points "up" for a flat heightmap given tX/tZ here
    // use a left-to-right / down-to-up finite difference).
    float ex = 2.0f * dx, ey = hR - hL;
    float fz = 2.0f * dz, fy = hU - hD;

    nx = -fz * ey;
    ny = fz * ex;
    nz = -fy * ex;

    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len < 1e-8f) { nx = 0; ny = 1; nz = 0; return; }
    nx /= len; ny /= len; nz /= len;
}

} // namespace

TerrainMeshData generateTerrainMesh(const std::vector<float>& heights, int gridW, int gridH,
                                    float worldW, float worldH, float uvScale) {
    TerrainMeshData mesh;
    if (gridW < 2 || gridH < 2 || static_cast<size_t>(gridW) * static_cast<size_t>(gridH) != heights.size()) {
        return mesh; // caller-visible as an empty mesh; malformed input
    }

    float dx = worldW / static_cast<float>(gridW - 1);
    float dz = worldH / static_cast<float>(gridH - 1);

    mesh.vertices.reserve(static_cast<size_t>(gridW) * static_cast<size_t>(gridH));
    for (int z = 0; z < gridH; ++z) {
        for (int x = 0; x < gridW; ++x) {
            TerrainVertex v;
            v.px = static_cast<float>(x) * dx;
            v.py = sampleH(heights, gridW, gridH, x, z);
            v.pz = static_cast<float>(z) * dz;
            computeNormal(heights, gridW, gridH, x, z, dx, dz, v.nx, v.ny, v.nz);
            v.u = v.px / uvScale;
            v.v = v.pz / uvScale;
            mesh.vertices.push_back(v);
        }
    }

    mesh.indices.reserve(static_cast<size_t>(gridW - 1) * static_cast<size_t>(gridH - 1) * 6);
    for (int z = 0; z < gridH - 1; ++z) {
        for (int x = 0; x < gridW - 1; ++x) {
            uint32_t i0 = static_cast<uint32_t>(z * gridW + x);
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + static_cast<uint32_t>(gridW);
            uint32_t i3 = i2 + 1;
            // Counter-clockwise winding when viewed from +Y (matches the
            // VK_FRONT_FACE_COUNTER_CLOCKWISE convention already used by
            // both ks::VulkanRenderer's pipelines and NativeRenderer's).
            mesh.indices.push_back(i0); mesh.indices.push_back(i2); mesh.indices.push_back(i1);
            mesh.indices.push_back(i1); mesh.indices.push_back(i2); mesh.indices.push_back(i3);
        }
    }

    return mesh;
}

TerrainMeshData generateTerrainMeshLOD(const std::vector<float>& heights, int gridW, int gridH,
                                       float worldW, float worldH, int stride, float uvScale) {
    if (stride <= 1) return generateTerrainMesh(heights, gridW, gridH, worldW, worldH, uvScale);

    int lodW = (gridW - 1) / stride + 1;
    int lodH = (gridH - 1) / stride + 1;
    if (lodW < 2 || lodH < 2) return generateTerrainMesh(heights, gridW, gridH, worldW, worldH, uvScale);

    std::vector<float> lodHeights;
    lodHeights.reserve(static_cast<size_t>(lodW) * static_cast<size_t>(lodH));
    for (int z = 0; z < lodH; ++z) {
        for (int x = 0; x < lodW; ++x) {
            lodHeights.push_back(sampleH(heights, gridW, gridH, x * stride, z * stride));
        }
    }
    return generateTerrainMesh(lodHeights, lodW, lodH, worldW, worldH, uvScale);
}

bool writeTerrainNMSH(const std::string& path, const TerrainMeshData& mesh) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f.write("NMSH", 4);
    uint32_t vCount = static_cast<uint32_t>(mesh.vertices.size());
    uint32_t iCount = static_cast<uint32_t>(mesh.indices.size());
    f.write(reinterpret_cast<const char*>(&vCount), 4);
    f.write(reinterpret_cast<const char*>(&iCount), 4);
    f.write(reinterpret_cast<const char*>(mesh.vertices.data()), static_cast<std::streamsize>(sizeof(TerrainVertex) * mesh.vertices.size()));
    f.write(reinterpret_cast<const char*>(mesh.indices.data()), static_cast<std::streamsize>(sizeof(uint32_t) * mesh.indices.size()));
    return f.good();
}

} // namespace ks::engine::terrain
