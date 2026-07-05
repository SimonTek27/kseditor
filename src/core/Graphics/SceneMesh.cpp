#include "SceneMesh.h"

namespace ks {

SceneMesh::~SceneMesh()
{
    // GPU resources were previously freed explicitly; now this is a CPU-only mesh.
}

SceneMesh* createTestTriangleMesh()
{
    SceneMesh* mesh = new SceneMesh();

    mesh->addTriangle(
        Vec3(-0.5f, -0.5f, 0.0f),
        Vec3(0.5f, -0.5f, 0.0f),
        Vec3(0.0f, 0.5f, 0.0f),
        Vec3(1.0f, 0.0f, 0.0f)
    );
    mesh->addTriangle(
        Vec3(0.5f, -0.5f, 0.0f),
        Vec3(0.0f, 0.5f, 0.0f),
        Vec3(-0.5f, -0.5f, 0.0f),
        Vec3(0.0f, 1.0f, 0.0f)
    );
    mesh->computeNormals();

    return mesh;
}

} // namespace ks
