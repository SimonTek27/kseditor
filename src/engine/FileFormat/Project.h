#pragma once
#include <string>
namespace ks { namespace engine { namespace fileformat {
class Project {
public:
    static Project& instance() { static Project s; return s; }
    bool open(const std::string& path) { m_path = path; return true; }
    void close() { m_path.clear(); }
    const std::string& path() const { return m_path; }
private:
    std::string m_path;
};
}}} // namespace
