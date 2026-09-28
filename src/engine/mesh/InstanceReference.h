#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class InstanceReference {
public:
    static InstanceReference& instance() { static InstanceReference s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
