#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class ExternalToolManager {
public:
    static ExternalToolManager& instance() { static ExternalToolManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
