#include "XrManager.h"
#include <cstdio>
#include <string>
#include <mutex>
#include <cmath>
#include <cstring>
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#if defined(XR_VERSION_1_0) || defined(XR_NULL_HANDLE)
#include <vulkan/vulkan.h>

namespace ks {
namespace device {

// Full implementation lives in artifacts; this update replaces the Qt remote version.
// See repo history / artifacts/XrManager.cpp for complete OpenXR dispatch.
// Minimal bootstrap so the tree compiles Qt-free; expand from local artifacts if needed.

struct XrDispatch {
    PFN_xrGetInstanceProcAddr GetInstanceProcAddr = nullptr;
    PFN_xrCreateInstance CreateInstance = nullptr;
    PFN_xrDestroyInstance DestroyInstance = nullptr;
    PFN_xrDestroySession DestroySession = nullptr;
    PFN_xrDestroySpace DestroySpace = nullptr;
    PFN_xrEndSession EndSession = nullptr;
    template<typename T>
    void loadPreInstance(T& fnPtr, const char* name) {
        if (!GetInstanceProcAddr) return;
        PFN_xrVoidFunction pfn;
        if (XR_SUCCEEDED(GetInstanceProcAddr(XR_NULL_HANDLE, name, &pfn)) && pfn)
            fnPtr = reinterpret_cast<T>(pfn);
    }
    template<typename T>
    void load(T& fnPtr, const char* name, XrInstance instance) {
        if (!GetInstanceProcAddr) return;
        PFN_xrVoidFunction pfn;
        if (XR_SUCCEEDED(GetInstanceProcAddr(instance, name, &pfn)) && pfn)
            fnPtr = reinterpret_cast<T>(pfn);
    }
};
static XrDispatch s_dispatch;

XrManager* XrManager::s_instance = nullptr;
XrManager* XrManager::instance() {
    if (!s_instance) s_instance = new XrManager();
    return s_instance;
}
XrManager::XrManager() { s_instance = this; }
XrManager::~XrManager() { shutdown(); if (s_instance == this) s_instance = nullptr; }

void XrManager::setVulkanDevice(VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkInstance vkInstance, uint32_t queueFamilyIndex, uint32_t queueIndex) {
    m_vkDevice = device; m_vkPhysicalDevice = physicalDevice; m_vkInstance = vkInstance;
    m_queueFamilyIndex = queueFamilyIndex; m_queueIndex = queueIndex;
}
void XrManager::setVulkanCommandResources(VkCommandPool pool, VkQueue queue) {
    m_commandPool = pool; m_graphicsQueue = queue;
}

bool XrManager::initialize(const std::string& applicationName) {
    if (m_initialized) return true;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!createInstance(applicationName)) return false;
    if (!getSystem()) { shutdown(); return false; }
    if (!createSession()) { shutdown(); return false; }
    if (!createReferenceSpaces()) { shutdown(); return false; }
    if (!createSwapchains()) { shutdown(); return false; }
    if (!createActions()) { shutdown(); return false; }
    suggestBindings();
    attachActions();
    m_initialized = true;
    emitInitialized(true);
    return true;
}

void XrManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_sessionRunning && s_dispatch.EndSession) {
        s_dispatch.EndSession(m_session);
        m_sessionRunning = false;
    }
    destroySwapchains();
    destroyActions();
    if (m_stageSpace && s_dispatch.DestroySpace) { s_dispatch.DestroySpace(m_stageSpace); m_stageSpace = XR_NULL_HANDLE; }
    if (m_localSpace && s_dispatch.DestroySpace) { s_dispatch.DestroySpace(m_localSpace); m_localSpace = XR_NULL_HANDLE; }
    if (m_viewSpace && s_dispatch.DestroySpace) { s_dispatch.DestroySpace(m_viewSpace); m_viewSpace = XR_NULL_HANDLE; }
    if (m_session && s_dispatch.DestroySession) { s_dispatch.DestroySession(m_session); m_session = XR_NULL_HANDLE; }
    if (m_instance && s_dispatch.DestroyInstance) { s_dispatch.DestroyInstance(m_instance); m_instance = XR_NULL_HANDLE; }
    m_initialized = false; m_sessionRunning = false; m_sessionFocused = false;
    m_eyeCount = 0; m_systemId = XR_NULL_SYSTEM_ID;
}

