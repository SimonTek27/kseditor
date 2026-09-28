#pragma once
#include <string>
namespace ks { namespace engine { namespace tools {
class FileDiffEngine {
public:
    static FileDiffEngine& instance() { static FileDiffEngine s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
