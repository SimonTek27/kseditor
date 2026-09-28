# KSEngine / SimulatorApp Qt-free

**Updated:** 2026-09-28

## Runtime 100% Qt-free
Engine, SimulatorApp, SimulationLoop (NativeRenderer), InputManager,
DeviceManager, VehicleSimulator, FFB, Pacejka, Aero, EngineSimulator,
StrategySimulator, PhysicsMessage, LapTimer, PostProcessing stub,
ReplaySystem, TelemetryPhysics, WeatherConfig.

## Editor monoliths (Qt)
Under `KSENGINE_QT_FREE` these TUs are empty / excluded by CMake:
VehiclePhysics*, PhysicsSimulations, TireCurveEditor, TrackPhysics,
PhysicsValidator, 3dprint/*, scanners/*, weather editor widgets,
MultiplayerWidget, DeviceSettingsWidget, SimulatorServerApp.

**Note:** Full Qt bodies for those files live in git history before the
qt-free stub commits. Restore when building the editor with Qt.

## Build
```bash
cmake -DKSIMULATOR_QT_FREE=ON -DKSENGINE_QT_FREE=ON ..
cmake --build . --target ksimulator
```
