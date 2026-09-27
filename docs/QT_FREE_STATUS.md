# KSEngine Qt-free status

**Updated:** 2026-09-27

## Policy
Runtime (`ksengine` + `SimulatorApp`) must be 100% Qt-free. `KSENGINE_QT_FREE=1`.

## Runtime — DONE
Engine, devices, XR, SimulatorApp, NativeRenderer, UI overlays, GpuProfiler.

### Physics (all Qt-free, included in ksengine)
PhysicsEngine, Aero, Tires/Pacejka, EngineModel, Diff, Gearbox, HybridSystem,
ChassisSimulator, Weather, TrackSurface, Suspension, BrakeThermal,
**BrakeWearSystem**, **ACModelManager**, **CharacterPhysics**, Damage, Logger, Profiler.

### Tick order
Weather → TrackSurface → Engine → Gear → Diff → Hybrid → Aero → Suspension → Brakes/Wear → Tires → step → Chassis → callbacks.

## Still Qt (editor only)
DeviceSettingsWidget, MultiplayerWidget, Graphics/QVulkanWindow, mesh editors, Scripting, Audio, VehiclePhysics monolith, TireCurveEditor.

## Gate
No QObject/QString/QVector/Q_OBJECT in linked runtime modules.
