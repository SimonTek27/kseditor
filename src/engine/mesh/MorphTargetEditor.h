#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class MorphTargetEditor {
public:
    static MorphTargetEditor& instance() { static MorphTargetEditor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
