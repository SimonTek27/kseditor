#pragma once
#include <string>
#include <unordered_map>

namespace ks {
namespace engine {
namespace graphics {

class ShaderMaterial {
public:
    void setName(const std::string& n) { m_name = n; }
    const std::string& name() const { return m_name; }

    void setParam(const std::string& key, float v) { m_floats[key] = v; }
    float param(const std::string& key, float def = 0.f) const {
        auto it = m_floats.find(key);
        return it == m_floats.end() ? def : it->second;
    }

private:
    std::string m_name;
    std::unordered_map<std::string, float> m_floats;
};

} // namespace graphics
} // namespace engine
} // namespace ks
