#pragma once
#include <string>
#include <vector>

namespace ks {
namespace engine {
namespace mesh {

struct SculptLayerInfo {
    int id = 0;
    std::string name;
    bool visible = true;
};

class SculptLayersManager {
public:
    static SculptLayersManager& instance() { static SculptLayersManager s; return s; }
    int addLayer(const std::string& name) {
        SculptLayerInfo info;
        info.id = ++m_next;
        info.name = name;
        m_layers.push_back(info);
        return info.id;
    }
    bool renameLayer(int id, const std::string& name) {
        for (auto& l : m_layers) if (l.id == id) { l.name = name; return true; }
        return false;
    }
    void clear() { m_layers.clear(); }
private:
    int m_next = 0;
    std::vector<SculptLayerInfo> m_layers;
};

} // namespace mesh
} // namespace engine
} // namespace ks
