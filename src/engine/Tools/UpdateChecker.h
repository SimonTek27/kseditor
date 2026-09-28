#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class UpdateChecker {
public:
    static UpdateChecker& instance() { static UpdateChecker s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
