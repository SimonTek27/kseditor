#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "simulator/SimulationLoop.h"
#include "simulator/CameraController.h"
#include "simulator/DashboardOverlay.h"
#include "simulator/TelemetryOverlay.h"
#include "simulator/GameMenuOverlay.h"
#include "simulator/InputManager.h"
#include "simulator/SetupGarage.h"
#include "simulator/NetworkManager.h"
#include "simulator/ShadowSystem.h"
#include "simulator/NativeRenderer.h"
#include "engine/physics/VehicleSimulator.h"
#include "devices/DeviceManager.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

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
    const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
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
    if (auto* sg = g_simulation->setupGarage(); sg && sg->isVisible()) {
        if (sg->handleKeyPress(vk)) return;
    }
    switch (vk) {
    case VK_ESCAPE: if (g_menu) g_menu->toggleVisible(); break;
    case 'W': case VK_UP: g_throttle = true; break;
    case 'S': case VK_DOWN: g_brake = true; break;
    case 'A': case VK_LEFT: g_steerLeft = true; break;
    case 'D': case VK_RIGHT: g_steerRight = true; break;
    case 'R': g_simulation->reset(); break;
    case VK_SPACE:
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) { g_simulation->stop(); }
        else { g_simulation->start(); }
        break;
    default: break;
    }
}

static void handleKeyUp(int vk) {
    switch (vk) {
    case 'W': case VK_UP: g_throttle = false; break;
    case 'S': case VK_DOWN: g_brake = false; break;
    case 'A': case VK_LEFT: g_steerLeft = false; break;
    case 'D': case VK_RIGHT: g_steerRight = false; break;
    default: break;
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN: handleKeyDown((int)wParam); return 0;
    case WM_KEYUP: handleKeyUp((int)wParam); return 0;
    case WM_DESTROY: g_running = false; PostQuitMessage(0); return 0;
    default: return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInstance = hInstance;
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS;
    RegisterClassExW(&wc);
    g_hWnd = CreateWindowExW(0, WINDOW_CLASS, L"ksim", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) return 1;
    ShowWindow(g_hWnd, nCmdShow);

    createVulkanInstance();
    createWin32Surface();

    g_simulation = std::make_unique<ks::sim::SimulationLoop>();
    g_simulation->initialize();
    g_simulation->startFeatureServices(false);

    g_menu = std::make_unique<ks::sim::GameMenuOverlay>();
    g_menu->onExitRequested = []() { g_running = false; PostMessageW(g_hWnd, WM_CLOSE, 0, 0); };
    g_menu->onStartDrivingRequested = []() {
        if (g_simulation) {
            g_simulation->beginSession(ks::sim::GameSessionMode::Race);
            g_simulation->start();
        }
        if (g_menu) g_menu->setVisible(false);
        printf("Driving started!\n");
    };
    g_menu->onStartSessionRequested = [](const std::string& modeLabel) {
        if (!g_simulation) return;
        auto mode = ks::sim::modeFromMenuEntry(modeLabel);
        g_simulation->beginSession(mode);
        g_simulation->start();
        if (g_menu) g_menu->setVisible(false);
        printf("Session started: %s\n", ks::sim::sessionModeName(mode));
    };
    g_menu->onLoadReplayRequested = []() {
        if (!g_simulation) return;
        const char* path = std::getenv("KS_REPLAY_FILE");
        std::string p = path && path[0] ? path : "replays/last.ksreplay";
        if (g_simulation->loadReplayFile(p)) {
            if (g_menu) g_menu->setVisible(false);
            g_simulation->start();
            printf("Replay loaded: %s\n", p.c_str());
        } else {
            printf("Load replay failed: %s (set KS_REPLAY_FILE)\n", p.c_str());
        }
    };
    g_menu->onHostServerRequested = []() {
        auto* net = g_simulation ? g_simulation->networkManager() : nullptr;
        if (!net) return;
        if (net->hostServer(40000, 8, "ksim Server", "track")) {
            g_simulation->features().announceHost("ksim Server", "track", 40000, 1, 8);
            g_simulation->startFeatureServices(true);
            if (g_menu) g_menu->setVisible(false);
            printf("Hosting on port 40000\n");
        }
    };
    g_menu->onOpenServerBrowserRequested = []() {
        if (!g_simulation) return;
        if (!g_simulation->features().discoveryStarted)
            g_simulation->startFeatureServices(false);
        g_simulation->features().discovery.queryLan();
        if (g_menu) g_menu->setVisible(false);
        printf("Server browser: LAN query sent\n");
    };
    g_menu->onDisconnectRequested = []() {
        auto* net = g_simulation ? g_simulation->networkManager() : nullptr;
        if (!net) return;
        net->disconnectFromServer();
        net->stopServer();
    };
    g_menu->setVisible(true);

    MSG msg = {};
    while (g_running) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) g_running = false;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        pollInput();
        if (g_simulation) g_simulation->tick();
        if (g_menu) g_menu->render(1280, 720);
    }

    g_menu.reset();
    g_simulation.reset();
    if (g_surface) vkDestroySurfaceKHR(g_vkInstance, g_surface, nullptr);
    if (g_vkInstance) vkDestroyInstance(g_vkInstance, nullptr);
    return 0;
}
