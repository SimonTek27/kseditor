# KSEngine / SimulatorApp Qt-free

**Updated:** 2026-09-28

## Clean (runtime)
| Module | Status |
|--------|--------|
| Engine.h / Engine.cpp | ✅ |
| DeviceManager | ✅ |
| InputManager + DI/XInput | ✅ |
| SimulationLoop + VehicleSimulator | ✅ |
| FFB stack | ✅ |
| SimulatorApp (Win32+Vulkan) | ✅ NativeRenderer, no Graphics/Qt |
| GameMenuOverlay / UiRenderer / GpuProfiler | ✅ |

## Excluded (editor / Qt UI)
DeviceSettingsWidget, MultiplayerWidget, SimulatorServerApp,
PostProcessing (QOpenGL*), Graphics/* (QObject/QWindow),
VehiclePhysics monolith, 3dprint/, scanners/

## SimulatorApp
- `src/simulator/SimulatorApp.cpp` and `src/engine/sim/SimulatorApp.cpp`
- No `Graphics/RenderSystem` / `PostProcessingPipeline`
- Uses `NativeRenderer` + `ShadowSystem` stub
