# Architecture: ksengine vs SimulatorApp

## Product split

| Target | Role |
|--------|------|
| **ksengine** | Generic **open-source simulation engine** (physics, devices, render, tick). No AC/CSP required. |
| **SimulatorApp** (`ksimulator`) | **Open-source clone of the Assetto Corsa + CSP experience** — sessions, AC content, shared memory, CSP-oriented graphics hooks, garage, multiplayer. |
| **kseditor** | Qt tool for authoring (separate from runtime). |

```
┌─────────────────────────────────────────────┐
│  SimulatorApp  ≈  AC + CSP (open source)    │
│  sessions · AC assets · CSP configs · HUD   │
└───────────────────┬─────────────────────────┘
                    │ uses
┌───────────────────▼─────────────────────────┐
│  ksengine  (generic runtime)                │
│  physics · FFB · Vulkan · NativeUi · net    │
└───────────────────┬─────────────────────────┘
                    │ optional / app-linked
┌───────────────────▼─────────────────────────┐
│  adapters/assetto_corsa                     │
│  shared mem · surfaces.ini · CSP · banks    │
└─────────────────────────────────────────────┘
```

## ksengine (`src/engine/`)
Generic layers only. **Rule:** must not `#include` `adapters/*`.

## SimulatorApp (`src/simulator/`)
Product aiming at **AC + CSP parity** (open source):
- AC content tree, shared memory, CSP via adapters
- Sessions, garage, multiplayer, Native UI, Vulkan

## Qt-free
`ksengine` + `ksimulator`: `KSENGINE_QT_FREE=1`. Editor may use Qt.

```
SimulatorApp  →  engine
SimulatorApp  →  adapters/assetto_corsa
engine        ↛  adapters/*
```
