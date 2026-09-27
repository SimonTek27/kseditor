# KSEngine Qt-free status

**Updated:** 2026-09-27

## Policy
Runtime (`ksengine` + `SimulatorApp`) must be 100% Qt-free before any real build is greenlit.  
`KSENGINE_QT_FREE=1` is defined on the `ksengine` target.

## Runtime — DONE (Qt-free)

### Core / devices / app
Engine.h, EngineModule.h, DeviceManager, SimRacingDevices, SimulatorApp, NativeRenderer, GameMenuOverlay, UiRenderer, GpuProfiler.

### XR (`src/engine/devices/vr/`)
XrDispatch.h, XrManager.h/.cpp, XrManager_session.cpp, XrInput, XrIntegration, XrViewportRenderer.

### Physics
PhysicsCoreTypes, PhysicsEngine, Aero*, Tires/Pacejka, EngineModel, Gearbox, Differential, **HybridSystem (header-only)**, Weather, TrackSurface, Suspension, BrakeThermal, Chassis, Damage, Logger, Profiler, phys_Simulator.

### Tick order
Weather → TrackSurface → Engine → Gear → Diff → Hybrid → Aero → Suspension → Brakes → Tires → step → Chassis → rubber/callbacks.

## Still Qt (editor only)
DeviceSettingsWidget, MultiplayerWidget, Graphics/QVulkanWindow, mesh editors, Scripting, Audio, VehiclePhysics monolith, TireCurveEditor.

## Gate
No QObject/QString/QVector/Q_OBJECT in runtime modules. Build of SimulatorApp + ksengine allowed under Qt-free policy.
