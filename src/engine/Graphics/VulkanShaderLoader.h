#pragma once
/** Qt-free shader loader facade — reads SPIR-V via std file I/O when implemented. */
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>

namespace ks {

class VulkanShaderLoader {
public:
    VulkanShaderLoader() = default;

    /** Load raw SPIR-V bytes from path. Returns empty on failure. */
    static std::vector<uint32_t> loadSpirv(const std::string& path) {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in) return {};
        const auto size = static_cast<size_t>(in.tellg());
        if (size < 4 || (size % 4) != 0) return {};
        in.seekg(0);
        std::vector<uint32_t> code(size / 4);
        in.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(size));
        return in ? code : std::vector<uint32_t>{};
    }

    bool registerShader(const std::string& name, const std::string& path) {
        auto code = loadSpirv(path);
        if (code.empty()) return false;
        m_shaders[name] = std::move(code);
        return true;
    }

    const std::vector<uint32_t>* shader(const std::string& name) const {
        auto it = m_shaders.find(name);
        return it == m_shaders.end() ? nullptr : &it->second;
    }

    void setFloat(const std::string& name, float v) { m_floats[name] = v; }
    float getFloat(const std::string& name, float def = 0.f) const {
        auto it = m_floats.find(name);
        return it == m_floats.end() ? def : it->second;
    }

private:
    std::unordered_map<std::string, std::vector<uint32_t>> m_shaders;
    std::unordered_map<std::string, float> m_floats;
};

} // namespace ks
