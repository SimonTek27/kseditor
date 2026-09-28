#pragma once
#include <string>

namespace ks {
namespace engine {
namespace assets {

class ProjectBuilder {
public:
    static ProjectBuilder& instance() { static ProjectBuilder s; return s; }
    bool build(const std::string& /*projectPath*/) { return false; }
};

} // namespace assets
} // namespace engine
} // namespace ks
