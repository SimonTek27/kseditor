#pragma once
/** Qt-free XR config (no QSettings/QObject). */
#include <string>
#include <unordered_map>

namespace ks {
namespace engine {
namespace devices {
namespace vr {

class XrConfig {
public:
    static XrConfig& instance() { static XrConfig s; return s; }

    void set(const std::string& key, const std::string& value) { m_vals[key] = value; }
    std::string get(const std::string& key, const std::string& def = "") const {
        auto it = m_vals.find(key);
        return it == m_vals.end() ? def : it->second;
    }

    bool load(const std::string& /*path*/) { return false; }
    bool save(const std::string& /*path*/) const { return false; }

    void setEnabled(bool e) { m_enabled = e; }
    bool enabled() const { return m_enabled; }

private:
    bool m_enabled = false;
    std::unordered_map<std::string, std::string> m_vals;
};

} // namespace vr
} // namespace devices
} // namespace engine
} // namespace ks