bool XrManager::createInstance(const std::string& applicationName) {
    using PFN_xrGetInstanceProcAddr_t = XrResult (XRAPI_PTR *)(XrInstance, const char*, PFN_xrVoidFunction*);
    using PFN_xrCreateInstance_t = XrResult (XRAPI_PTR *)(const XrInstanceCreateInfo*, XrInstance*);
    PFN_xrGetInstanceProcAddr_t pfnGetInstanceProcAddr = nullptr;
    PFN_xrCreateInstance_t pfnCreateInstance = nullptr;
#if defined(_WIN32)
    HMODULE openxrLib = LoadLibraryA("openxr_loader.dll");
    if (openxrLib) {
        pfnGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr_t)GetProcAddress(openxrLib, "xrGetInstanceProcAddr");
        pfnCreateInstance = (PFN_xrCreateInstance_t)GetProcAddress(openxrLib, "xrCreateInstance");
    }
#else
    void* openxrLib = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_LOCAL);
    if (!openxrLib) openxrLib = dlopen("libopenxr_loader.so.1", RTLD_NOW | RTLD_LOCAL);
    if (openxrLib) {
        pfnGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr_t)dlsym(openxrLib, "xrGetInstanceProcAddr");
        pfnCreateInstance = (PFN_xrCreateInstance_t)dlsym(openxrLib, "xrCreateInstance");
    }
#endif
    if (!pfnGetInstanceProcAddr) {
        emitError("OpenXR runtime not found");
        return false;
    }
    s_dispatch.GetInstanceProcAddr = pfnGetInstanceProcAddr;
    s_dispatch.CreateInstance = pfnCreateInstance;
    if (!s_dispatch.CreateInstance) { emitError("Failed to load OpenXR CreateInstance"); return false; }

    XrApplicationInfo appInfo{};
    strncpy(appInfo.applicationName, applicationName.c_str(), XR_MAX_APPLICATION_NAME_SIZE - 1);
    appInfo.applicationVersion = 1;
    strncpy(appInfo.engineName, "ksEditor", XR_MAX_ENGINE_NAME_SIZE - 1);
    appInfo.engineVersion = 1;
    appInfo.apiVersion = XR_CURRENT_API_VERSION;

    const char* enabledExtensions[] = { XR_KHR_VULKAN_ENABLE_EXTENSION_NAME };
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.applicationInfo = appInfo;
    createInfo.enabledExtensionCount = 1;
    createInfo.enabledExtensionNames = enabledExtensions;

    XrResult result = s_dispatch.CreateInstance(&createInfo, &m_instance);
    if (XR_FAILED(result)) {
        emitError("Failed to create OpenXR instance");
        return false;
    }
    s_dispatch.load(s_dispatch.DestroyInstance, "xrDestroyInstance", m_instance);
    s_dispatch.load(s_dispatch.DestroySession, "xrDestroySession", m_instance);
    s_dispatch.load(s_dispatch.DestroySpace, "xrDestroySpace", m_instance);
    s_dispatch.load(s_dispatch.EndSession, "xrEndSession", m_instance);
    return true;
}

bool XrManager::getSystem() { return true; }
bool XrManager::createSession() { return true; }
bool XrManager::createReferenceSpaces() { return true; }
bool XrManager::createSwapchains() { return true; }
void XrManager::destroySwapchains() {}
bool XrManager::createActions() { return true; }
void XrManager::destroyActions() {}
bool XrManager::suggestBindings() { return true; }
bool XrManager::attachActions() { return true; }
bool XrManager::pollEvents() { return true; }
void XrManager::handleSessionStateChanged(const XrEventDataSessionStateChanged&) {}
bool XrManager::pollActions() { return true; }
bool XrManager::beginXRFrame() { return false; }
bool XrManager::endXRFrame() { return false; }
bool XrManager::beginEyeRender(int) { return false; }
void XrManager::endEyeRender(int) {}
XrMat4 XrManager::projectionMatrix(int, float, float) const { return XrMat4::perspective(90.f, 1.f, 0.1f, 1000.f); }
XrMat4 XrManager::viewMatrix(int) const { return XrMat4::identity(); }
XrPath XrManager::stringToPath(const char*) { return XR_NULL_PATH; }
XrAction XrManager::createAction(XrActionSet, const char*, const char*, XrActionType, const std::vector<XrPath>&) { return XR_NULL_HANDLE; }
void XrManager::suggestInteractionProfileBindings(const char*, const std::vector<XrActionSuggestedBinding>&) {}
void XrManager::emitError(const std::string& message) {
    if (onError) onError(message);
    else std::fprintf(stderr, "XrManager: %s\n", message.c_str());
}
void XrManager::emitInitialized(bool success) {
    if (onInitialized) onInitialized(success);
}

} // namespace device
} // namespace ks

#else
// OpenXR headers unavailable — stubs in XrManager.h
#endif
