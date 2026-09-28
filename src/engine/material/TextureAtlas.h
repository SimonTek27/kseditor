#pragma once
#include <string>
namespace ks { namespace engine { namespace material {
class TextureAtlas {
public:
    static TextureAtlas& instance() { static TextureAtlas s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
