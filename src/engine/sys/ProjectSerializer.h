#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class ProjectSerializer {
public:
    static ProjectSerializer& instance() { static ProjectSerializer s; return s; }
    bool save(const std::string&) { return false; }
    bool load(const std::string&) { return false; }
};
}}} // namespace
