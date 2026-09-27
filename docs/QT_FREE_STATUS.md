# KSEngine Qt-free status

**Updated:** 2026-09-27

## Runtime std-only
Engine, XR, SimulatorApp, NativeRenderer, SimulationLoop,
FFB full stack, AiSpline, TrackLoader, AIController,
**VehicleSimulator** (new Qt-free vehicle + INI loaders),
physics core modules.

## Vehicle
- `VehicleSimulator.h/.cpp` — std::string `load*FromIni`, no QObject
- Legacy `VehiclePhysics.*` remains on disk, **CMake-excluded** (editor/monolith)

## Still Qt (excluded / editor)
VehiclePhysics*, TireCurveEditor, TrackPhysics, PhysicsSimulations,
Telemetry, Validator, WeatherConfig, Replay, 3dprint/, scanners/,
DeviceSettingsWidget, MultiplayerWidget, Graphics stack.
