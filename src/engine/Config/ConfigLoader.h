#pragma once
#include <string>
#include <unordered_map>
namespace ks { namespace engine { namespace config {
class ConfigLoader {
public:
    static ConfigLoader& instance() { static ConfigLoader s; return s; }
    bool load(const std::string& path) { m_path = path; return true; }
    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_vals.find(key); return it == m_vals.end() ? def : it->second;
    }
    void set(const std::string& key, const std::string& val) { m_vals[key] = val; }
private:
    std::string m_path;
    std::unordered_map<std::string, std::string> m_vals;
};
}}} // namespace
