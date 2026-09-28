#pragma once
/** Qt-free stub — mesh data for native renderer path. */
#include <string>
#include <vector>
#include <cstdint>

namespace ks {
namespace engine {
namespace graphics {

struct SceneVertex {
    float px = 0, py = 0, pz = 0;
    float nx = 0, ny = 1, nz = 0;
    float u = 0, v = 0;
};

class SceneMesh {
public:
    static SceneMesh& instance() { static SceneMesh s; return s; }
    bool initialize() { return true; }
    void shutdown() { m_vertices.clear(); m_indices.clear(); }

    void setName(const std::string& n) { m_name = n; }
    const std::string& name() const { return m_name; }

    void setVertices(std::vector<SceneVertex> v) { m_vertices = std::move(v); }
    void setIndices(std::vector<uint32_t> i) { m_indices = std::move(i); }
    const std::vector<SceneVertex>& vertices() const { return m_vertices; }
    const std::vector<uint32_t>& indices() const { return m_indices; }

private:
    std::string m_name;
    std::vector<SceneVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

} // namespace graphics
} // namespace engine
} // namespace ks
