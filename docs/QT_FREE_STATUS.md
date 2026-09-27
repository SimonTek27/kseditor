# KSEngine Qt-free status

**Updated:** 2026-09-27

## Runtime Qt-free (linked by ksengine)
Engine, devices, XR, SimulatorApp, NativeRenderer, UI, GpuProfiler.
Physics: PhysicsEngine, Aero, Tires/Pacejka, EngineModel, Diff, Hybrid, Chassis,
Weather, TrackSurface, Suspension(+Kinematics), BrakeThermal, BrakeWear,
TireWear, ACModelManager, CharacterPhysics, AIDriver, DriverSimulator, Damage, Logger.

## CMake-excluded (still Qt on disk)
VehiclePhysics(+Models), TireCurveEditor, TrackPhysics, PhysicsSimulations,
PhysicsValidator, TelemetryPhysics, WeatherConfig, ReplaySystem, PhysicsMessage,
StrategySimulator, phys_LapTimer, weather editors.

## Editor-only Qt
DeviceSettingsWidget, MultiplayerWidget, Graphics, mesh editors, Scripting, Audio.

## Gate
No QObject/QString/QVector in **linked** runtime modules.
