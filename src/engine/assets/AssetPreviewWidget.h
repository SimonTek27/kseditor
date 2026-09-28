#pragma once
/** Qt-free stub (was QWidget). */
#include <string>
namespace ks { namespace engine { namespace assets {
class AssetPreviewWidget {
public:
    static AssetPreviewWidget& instance() { static AssetPreviewWidget s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
};
}}} // namespace
