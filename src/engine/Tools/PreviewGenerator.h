#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class PreviewGenerator {
public:
    static PreviewGenerator& instance() { static PreviewGenerator s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
