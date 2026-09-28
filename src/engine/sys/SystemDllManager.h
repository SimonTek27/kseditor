#pragma once
#include <string>
#include <unordered_map>

namespace ks {
namespace engine {
namespace sys {

class SystemDllManager {
public:
    static SystemDllManager& instance() { static SystemDllManager s; return s; }

    bool load(const std::string& name, const std::string& path) {
        m_paths[name] = path;
        return true;
    }
    void unload(const std::string& name) { m_paths.erase(name); }
    bool isLoaded(const std::string& name) const {
        return m_paths.find(name) != m_paths.end();
    }
    void* symbol(const std::string& /*name*/, const std::string& /*sym*/) { return nullptr; }

private:
    std::unordered_map<std::string, std::string> m_paths;
};

} // namespace sys
} // namespace engine
} // namespace ks
