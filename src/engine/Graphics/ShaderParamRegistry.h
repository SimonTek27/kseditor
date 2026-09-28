#pragma once
#include <string>
#include <unordered_map>

namespace ks {
namespace engine {
namespace graphics {

class ShaderParamRegistry {
public:
    static ShaderParamRegistry& instance() { static ShaderParamRegistry s; return s; }
    void registerParam(const std::string& name, float defaultValue = 0.f) {
        m_params[name] = defaultValue;
    }
    float get(const std::string& name, float def = 0.f) const {
        auto it = m_params.find(name);
        return it == m_params.end() ? def : it->second;
    }
    void set(const std::string& name, float v) { m_params[name] = v; }

private:
    std::unordered_map<std::string, float> m_params;
};

} // namespace graphics
} // namespace engine
} // namespace ks
