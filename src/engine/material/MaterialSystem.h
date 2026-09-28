#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class MaterialSystem {
public:
    static MaterialSystem& instance() { static MaterialSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
