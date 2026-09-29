# KSEngine / ksimulator Qt-free

**Updated:** 2026-09-29

## Engine
- `Engine.h` + `EngineModule.h` — fixed tick, `update(dt)`, no Qt
- `KsQtFreeGuard.h` — hard fail if Qt headers leak in
- CMake AUTOMOC/UIC/RCC off, no Qt link

## Simulator
- `SimulationLoop` wires **NativeUiHub** each frame
- HUD from vehicle state; modal UI blocks driving input + FFB
- `handleUiKey(vk)` → menu / F1 devices / F2 multiplayer
- Font atlas + UiRenderer batch ready for GPU upload

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine ksimulator
```
