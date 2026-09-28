#pragma once
/** Qt-free video mode query facade (no QObject). */
#include <string>
#include <vector>
#include <cstdint>

namespace ks {
namespace engine {
namespace graphics {

struct VideoMode {
    int width = 0;
    int height = 0;
    int refreshRate = 0;
};

class VideoModesWrapper {
public:
    static VideoModesWrapper& instance() {
        static VideoModesWrapper s;
        return s;
    }

    bool initialize() { m_initialized = true; return true; }
    void shutdown() { m_initialized = false; m_modes.clear(); }
    bool isInitialized() const { return m_initialized; }

    const std::vector<VideoMode>& modes() const { return m_modes; }
    void setModes(std::vector<VideoMode> modes) { m_modes = std::move(modes); }

private:
    bool m_initialized = false;
    std::vector<VideoMode> m_modes;
};

} // namespace graphics
} // namespace engine
} // namespace ks
