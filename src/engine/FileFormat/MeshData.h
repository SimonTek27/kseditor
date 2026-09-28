#pragma once
#include <vector>
#include <cstdint>
#include <string>
namespace ks { namespace engine { namespace fileformat {
struct MeshData {
    std::string name;
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    std::vector<uint32_t> indices;
};
}}} // namespace
