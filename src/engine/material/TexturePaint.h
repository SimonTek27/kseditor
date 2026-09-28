#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class TexturePaint {
public:
    static TexturePaint& instance() { static TexturePaint s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
