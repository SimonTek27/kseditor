#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class TemplateManager {
public:
    static TemplateManager& instance() { static TemplateManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
