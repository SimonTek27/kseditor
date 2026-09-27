# KSEngine Qt-free status

**Updated:** 2026-09-27

## Runtime std-only
Engine, XR, SimulatorApp, NativeRenderer, SimulationLoop,
FFBBridge, AiSpline, TrackLoader, AIController,
physics core (Pacejka, Aero, Hybrid, Chassis, BrakeWear, TireWear, …).

## FFB vendors
modelName() → std::string (headers). Rebuild CPP from artifacts (no QDebug/qBound).

## Still Qt (excluded / editor)
VehiclePhysics*, TireCurveEditor, TrackPhysics, PhysicsSimulations,
Telemetry, Validator, WeatherConfig, Replay, 3dprint/, scanners/,
DeviceSettingsWidget, MultiplayerWidget, Graphics stack.
