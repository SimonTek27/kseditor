#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class DecalSystem {
public:
    static DecalSystem& instance() { static DecalSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
