# ksengine

**ksengine** is the Qt-free core engine framework of this project: a static C++17
library (`src/engine/`) with math, physics, devices/force-feedback, file formats,
config, networking, materials and terrain. It builds without Qt (progress tracked
by `tools/check_no_qt.ps1`) and optionally links Vulkan, Bullet, Eigen, Lua and
mikktspace.

---

# SimulatorApp

**SimulatorApp** is the standalone runtime executable (`src/simulator/`) that links
*only* ksengine. It is a native Win32 window with a raw Vulkan renderer (`NativeRenderer`, precompiled `.spv` shaders), driving `SimulationLoop`: KN5 track/car loading, vehicle physics, FFB and sim-racing device input, audio, dashboard/telemetry overlays, setup garage and multiplayer networking. `examples/MinimalSimulator` shows the minimal way to run it.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL3-blue.svg)](LICENSE.txt)
[![Version](https://img.shields.io/badge/version-1.16.4-orange)]()
[![C++](https://img.shields.io/badge/C++-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

---

# ksEditor

A comprehensive, professional-grade modding toolkit for racing-game content. ksEditor provides a unified environment for editing audio, 3D models, physics, telemetry, liveries, events, server configs, and more.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL3-blue.svg)](LICENSE.txt)
[![Version](https://img.shields.io/badge/version-1.16.4-orange)]()
[![Qt](https://img.shields.io/badge/Qt-6.11-green)]()
[![C++](https://img.shields.io/badge/C++-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

---

## Overview

This repository ships three products that share one core:

1. **ksengine** — Qt-free C++17 static library (math, physics, devices, formats).
2. **SimulatorApp (ksim)** — Qt-free race runtime (Vulkan + `SimulationLoop`).
3. **ksEditor** — optional Qt6 modding UI that links ksengine for all non-UI work.

The editor (`kseditor.exe`) links `kslib`, which PUBLIC-links `ksengine`; `ksengine.lib` is copied next to the executable at build time. Physics, file-format parsing, FFB/device handling and engine logic used by editor modules come from ksengine — the Qt layer only provides the UI.

ksEditor still covers the full modding pipeline (audio, 3D, physics, liveries, events, server config) and **ksAudioStudio** for FMOD-compatible banks. See **Architecture** below for layering and data flow.

---

## Architecture

ksengine is organized in **layers**. The editor is optional; the engine and simulator build without Qt.

```
┌─────────────────────────────────────────────────────────────┐
│  ksEditor (Qt6)                                             │
│  MainWindow + modding modules (modeler, audio, physics UI…) │
└────────────────────────────┬────────────────────────────────┘
                             │ links
                             ▼
┌─────────────────────────────────────────────────────────────┐
│  SimulatorApp / ksim  (Qt-free)                             │
│  Win32 + Vulkan · SimulationLoop · FeatureHub · native UI   │
│  session modes · pit/garage · telemetry · multiplayer       │
└────────────────────────────┬────────────────────────────────┘
                             │ links
                             ▼
┌─────────────────────────────────────────────────────────────┐
│  ksengine  (Qt-free static lib)                             │
│  Math · physics · devices/FFB · formats · scene · network   │
│  Engine + EngineModule registry · fixed-timestep core       │
└────────────────────────────┬────────────────────────────────┘
                             │ uses (optional)
                             ▼
┌─────────────────────────────────────────────────────────────┐
│  adapters/  (format bridges, not game-specific runtime)     │
│  assetto_corsa → shared memory, surfaces.ini, CSP configs   │
└─────────────────────────────────────────────────────────────┘
```

### Responsibilities

| Component | Responsibility |
|-----------|----------------|
| **ksengine** | Generic sim engine: vehicle dynamics, input/FFB, file I/O, ECS scene, low-level net. No UI framework. |
| **SimulatorApp** | Standalone race client: load track/car, drive loop, HUD/menu, sessions, pits, replay, LAN discovery, control API. |
| **adapters/** | Read/write content formats used by existing mods. Keeps the engine independent of any one title. |
| **ksEditor** | Modding toolkit UI on top of ksengine; not required to run ksim. |

### Runtime data flow (SimulatorApp)

```
Input (wheel/keys) → InputManager → VehicleSimulator (ksengine physics)
                                         │
                                         ▼
                              SimulationLoop tick @ fixed dt
                                         │
                    ┌────────────────────┼────────────────────┐
                    ▼                    ▼                    ▼
              FeatureHub            NativeRenderer         Telemetry
         (session, limits,         (Vulkan frame)     (UDP/TCP/shared mem)
          weather, PB, API)
                    │
                    ▼
              GameMenuOverlay / NativeUiHub  (GPU UI pass)
```

### Design rules

1. **Qt only in the editor** — `src/engine/` and `src/simulator/` stay std/Vulkan.
2. **No hard dependency on a commercial title** — format adapters are isolated under `src/adapters/`.
3. **FeatureHub** centralizes session mode, LAN discovery (`:20779`), external control (`:20780`), track limits, weather, setup, and personal bests.
4. **Pit/garage** follows a clear state machine (garage exit → pit queue → collision → repair) separate from core vehicle physics.

---

## Features

### 3D Printing

The **3D Printing Module** prepares **game assets for physical fabrication**. The **slicer engine** generates **GCode** with configurable layer height, infill patterns (grid, gyroid, honeycomb, adaptive), wall count, top/bottom layers. **Support structures** use tree/organic algorithms with customizable contact points and interface layers. **Print preview** simulates layer-by-layer with nozzle travel visualization, estimated time/material. **Printer profiles** store bed size, nozzle diameter, temperature curves, acceleration/jerk limits for popular printers (Prusa, Bambu, Creality, Voron). **STL repair** fixes non-manifold edges, inverted normals, self-intersections before slicing.

### VR Support

The **VR Editor** enables **immersive content creation** via OpenXR. The **VR viewport renderer** provides stereoscopic rendering with **lens distortion correction**, **variable rate shading**, and **foveated rendering** support. **Controller input** maps 6-DOF poses, triggers, thumbsticks, haptics to editor tools. **Vulkan interop** shares textures/buffers between desktop and VR views. Supports **Meta Quest, Valve Index, HTC Vive, Varjo, Pimax, Windows Mixed Reality**.

### Workshop Manager

The **Workshop Manager** integrates **Steam Workshop** for mod distribution: upload/publish, versioning, dependencies, local staging, and subscribed-content downloads.

### Mod Manager

The **Mod Manager** organizes installed content with metadata tracking, enable/disable, load order, integrity checks, repair, and profiles.

### Assets Library

Centralized browser for models, textures, audio, materials, physics configs, and scripts with search, preview, and dependency graph.

### Event Editor / Server Config / FFB / Telemetry / Weather / Help

Career events, dedicated-server rules, force-feedback curves, high-rate telemetry analysis, weather keyframes, and context-sensitive help (F1).

## Modules

### Text Editor (ksIDEEditor)

Code-aware IDE with LSP, syntax highlighting for 20+ languages, minimap, multi-cursor, and integrated terminal.

### 3D Modeler (ksModeler)

Vulkan-powered modeling for racing assets: primitives, UV, rigging, PBR, KN5/FBX/GLB/OBJ, Car/Track/Character editors.

### Audio Editor (ksAudioEditor)

DAW built into ksEditor with multi-track mixing, effects, FMOD bank compatibility, and **ksAudioStudio** as the internal engine.

### Physics Editor (ksPhysicsEditor)

Vehicle dynamics tuning: Pacejka tires, suspension, brakes, aero, powertrain, damage, weather grip effects.

### Display / Font / Paint / License Plates / Showroom / PP Filters

Dashboards, font atlases, liveries, plates, showroom presentation, and post-processing filter chains.

---

## Technical Stack

| Category | Technology |
|----------|------------|
| **Language** | C++17 |
| **Engine (ksengine)** | Qt-free static library |
| **SimulatorApp** | Win32 + Vulkan (no Qt) |
| **Editor UI** | Qt6 (Widgets + QML + Quick3D) |
| **3D Rendering** | Vulkan + GLSL / SPIR-V shaders |
| **Audio** | ksAudioStudio + WASAPI (simulator) |
| **Physics** | Bullet Physics 3.25+ (optional) |
| **Geometry** | CGAL, Eigen, libigl, OpenVDB, OpenSubdiv, mikktspace |
| **Scripting** | Python 3, Lua 5.4 |
| **Build System** | CMake 3.16+ |
| **Package Manager** | vcpkg (embedded) |
| **Target Platform** | Windows 10/11 (x64) |

---

## Project Structure

```
ksengine/
├── CMakeLists.txt              # Root build (ksengine + SimulatorApp + editor)
├── CMakePresets.json
├── LICENSE.txt                 # GPL-3.0
├── CONTRIBUTING.md
├── CHANGELOG.md
├── docs/                       # Architecture, parity, module notes
│   ├── PARITY_STATUS.md
│   ├── COMMIT_COMPLETE.md
│   ├── SIMLOOP_WIRING.md
│   └── …
├── examples/
│   └── MinimalSimulator/       # Minimal Qt-free runtime sample
├── external/                   # Eigen, Bullet, mikktspace, stb, …
├── include/                    # Public headers (if exported)
├── resources/                  # Qt UI / QML (editor only)
├── i18n/                       # Editor localization
├── tests/
└── src/
    ├── main.cpp                # ksEditor (Qt) entry
    ├── MainWindow.cpp/h        # Editor shell
    │
    ├── engine/                 # ★ ksengine — Qt-free static library
    │   ├── Engine.h / Engine.cpp / EngineModule.h
    │   ├── KsQtFreeGuard.h
    │   ├── Math/               # Math core (std)
    │   ├── physics/            # Vehicle, Pacejka, suspension, aero, weather
    │   ├── devices/            # FFB, input, simracing (Fanatec/Logitech/Moza/…)
    │   ├── Graphics/           # Vulkan-oriented render helpers
    │   ├── Audio/              # Engine audio utilities
    │   ├── FileFormat/         # KN5, INI, banks, mesh I/O
    │   ├── scene/              # ECS registry / systems
    │   ├── network/            # Low-level networking
    │   ├── vehicle/            # Vehicle helpers
    │   ├── material/ mesh/ terrain/
    │   ├── Config/ Scripting/ AI/ Tools/
    │   └── CMakeLists.txt
    │
    ├── simulator/              # ★ SimulatorApp — Qt-free runtime (ksim)
    │   ├── SimulatorApp.cpp            # Win32 + Vulkan entry
    │   ├── SimulationLoop.h/.cpp       # Fixed-timestep loop
    │   ├── SimulationLoop_FeatureMethods.cpp
    │   ├── SimulationLoop_Features.inl
    │   ├── FeatureHub.h                # Session, discovery, control API
    │   ├── SessionController.h
    │   ├── ServerDiscovery.h           # LAN UDP :20779
    │   ├── ExternalControlApi.h        # TCP control :20780
    │   ├── TrackLimitsMonitor.h
    │   ├── WeatherControl.h
    │   ├── PersonalBestStore.h
    │   ├── TrackLayout.h / ApplySetup.h / SetupFile.h
    │   ├── GarageExit.h / GarageSpawn.h
    │   ├── PitLaneQueue.h / PitLaneCollision.h / PitLaneRepair.h
    │   ├── RaceSessionManager.*
    │   ├── GameMenuOverlay.*           # Native menu (session modes)
    │   ├── NativeRenderer.* / ShadowSystem.* / PostProcessing.*
    │   ├── InputManager.* / CameraController.*
    │   ├── NetworkManager.* / UdpTelemetryBridge / TcpTelemetryBridge
    │   ├── SimulatorAudio.* / VehicleAudioHook / CarEventVolumes
    │   ├── SetupGarage.* / DashboardOverlay.* / TelemetryOverlay.*
    │   ├── ReplayRecorder.* / MultiCarManager.* / AIController.*
    │   ├── ui/                         # NativeUiHub, GPU UI pass
    │   └── shaders/                    # Precompiled .spv
    │
    ├── adapters/
    │   └── assetto_corsa/              # Format bridge (shared mem, surfaces, CSP)
    │       ├── AcSharedMemory*
    │       ├── AcSurfacesLoader*
    │       └── CspConfigParser*
    │
    └── sdk/                    # Optional game-SDK helpers
```

**Layering**

| Layer | Path | Qt | Role |
|-------|------|----|------|
| **ksengine** | `src/engine/` | No | Core math, physics, devices, formats |
| **SimulatorApp (ksim)** | `src/simulator/` | No | Standalone race runtime |
| **Adapters** | `src/adapters/` | No | Content-format bridges |
| **ksEditor** | `src/MainWindow*`, modules | Yes | Modding UI on top of ksengine |

---

## Building

### Requirements

- **Qt 6.11+** (MSVC 2022 or MinGW-w64) — required only for **ksEditor**
- **CMake 3.16+**
- **Vulkan SDK** — for SimulatorApp / NativeRenderer
- **Windows 10/11 x64**

### Quick build

```bash
# Configure (preset or manual)
cmake --preset default
cmake --build --preset default

# Engine + simulator only (Qt-free path when enabled in CMake)
# See CMakeLists_ksengine_QtFree.txt / tools/check_no_qt.ps1
```

---

## License

GPL-3.0 — see [LICENSE.txt](LICENSE.txt).
