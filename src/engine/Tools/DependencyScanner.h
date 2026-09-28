#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class DependencyScanner {
public:
    static DependencyScanner& instance() { static DependencyScanner s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
