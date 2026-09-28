#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class MetadataSystem {
public:
    static MetadataSystem& instance() { static MetadataSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
