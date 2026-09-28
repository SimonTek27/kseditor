#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace ks {
namespace engine {
namespace assets {

class AssetManager {
public:
    static AssetManager& instance() { static AssetManager s; return s; }
    bool initialize() { return true; }
    void shutdown() { m_paths.clear(); }
    void registerPath(const std::string& key, const std::string& path) { m_paths[key] = path; }
    std::string resolve(const std::string& key) const {
        auto it = m_paths.find(key);
        return it == m_paths.end() ? key : it->second;
    }
    std::vector<std::string> keys() const {
        std::vector<std::string> out;
        for (const auto& kv : m_paths) out.push_back(kv.first);
        return out;
    }
private:
    std::unordered_map<std::string, std::string> m_paths;
};

} // namespace assets
} // namespace engine
} // namespace ks
