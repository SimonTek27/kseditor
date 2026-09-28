# Layout + Qt-free status

**Updated:** 2026-09-28

## Directory layout (unified)

```
src/
  engine/
    physics/ devices/ Math/ ...
    sim/                 # NativeRenderer, GpuProfiler, UiRenderer ONLY
    Engine.h
  simulator/             # UNIQUE app layer
    SimulatorApp.cpp
    SimulationLoop.*
    InputManager.*
    GameMenuOverlay.*
    MultiCarManager.*
    ...
```

Removed duplicates:
- `src/engine/simulator/` (deleted)
- `src/engine/sim/SimulatorApp.cpp`
- `src/engine/sim/GameMenuOverlay.*`

Shim: `src/simulator/NativeRenderer.h` → `#include "engine/sim/NativeRenderer.h"`

## Build
```bash
cmake -DKSIMULATOR_QT_FREE=ON -DKSENGINE_QT_FREE=ON ..
cmake --build . --target ksimulator
```
