#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class LODGenerator {
public:
    static LODGenerator& instance() { static LODGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
