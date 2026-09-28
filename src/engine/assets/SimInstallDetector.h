#pragma once
#include <string>
#include <vector>

namespace ks {
namespace engine {
namespace assets {

/** Generic install-path probe (not tied to a specific commercial title in core). */
class SimInstallDetector {
public:
    static SimInstallDetector& instance() { static SimInstallDetector s; return s; }
    std::vector<std::string> detectPaths() const { return m_paths; }
    void addSearchRoot(const std::string& p) { m_paths.push_back(p); }

private:
    std::vector<std::string> m_paths;
};

} // namespace assets
} // namespace engine
} // namespace ks
