#pragma once
#include <string>
#include <vector>

namespace ks {
namespace engine {
namespace assets {

class ProjectTemplates {
public:
    static ProjectTemplates& instance() { static ProjectTemplates s; return s; }
    std::vector<std::string> list() const { return {}; }
    bool createFromTemplate(const std::string& /*name*/, const std::string& /*dest*/) {
        return false;
    }
};

} // namespace assets
} // namespace engine
} // namespace ks
