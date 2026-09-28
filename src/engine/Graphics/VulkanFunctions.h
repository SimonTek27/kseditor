#pragma once
/** Vulkan function loader via platform dynload (no Qt). */
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#else
#include <dlfcn.h>
#endif

#include <vulkan/vulkan.h>
#include <cstdio>

namespace ks {

struct VulkanFunctionTable {
    PFN_vkGetInstanceProcAddr getInstanceProcAddr = nullptr;
    PFN_vkCreateInstance createInstance = nullptr;
    PFN_vkDestroyInstance destroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices enumeratePhysicalDevices = nullptr;
    PFN_vkGetPhysicalDeviceProperties getPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties getPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkCreateDevice createDevice = nullptr;
    PFN_vkDestroyDevice destroyDevice = nullptr;
    PFN_vkGetDeviceQueue getDeviceQueue = nullptr;
    PFN_vkCreateCommandPool createCommandPool = nullptr;
    PFN_vkDestroyCommandPool destroyCommandPool = nullptr;
    PFN_vkAllocateCommandBuffers allocateCommandBuffers = nullptr;
    PFN_vkFreeCommandBuffers freeCommandBuffers = nullptr;
    PFN_vkCreateFence createFence = nullptr;
    PFN_vkDestroyFence destroyFence = nullptr;
    PFN_vkCreateSemaphore createSemaphore = nullptr;
    PFN_vkDestroySemaphore destroySemaphore = nullptr;

    bool load() {
#ifdef _WIN32
        HMODULE lib = LoadLibraryA("vulkan-1.dll");
        if (!lib) {
            std::fprintf(stderr, "VulkanFunctions: vulkan-1.dll not found\n");
            return false;
        }
        getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            GetProcAddress(lib, "vkGetInstanceProcAddr"));
#else
        void* lib = dlopen("libvulkan.so.1", RTLD_NOW);
        if (!lib) lib = dlopen("libvulkan.so", RTLD_NOW);
        if (!lib) {
            std::fprintf(stderr, "VulkanFunctions: libvulkan not found\n");
            return false;
        }
        getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            dlsym(lib, "vkGetInstanceProcAddr"));
#endif
        if (!getInstanceProcAddr) return false;

        auto loadFn = [&](const char* name) -> PFN_vkVoidFunction {
            return getInstanceProcAddr(VK_NULL_HANDLE, name);
        };
        createInstance = reinterpret_cast<PFN_vkCreateInstance>(loadFn("vkCreateInstance"));
        destroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(loadFn("vkDestroyInstance"));
        enumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(loadFn("vkEnumeratePhysicalDevices"));
        getPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(loadFn("vkGetPhysicalDeviceProperties"));
        getPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(loadFn("vkGetPhysicalDeviceMemoryProperties"));
        getPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(loadFn("vkGetPhysicalDeviceQueueFamilyProperties"));
        createDevice = reinterpret_cast<PFN_vkCreateDevice>(loadFn("vkCreateDevice"));
        destroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(loadFn("vkDestroyDevice"));
        getDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(loadFn("vkGetDeviceQueue"));
        return createInstance != nullptr;
    }
};

} // namespace ks
