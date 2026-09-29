# SimulatorApp — open-source AC + CSP

**SimulatorApp** is the user-facing simulator: an open-source counterpart to the
**Assetto Corsa + Custom Shaders Patch** stack.

It is **not** a binary reimplementation of Kunos code. It reuses:
- **ksengine** for physics / FFB / render / loop
- **adapters/assetto_corsa** for AC file formats, shared memory, CSP configs

## Goals (parity targets)
| AC / CSP feature | SimulatorApp direction |
|------------------|------------------------|
| AC cars / tracks (INI, KN5, surfaces) | Load from AC content tree |
| Shared memory overlays | `AcSharedMemoryPublisher` |
| Sessions (practice / quali / race) | SimulationLoop session phases |
| CSP visuals / extras | Adapter parsers + render hooks (not in engine core) |
| FFB wheels | FFBBridge + device SDKs |
| Apps / telemetry | SM + UDP compatible layouts |
| Garage / setup | SetupGarage + NativeUi |

## Content layout (AC-compatible)
```
<AC_ROOT>/
  content/cars/<car_id>/...
  content/tracks/<track_id>/...
  system/cfg/...
```
Env / config: `KS_AC_ROOT` or in-app path (see SimulatorApp bootstrap).

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksimulator
```

## vs engine
- Need a **library** for any sim → **ksengine**
- Need the **AC+CSP-like game** → **SimulatorApp**
