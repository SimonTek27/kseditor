#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef ENGINESIMHOOK_EXPORTS
#define ENGINESIMHOOK_API __declspec(dllexport)
#else
#define ENGINESIMHOOK_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

ENGINESIMHOOK_API void __stdcall EngineSimHook_Initialize(void);
ENGINESIMHOOK_API void __stdcall EngineSimHook_Shutdown(void);
ENGINESIMHOOK_API float __stdcall EngineSimHook_GetRPM(void);
ENGINESIMHOOK_API float __stdcall EngineSimHook_GetThrottle(void);
ENGINESIMHOOK_API float __stdcall EngineSimHook_GetLoad(void);
ENGINESIMHOOK_API float __stdcall EngineSimHook_GetSpeed(void);
ENGINESIMHOOK_API void __stdcall EngineSimHook_SetThrottle(float throttle);
ENGINESIMHOOK_API void __stdcall EngineSimHook_SetTargetRPM(float rpm);
ENGINESIMHOOK_API void __stdcall EngineSimHook_ClearTargetRPM(void);
ENGINESIMHOOK_API int __stdcall EngineSimHook_IsConnected(void);
ENGINESIMHOOK_API const char* __stdcall EngineSimHook_GetGameVersion(void);

#ifdef __cplusplus
}
#endif
