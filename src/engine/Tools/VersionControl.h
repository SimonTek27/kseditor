#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class VersionControl {
public:
    static VersionControl& instance() { static VersionControl s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
