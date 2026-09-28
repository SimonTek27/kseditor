#pragma once
#include <string>
namespace ks { namespace engine { namespace graphics {
class CascadedShadowMaps {
public:
    static CascadedShadowMaps& instance() { static CascadedShadowMaps s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    std::string name() const { return "CascadedShadowMaps"; }
};
}}} // namespace
