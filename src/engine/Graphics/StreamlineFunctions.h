#pragma once

#include <cstdint>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

namespace ks {

// ============================================================================
// NVIDIA Streamline SDK - Minimal type definitions
// ============================================================================

using sl::Result = int;
constexpr int kSLResultSuccess = 0;
constexpr int kSLResultErrorFailed = -1;
constexpr int kSLResultErrorUnsupported = -2;

enum class SLFeature : uint32_t {
    DLSS = 0,
    DLSS_G = 1,
    DLSS_RR = 2,
    Reflex = 3,
    NIS = 4,
    ImGui = 5,
    Count = 6
};

enum class SLResourceType : uint32_t {
    Buffer = 0,
    Texture1D = 1,
    Texture2D = 2,
    Texture3D = 3
};

enum class SLResourceState : uint32_t {
    Common = 0,
    CopyDest = 1,
    CopySource = 2,
    RenderTarget = 3
};

enum class SLDLSSMode : uint32_t {
    Off = 0,
    MaxPerformance = 1,
    Balanced = 2,
    MaxQuality = 3,
    UltraPerformance = 4,
    UltraQuality = 5,
    DLAA = 6
};

enum class SLReflexMode : uint32_t {
    Off = 0,
    Enabled = 1,
    EnabledPlusBoost = 2
};

enum class SLNISMode : uint32_t {
    Off = 0,
    On = 1
};

struct SLResourceVulkan {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t mipLevel = 0;
    uint32_t arraySlice = 0;
};

struct SLResource {
    SLResourceType type = SLResourceType::Texture2D;
    SLResourceState state = SLResourceState::Common;
    union {
        SLResourceVulkan vulkan;
    };
};

struct SLDLSSRecommendSettings {
    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;
    float optimalSharpness = 0.0f;
};

// ============================================================================
// Streamline core function (linked via sl.common.lib)
// sl.common.dll only exports slGetPluginFunction, which loads sub-DLLs
// ============================================================================

extern "C" {

// sl.common.dll - plugin loader (only export)
using SLGetPluginFunctionFn = void* (*)(const char* pluginName, const char* functionName);

} // extern "C"

// ============================================================================
// Streamline function table - resolved at runtime from sub-DLLs
// ============================================================================

// Core
using SLInitFn = int(*)(void* initParams);
using SLShutdownFn = int(*)();
using SLSetFeatureFn = int(*)(uint32_t feature, const void* params, uint32_t numTags, const void* tags);
using SLUnsetFeatureFn = int(*)(uint32_t feature, uint32_t numTags, const void* tags);
using SLEvaluateFeatureFn = int(*)(uint32_t feature, uint32_t frameIndex, const void* params, uint32_t numTags, const void* tags);
using SLGetFeatureRequirementsFn = int(*)(uint32_t feature, void* requirements);

// Feature-specific
using SLGetDLSSSettingsFn = int(*)(int quality, SLDLSSRecommendSettings* settings);
using SLSetDLSSModeFn = int(*)(int mode);
using SLGetDLSSStateFn = int(*)(void* state);
using SLGetReflexStateFn = int(*)(void* state);
using SLSetReflexModeFn = int(*)(int mode);
using SLGetNISStateFn = int(*)(void* state);
using SLSetNISModeFn = int(*)(int mode);

struct StreamlineFunctionTable {
    SLInitFn init = nullptr;
    SLShutdownFn shutdown = nullptr;
    SLSetFeatureFn setFeature = nullptr;
    SLUnsetFeatureFn unsetFeature = nullptr;
    SLEvaluateFeatureFn evaluateFeature = nullptr;
    SLGetFeatureRequirementsFn getFeatureRequirements = nullptr;

    // DLSS
    SLGetDLSSSettingsFn getDLSSSettings = nullptr;
    SLSetDLSSModeFn setDLSSMode = nullptr;
    SLGetDLSSStateFn getDLSSState = nullptr;

    // Reflex
    SLGetReflexStateFn getReflexState = nullptr;
    SLSetReflexModeFn setReflexMode = nullptr;

    // NIS
    SLGetNISStateFn getNISState = nullptr;
    SLSetNISModeFn setNISMode = nullptr;

    bool loaded = false;
    bool attempted = false;
};

StreamlineFunctionTable g_sl;

inline bool loadStreamlineLibrary() {
    if (g_sl.attempted) return g_sl.loaded;
    g_sl.attempted = true;

    // sl.common.dll exports only slGetPluginFunction
    // Use it to resolve functions from sl.interposer.dll (loaded by Streamline at runtime)
    SLGetPluginFunctionFn getPluginFn = nullptr;

    // Try to get the function from sl.common.dll
    // The actual resolution happens through Streamline's internal plugin loading
    // For now, mark as not loaded - StreamlineIntegration handles the full initialization
    qDebug() << "Streamline: slGetPluginFunction available via sl.common.lib";
    qDebug() << "Streamline: Full initialization delegated to StreamlineIntegration";

    g_sl.loaded = false; // Will be loaded by StreamlineIntegration
    return false;
}

} // namespace ks
