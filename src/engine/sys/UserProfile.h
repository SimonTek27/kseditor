#pragma once
#include <string>
#include <unordered_map>

namespace ks {
namespace engine {
namespace sys {

class UserProfile {
public:
    static UserProfile& instance() { static UserProfile s; return s; }

    void setName(const std::string& n) { m_name = n; }
    const std::string& name() const { return m_name; }

    void set(const std::string& key, const std::string& value) { m_prefs[key] = value; }
    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_prefs.find(key);
        return it == m_prefs.end() ? def : it->second;
    }

    bool load(const std::string& /*path*/) { return false; }
    bool save(const std::string& /*path*/) const { return false; }

private:
    std::string m_name;
    std::unordered_map<std::string, std::string> m_prefs;
};

} // namespace sys
} // namespace engine
} // namespace ks
