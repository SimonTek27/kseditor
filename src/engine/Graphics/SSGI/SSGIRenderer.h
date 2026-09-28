#pragma once
/** Qt-free SSGI facade — real path uses NativeRenderer / compute. */
#include "../GfxTypes.h"
#include <string>

namespace ks {
namespace engine {
namespace graphics {

class SSGIRenderer {
public:
    static SSGIRenderer& instance() { static SSGIRenderer s; return s; }

    bool initialize() { m_ok = true; return true; }
    void shutdown() { m_ok = false; }
    bool isReady() const { return m_ok; }

    void setEnabled(bool e) { m_enabled = e; }
    bool enabled() const { return m_enabled; }

    void setIntensity(float v) { m_intensity = v; }
    float intensity() const { return m_intensity; }

    void setViewProjection(const Mat4& /*vp*/) {}
    void render() {}

private:
    bool m_ok = false;
    bool m_enabled = false;
    float m_intensity = 1.0f;
};

} // namespace graphics
} // namespace engine
} // namespace ks
