# KSEngine / SimulatorApp Qt-free

**Updated:** 2026-09-28

## Converted this session
- EngineSimulator, InputSystem, SimucubeFFB
- phys_LapTimer, StrategySimulator, PhysicsMessage
- PostProcessing stub, ReplaySystem, TelemetryPhysics, WeatherConfig
- SimulationLoop → NativeRenderer
- SimulatorApp no Graphics/*

## Guarded (editor Qt only, skipped if KSENGINE_QT_FREE)
- MultiplayerWidget, DeviceSettingsWidget

## Build
```cmake
# root CMakeLists.txt
include(cmake/CMakeLists_root_qtfree_hook.cmake)
```
```bash
cmake -DKSIMULATOR_QT_FREE=ON -DKSENGINE_QT_FREE=ON ..
cmake --build . --target ksimulator
```

## Still excluded / editor monoliths
VehiclePhysics*.cpp, PhysicsSimulations.cpp, TrackPhysics.cpp (QJson),
3dprint/*, TireCurveEditor, Graphics/*
