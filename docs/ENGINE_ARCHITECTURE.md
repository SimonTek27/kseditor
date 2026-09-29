# Architecture: ksengine vs SimulatorApp

## Product tree

```
SimulatorApp  ≈  AC + CSP (open source)
       │
       ├── ksengine          (motore generico)
       ├── adapters/ac       (contenuti / SM / CSP)
       └── network           (multiplayer / telemetry transport)
```

| Branch | Role |
|--------|------|
| **SimulatorApp** | Product: open-source AC + CSP experience |
| **ksengine** | Generic sim runtime (physics, FFB, Vulkan, tick, NativeUi core) |
| **adapters/ac** | AC/CSP formats only (`src/adapters/assetto_corsa`) |
| **network** | Multiplayer, UDP/TCP, session sync (`src/simulator` net + engine net services) |
| **kseditor** | Qt authoring tool (separate target) |

```
┌──────────────────────────────────────────────┐
│           SimulatorApp (product)             │
│     sessions · HUD · garage · content UX     │
└───────┬──────────────┬──────────────┬────────┘
        │              │              │
        ▼              ▼              ▼
   ksengine      adapters/ac      network
   physics         SM / CSP        MP / UDP
   FFB / Vulkan    surfaces.ini    session sync
   NativeUi core   banks / GUID    car state
```

## Dependency rules
```
SimulatorApp  →  ksengine
SimulatorApp  →  adapters/assetto_corsa
SimulatorApp  →  network (simulator + optional engine/network helpers)
ksengine      ↛  adapters/*
ksengine      ↛  SimulatorApp-only UI
adapters/ac   ↛  network protocol ownership (app wires them)
```

### ksengine (`src/engine/`)
Generic only. Must not `#include` `adapters/*`.

### adapters/ac (`src/adapters/assetto_corsa/`)
Shared memory, surfaces.ini, CSP configs, AC banks/GUIDs. Linked by SimulatorApp.

### network
| Location | Use |
|----------|-----|
| `src/simulator/NetworkManager*` | App multiplayer orchestration |
| `src/simulator/NetworkLowLevel*` | Sockets / packets |
| `src/simulator/UdpTelemetryListener*` | AC-style telemetry ingest |
| `src/engine/network/*` (if present) | Reusable transport primitives |

Network is a **first-class peer** of the product stack: not buried only inside physics, and not an AC adapter concern.

## Qt-free
`ksengine` + `ksimulator` (SimulatorApp): `KSENGINE_QT_FREE=1`.
