#pragma once

#include <cstdint>

namespace ks::engine::graphics {

// ============================================================================
// Video Modes - Direct declarations (linked via ksengineVideoModes.lib)
// ============================================================================

struct VideoModeInfo {
    int width = 0;
    int height = 0;
    int refreshRate = 0;
    int colorDepth = 32;
    bool fullscreen = false;
    bool vsync = true;
    int adapterIndex = 0;
    int outputIndex = 0;
    char adapterName[128] = {};
    char outputName[128] = {};
};

struct MonitorInfo {
    int index = 0;
    char name[128] = {};
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
    bool primary = false;
};

extern "C" {

// ksengineVideoModes.dll exports
int acInitVideoModes();
int acGetVideoMode(int modeIndex, VideoModeInfo* info);

} // extern "C"

} // namespace ks::engine::graphics
