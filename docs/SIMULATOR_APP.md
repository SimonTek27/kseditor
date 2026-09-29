# SimulatorApp — open-source AC + CSP

```
SimulatorApp  ≈  AC + CSP (open source)
       │
       ├── ksengine          (motore generico)
       ├── adapters/ac       (contenuti / SM / CSP)
       └── network           (multiplayer / telemetry)
```

## Branches
| Path | Responsibility |
|------|----------------|
| **ksengine** | Physics, devices/FFB, Vulkan, fixed tick, native UI primitives |
| **adapters/ac** | AC folder formats, shared memory, CSP, surfaces.ini |
| **network** | Multiplayer sessions, car-state sync, UDP telemetry |

## Product goals
- Load AC cars/tracks
- AC-compatible shared memory for overlays
- CSP-oriented options via adapter (not in engine core)
- Online / LAN multiplayer as a peer subsystem
- Qt-free runtime binary (`ksimulator`)

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksimulator
```
