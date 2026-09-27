# KSEngine Qt-free status

**Updated:** 2026-09-27

## Policy
Runtime (`ksengine` + `SimulatorApp`) must be 100% Qt-free before any real build is greenlit.  
`KSENGINE_QT_FREE=1` is defined on the `ksengine` target.

## Runtime stack — DONE (Qt-free) and on GitHub (SimonTek27/kseditor)

### Engine / devices / XR / app
| Area | Files |
|------|--------|
| Core loop | Engine.h (shared_mutex ECS, EngineLoop, callbacks) |
| Devices | DeviceManager.h/.cpp, SimRacingDevices.h/.cpp (TripleMonitor, FFB) |
| XR | XrManager.h, XrInput.h/.cpp, XrIntegration.h/.cpp, XrViewportRenderer.h |
| App | SimulatorApp.cpp, NativeRenderer.h, GameMenuOverlay, UiRenderer, GpuProfiler |

### Physics (integrated in phys_Simulator)
PhysicsCoreTypes, PhysicsEngine, Aero, Pacejka/Tires, Engine/Gear/Diff, Hybrid, Weather, TrackSurface, Suspension, BrakeThermal, Chassis, Damage, Logger/Profiler, **phys_Simulator**.

### Tick order
Weather → TrackSurface → Engine → Gear → Diff → Hybrid → Aero → Suspension → Brakes → Tires → step → Chassis → rubber/callbacks.

## Still Qt (editor only — excluded from ksengine)
DeviceSettingsWidget, MultiplayerWidget, Graphics/QVulkanWindow path, mesh editors, Scripting, Audio.

## Gate
Runtime scan: **no QObject/QString/QVector/Q_OBJECT** in Engine, devices, physics, SimulatorApp, NativeRenderer, XR headers.
