#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "simulator/SimulationLoop.h"
#include "simulator/CameraController.h"
#include "simulator/DashboardOverlay.h"
#include "simulator/GameMenuOverlay.h"
#include "simulator/NetworkManager.h"
#include "simulator/ShadowSystem.h"
#include "simulator/NativeRenderer.h"
#include "devices/DeviceManager.h"
#include <cstdio>
#include <memory>

static const char* SHADER_DIR = "shaders";

static HINSTANCE g_hInstance = nullptr;
static HWND g_hWnd = nullptr;
static VkInstance g_vkInstance = VK_NULL_HANDLE;
static VkSurfaceKHR g_surface = VK_NULL_HANDLE;
static ks::sim::NativeRenderer* g_nativeRenderer = nullptr;
static ks::sim::CascadedShadowMap g_shadowMap;
static std::unique_ptr<ks::sim::SimulationLoop> g_simulation;
static std::unique_ptr<ks::sim::GameMenuOverlay> g_menu;

static bool g_throttle = false, g_brake = false;
static bool g_steerLeft = false, g_steerRight = false;
static bool g_running = true;

static const wchar_t* WINDOW_CLASS = L"KsEditorSimWindow";

static bool createVulkanInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ksEditor Simulator";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ksEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    return vkCreateInstance(&createInfo, nullptr, &g_vkInstance) == VK_SUCCESS;
}

static bool createWin32Surface() {
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = g_hInstance;
    createInfo.hwnd = g_hWnd;
    return vkCreateWin32SurfaceKHR(g_vkInstance, &createInfo, nullptr, &g_surface) == VK_SUCCESS;
}

static void pollInput() {
    if (!g_simulation || !g_simulation->isRunning()) return;
    auto* v = g_simulation->vehicle();
    if (!v) return;
    v->setThrottle(g_throttle ? 1.0 : 0.0);
    v->setBrake(g_brake ? 1.0 : 0.0);
    double steer = 0;
    if (g_steerLeft) steer -= 1.0;
    if (g_steerRight) steer += 1.0;
    v->setSteering(steer);
}

static void handleKeyDown(int vk) {
    if (!g_simulation) return;
    if (g_menu && g_menu->isVisible()) { g_menu->handleKeyPress(vk); return; }
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    switch (vk) {
    case VK_ESCAPE: if (g_menu) g_menu->toggleVisible(); break;
    case 'W': case VK_UP:
        g_throttle = true;
        if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('W');
        break;
    case 'S': case VK_DOWN:
        g_brake = true;
        if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('S');
        break;
    case 'A': case VK_LEFT:
        g_steerLeft = true;
        if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('A');
        break;
    case 'D': case VK_RIGHT:
        g_steerRight = true;
        if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('D');
        break;
    case 'E': if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('E'); break;
    case 'Q': if (g_simulation->inputManager()) g_simulation->inputManager()->setKeyDown('Q'); break;
    case 'R': g_simulation->reset(); break;
    case VK_F5:
        if (shift) g_simulation->stop();
        else g_simulation->start();
        break;
    default: break;
    }
}

static void handleKeyUp(int vk) {
    if (g_menu && g_menu->isVisible()) return;
    switch (vk) {
    case 'W': case VK_UP:
        g_throttle = false;
        if (g_simulation && g_simulation->inputManager()) g_simulation->inputManager()->setKeyUp('W');
        break;
    case 'S': case VK_DOWN:
        g_brake = false;
        if (g_simulation && g_simulation->inputManager()) g_simulation->inputManager()->setKeyUp('S');
        break;
    case 'A': case VK_LEFT:
        g_steerLeft = false;
        if (g_simulation && g_simulation->inputManager()) g_simulation->inputManager()->setKeyUp('A');
        break;
    case 'D': case VK_RIGHT:
        g_steerRight = false;
        if (g_simulation && g_simulation->inputManager()) g_simulation->inputManager()->setKeyUp('D');
        break;
    default: break;
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN: handleKeyDown((int)wParam); return 0;
    case WM_KEYUP: handleKeyUp((int)wParam); return 0;
    case WM_SIZE: {
        if (g_nativeRenderer && g_nativeRenderer->isInitialized() && g_surface) {
            int w = LOWORD(lParam), h = HIWORD(lParam);
            if (w > 0 && h > 0) g_nativeRenderer->createSwapChain(g_surface, w, h);
        }
        return 0;
    }
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void initWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, WINDOW_CLASS, L"ksEditor Simulator",
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080,
                             nullptr, nullptr, g_hInstance, nullptr);
    ShowWindow(g_hWnd, SW_SHOW);
}

static void initVulkanAndSimulation() {
    if (!createVulkanInstance() || !createWin32Surface()) return;
    g_nativeRenderer = new ks::sim::NativeRenderer();
    if (!g_nativeRenderer->createDevice(g_vkInstance, g_surface)) {
        delete g_nativeRenderer;
        g_nativeRenderer = nullptr;
        return;
    }
    RECT rc; GetClientRect(g_hWnd, &rc);
    g_nativeRenderer->createSwapChain(g_surface, rc.right - rc.left, rc.bottom - rc.top);
    g_nativeRenderer->loadPipelines(SHADER_DIR);
    g_shadowMap.initialize(g_nativeRenderer->physicalDevice(), g_nativeRenderer->device(),
                           g_nativeRenderer->commandPool(), g_nativeRenderer->graphicsQueue(),
                           SHADER_DIR);
    g_nativeRenderer->attachShadowMap(&g_shadowMap);

    g_simulation = std::make_unique<ks::sim::SimulationLoop>();
    g_simulation->setVulkanRenderer(g_nativeRenderer);
    g_simulation->initialize();
    g_menu = std::make_unique<ks::sim::GameMenuOverlay>();
    g_menu->onExitRequested = []() {
        g_running = false;
        PostMessageW(g_hWnd, WM_CLOSE, 0, 0);
    };
    g_menu->onStartDrivingRequested = []() { g_simulation->start(); };
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;
    initWindow();
    initVulkanAndSimulation();
    printf("ksEditor Simulator (Qt-free Win32+Vulkan)\n");

    LARGE_INTEGER freq, lastTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    while (g_running) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running) break;
        pollInput();
        if (g_simulation && g_simulation->isRunning() && g_menu && !g_menu->isVisible())
            g_simulation->tick();
        Sleep(1);
    }

    g_shadowMap.shutdown();
    delete g_nativeRenderer;
    if (g_surface) vkDestroySurfaceKHR(g_vkInstance, g_surface, nullptr);
    if (g_vkInstance) vkDestroyInstance(g_vkInstance, nullptr);
    return 0;
}
