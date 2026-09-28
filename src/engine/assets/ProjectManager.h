#pragma once
#include <string>
namespace ks { namespace engine { namespace assets {
class ProjectManager {
public:
    static ProjectManager& instance() { static ProjectManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool open(const std::string&) { return false; }
};
}}} // namespace
