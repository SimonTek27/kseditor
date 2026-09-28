#pragma once
#include <string>
namespace ks { namespace engine { namespace mesh {
class ModifierSystem {
public:
    static ModifierSystem& instance() { static ModifierSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
