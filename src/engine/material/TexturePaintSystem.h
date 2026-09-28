#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class TexturePaintSystem {
public:
    static TexturePaintSystem& instance() { static TexturePaintSystem s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
