#pragma once
/** Qt-free stub (was QML bridge). */
#include <string>
namespace ks { namespace engine { namespace material {
class TexturePaintQmlBridge {
public:
    static TexturePaintQmlBridge& instance() { static TexturePaintQmlBridge s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
