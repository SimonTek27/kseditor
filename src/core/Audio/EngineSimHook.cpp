#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <tlhelp32.h>

#define ENGINESIMHOOK_EXPORTS
#include "EngineSimHook.h"

#if HAS_DETOURS
#include <detours.h>
#endif

static bool s_initialized = false;
static bool s_connected = false;
static float s_currentRPM = 0.0f;
static float s_currentThrottle = 0.0f;
static float s_currentLoad = 0.0f;
static float s_currentSpeed = 0.0f;
static float s_targetRPM = 0.0f;
static bool s_hasTargetRPM = false;
static HANDLE s_EngineSimProcess = NULL;
static const char* s_gameVersion = "Unknown";

static volatile long s_detourRefCount = 0;

typedef float(*GetEngineRPM_t)(void*);
typedef float(*GetThrottle_t)(void*);
typedef float(*GetLoad_t)(void*);
typedef void(*SetThrottle_t)(void*, float);

static GetEngineRPM_t s_originalGetRPM = nullptr;
static GetThrottle_t s_originalGetThrottle = nullptr;
static GetLoad_t s_originalGetLoad = nullptr;
static SetThrottle_t s_originalSetThrottle = nullptr;

static void* s_engineInstance = nullptr;

static float HookGetRPM(void* instance) {
    if (s_hasTargetRPM && s_originalGetRPM) {
        float currentRPM = s_originalGetRPM(instance);
        s_currentRPM = currentRPM;
        
        if (s_originalSetThrottle && instance) {
            float error = s_targetRPM - currentRPM;
            float throttle = s_currentThrottle;
            
            if (std::abs(error) > 10.0f) {
                float kp = 0.0008f;
                float ki = 0.0001f;
                static float integral = 0.0f;
                integral += error * 0.016f;
                if (integral < -0.3f) integral = -0.3f;
                else if (integral > 0.3f) integral = 0.3f;
                throttle = 0.5f + kp * error + ki * integral;
                if (throttle < 0.0f) throttle = 0.0f;
                else if (throttle > 1.0f) throttle = 1.0f;
                s_originalSetThrottle(instance, throttle);
                s_currentThrottle = throttle;
            }
        }
        return currentRPM;
    }
    
    if (s_originalGetRPM && instance) {
        s_currentRPM = s_originalGetRPM(instance);
    }
    return s_currentRPM;
}

static float HookGetThrottle(void* instance) {
    if (s_originalGetThrottle && instance) {
        s_currentThrottle = s_originalGetThrottle(instance);
    }
    return s_currentThrottle;
}

static float HookGetLoad(void* instance) {
    if (s_originalGetLoad && instance) {
        s_currentLoad = s_originalGetLoad(instance);
    }
    return s_currentLoad;
}

static void HookSetThrottle(void* instance, float throttle) {
    s_currentThrottle = throttle;
}

static bool FindEngineSimProcess() {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (wcsstr(entry.szExeFile, L"engine-sim") != nullptr ||
                wcsstr(entry.szExeFile, L"engine-sim.exe") != nullptr ||
                wcsstr(entry.szExeFile, L"EngineSimulator") != nullptr) {
                s_EngineSimProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                found = (s_EngineSimProcess != NULL);
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return found;
}

extern "C" {

ENGINESIMHOOK_API void __stdcall EngineSimHook_Initialize(void) {
    if (s_initialized) return;
    
#if HAS_DETOURS
    DetourRestoreAfterLoad();
#endif
    
    s_initialized = true;
    s_connected = FindEngineSimProcess();
    
    if (s_connected) {
        s_gameVersion = "Engine Simulator";
    }
}

ENGINESIMHOOK_API void __stdcall EngineSimHook_Shutdown(void) {
    if (!s_initialized) return;
    
    if (s_EngineSimProcess) {
        CloseHandle(s_EngineSimProcess);
        s_EngineSimProcess = NULL;
    }
    
    s_connected = false;
    s_initialized = false;
    s_hasTargetRPM = false;
}

ENGINESIMHOOK_API float __stdcall EngineSimHook_GetRPM(void) {
    return s_currentRPM;
}

ENGINESIMHOOK_API float __stdcall EngineSimHook_GetThrottle(void) {
    return s_currentThrottle;
}

ENGINESIMHOOK_API float __stdcall EngineSimHook_GetLoad(void) {
    return s_currentLoad;
}

ENGINESIMHOOK_API float __stdcall EngineSimHook_GetSpeed(void) {
    return s_currentSpeed;
}

ENGINESIMHOOK_API void __stdcall EngineSimHook_SetThrottle(float throttle) {
    s_currentThrottle = throttle;
}

ENGINESIMHOOK_API void __stdcall EngineSimHook_SetTargetRPM(float rpm) {
    s_targetRPM = rpm;
    s_hasTargetRPM = true;
}

ENGINESIMHOOK_API void __stdcall EngineSimHook_ClearTargetRPM(void) {
    s_hasTargetRPM = false;
    s_targetRPM = 0.0f;
}

ENGINESIMHOOK_API int __stdcall EngineSimHook_IsConnected(void) {
    return s_connected ? 1 : 0;
}

ENGINESIMHOOK_API const char* __stdcall EngineSimHook_GetGameVersion(void) {
    return s_gameVersion;
}

}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        EngineSimHook_Shutdown();
        break;
    }
    return TRUE;
}
