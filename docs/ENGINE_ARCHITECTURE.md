# KS Engine architecture

## What the engine is

**ksengine** is a **generic, open-source simulation engine** aimed at simulator
applications (racing and related). It is **not** an Assetto Corsa fork and does
not embed AC or CSP as core dependencies.

### Core (`src/engine/`)
| Layer | Responsibility |
|-------|----------------|
| `Engine` / modules | Tick loop, registry, lifecycle |
| `physics/` | Vehicle, tires, aero, brakes (generic models) |
| `devices/` | Input, FFB, hardware abstraction |
| `Graphics` / `sim` | Render facade, NativeRenderer, GPU profiler |
| `Audio` | Generic audio core (optional backends) |
| `FileFormat` | Generic I/O (INI, mesh containers, …) |
| `Config` / `sys` / `network` | App-agnostic services |

### Simulator app (`src/simulator/`)
Session loop, UI overlays, input mapping, multiplayer — **uses** the engine.

### Content adapters (`src/adapters/`)
Game- or format-specific bridges. **Not required to build the engine.**

| Adapter | Purpose |
|---------|---------|
| `assetto_corsa/` | CSP configs, AC GUIDs, FSPRO banks, legacy AC assets |

CSP (Custom Shaders Patch) is an **Assetto Corsa community** component. It must
live under `adapters/assetto_corsa`, never as a hard dependency of `Engine` or
physics.

## Qt-free policy
Runtime targets (`ksengine`, `ksimulator`) build with `KSENGINE_QT_FREE=1` and
must not link Qt. The editor UI may still use Qt in a separate target.

## Dependency rule
```
simulator  →  engine  →  (stdlib, Vulkan, platform)
simulator  →  adapters/assetto_corsa   (optional)
engine     ↛  adapters/*               (forbidden)
```
