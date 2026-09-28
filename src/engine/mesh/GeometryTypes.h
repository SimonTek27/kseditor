#pragma once
#include <vector>
#include <cstdint>
namespace ks { namespace engine { namespace mesh {
struct Vec3 { float x=0,y=0,z=0; };
struct MeshBuffers {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
};
}}} // namespace
