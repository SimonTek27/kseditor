#pragma once
#include <string>
namespace ks { namespace engine { namespace config {
class EditorConfig {
public:
    static EditorConfig& instance() { static EditorConfig s; return s; }
    bool load(const std::string&) { return true; }
    void save(const std::string&) {}
};
}}} // namespace
