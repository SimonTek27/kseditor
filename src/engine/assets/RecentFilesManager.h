#pragma once
#include <string>
#include <vector>
namespace ks { namespace engine { namespace assets {
class RecentFilesManager {
public:
    static RecentFilesManager& instance() { static RecentFilesManager s; return s; }
    void add(const std::string& p) { m_files.push_back(p); }
    std::vector<std::string> files() const { return m_files; }
private:
    std::vector<std::string> m_files;
};
}}} // namespace
