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

A comprehensive, professional-grade modding toolkit for **Assetto Corsa** and other Kunos/Steam racing games. ksEditor provides a unified environment for editing game audio, 3D models, physics, telemetry, liveries, events, server configs, and more.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-GPL3-blue.svg)](LICENSE.txt)
[![Version](https://img.shields.io/badge/version-1.16.4-orange)]()
[![Qt](https://img.shields.io/badge/Qt-6.11-green)]()
[![C++](https://img.shields.io/badge/C++-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()
---

## Overview

ksEditor is a **Qt6-based desktop application** designed to provide modders with professional editing tools for racing game content. It combines multiple specialized editors into a single, cohesive environment with native performance and a modern UI, uses ksengine to run.** The editor (`kseditor.exe`) links `kslib`,
which PUBLIC-links `ksengine`; `ksengine.lib` is copied next to the executable at build time. Physics, file-format parsing, FFB/device handling and the rest of the engine logic used by the editor's modules all come from ksengine — the Qt layer only provides the UI.

The editor supports the **full Assetto Corsa modding pipeline**, from audio synthesis to 3D modeling, physics simulation, livery painting, event creation, and server configuration. It is compatible with **FMOD Studio 1.08.12** project formats used by the game and includes its own internal audio workstation (**ksAudioStudio**).

---

## Features

### 3D Printing

The **3D Printing Module** prepares **game assets for physical fabrication**. The **slicer engine** generates **GCode** with configurable layer height, infill patterns (grid, gyroid, honeycomb, adaptive), wall count, top/bottom layers. **Support structures** use tree/organic algorithms with customizable contact points and interface layers. **Print preview** simulates layer-by-layer with nozzle travel visualization, estimated time/material. **Printer profiles** store bed size, nozzle diameter, temperature curves, acceleration/jerk limits for popular printers (Prusa, Bambu, Creality, Voron). **STL repair** fixes non-manifold edges, inverted normals, self-intersections before slicing.

### VR Support

The **VR Editor** enables **immersive content creation** via OpenXR. The **VR viewport renderer** provides stereoscopic rendering with **lens distortion correction**, **variable rate shading**, and **foveated rendering** support. **Controller input** maps 6-DOF poses, triggers, thumbsticks, haptics to editor tools (grab/scale/rotate objects, paint liveries, sculpt terrain, place trackside objects). **UI panels** appear as world-space tablets or wrist-mounted menus. **Vulkan interop** shares textures/buffers between desktop and VR views for zero-copy mirroring. Supports **Meta Quest (Link/AirLink), Valve Index, HTC Vive, Varjo, Pimax, Windows Mixed Reality**.

### Workshop Manager

The **Workshop Manager** integrates **Steam Workshop** for mod distribution. It provides **upload/publish workflows** with metadata (title, description, tags, preview images, changelog). **Version management** tracks updates with semantic versioning. **Dependency declaration** links required mods/content. **Local staging** validates package structure before publishing. **Download manager** handles subscribed content with auto-update and conflict detection.

### Mod Manager

The **Mod Manager** organizes **installed content** with a unified library view. It tracks **mod metadata** (version, author, dependencies, compatibility), provides **enable/disable toggles** with load-order management, and runs **integrity checks** (file hashes, missing assets, version conflicts). **Repair function** re-downloads corrupted files from Workshop or source. **Profile system** saves mod sets for different leagues, series, or testing scenarios.

### Assets Library

The **Assets Library** is a **centralized content browser** for all project assets. It indexes **3D models, textures, audio, materials, physics configs, scripts** with metadata extraction (poly count, texture resolution, duration, format). **Search** supports filters (type, tags, size, date, usage), **full-text** in scripts/configs, and **visual similarity** for textures. **Preview pane** renders 3D models (turntable), plays audio, displays images with histogram. **Dependency graph** shows asset references (what uses this texture, which car needs this physics file). **Cloud sync** backs up library index and shares across workstations.

### Event Editor

The **Event Editor** creates **single-player career content**: **championships** (calendar, points systems, penalties, drop rounds), **races** (grid size, qualifying format, pit rules, weather slots, time acceleration), **special events** (time attack, drift, autocross, hillclimb, endurance with driver swaps). **AI roster management** assigns cars/liveries/names/skill per event. **Reward trees** define unlockables (cars, liveries, tracks, currency) with branching prerequisites. **Export** generates CSP-compatible event JSON for Career Mode.

### Server Config Editor

The **Server Config Editor** administers **dedicated multiplayer servers**. It edits **session rules** (practice/qualify/race durations, weather progression, dynamic track rubbering), **entry list** (car restrictions, BOP ballast, restrictor, fuel capacity, tire compounds), **driver aids** (ABS, TC, stability, auto-clutch, auto-blip), **penalty system** (track limits, pit speed, contact, drive-through/stop-go), and **admin commands** (kick, ban, restart, weather change, grid penalty). **Presets** for common series (GT3, TCR, Formula, Cup). **Live preview** validates config syntax and simulates session flow.

### FFB Editor

The **Force Feedback Editor** tunes **steering wheel feedback** for each car. It adjusts **gain, filter, damping, spring, friction** per effect type (curb, slip, impact, understeer, oversteer, ABS, surface). **Frequency analysis** visualizes FFB spectrum. **Curve editors** shape force-vs-slip and force-vs-speed relationships. **Profile comparison** overlays multiple configs. **Presets** for popular wheels (Fanatec, Logitech, Thrustmaster, Simucube, Moza) with auto-detection.

### Telemetry Viewer

The **Telemetry Viewer** records, analyzes, and compares **vehicle telemetry data**. It captures **high-frequency samples** (100-1000 Hz) for channels: speed, RPM, throttle/brake/clutch, steering, G-forces, suspension travel, tire temps/pressures, aero loads, fuel, temperatures. **Lap timing** auto-detects sectors with split comparison against reference laps. **Data export** supports CSV, JSON, and MAT formats for external analysis (MATLAB, Python). Overlay multiple laps with **delta-time visualization** and **driver input comparison**.

### Weather Editor

The **Weather Editor** designs **dynamic weather sequences** for races and showrooms. It defines **keyframes** (time, cloud cover, precipitation, fog, wind speed/direction, ambient/track temperature, humidity). **Interpolation** creates smooth transitions. **Preset library** includes clear, overcast, light rain, heavy storm, foggy morning, sunset transition. **Preview renderer** shows sky dome, cloud layers, rain particles, puddle formation, track drying line in real-time. **Export** generates CSP weather scripts and showroom lighting states.

### Help System

The **Help System** provides **context-sensitive assistance** throughout the application. **F1 key** opens relevant documentation for the focused widget/panel. **Tutorial system** guides users through multi-step workflows (create car from scratch, build track, setup physics, paint livery) with interactive highlights. **QuickStart** covers first-launch setup (game paths, SDK init, plugin config). **Help browser** searches all docs, shows keyboard shortcuts, FAQ, troubleshooting. **20+ help contexts** map to specific editors, panels, and dialogs. **Offline-first** with optional online updates.

## Modules

### Text Editor (ksIDEEditor)

The **Text Editor** is a **code-aware IDE** for scripting and config files. It provides **LSP client** integration (clangd for C++, pylsp for Python, lua-language-server for Lua) with **hover docs, go-to-definition, find-references, rename, diagnostics**. **Syntax highlighting** for 20+ languages (C++, Python, Lua, JSON, XML, INI, GLSL, HLSL, CSP, ACD). **Minimap** with error/warning markers. **Multi-cursor editing**, **snippets**, **bracket matching**, **auto-indent**, **folding**. **Integrated terminal** for build/test commands.

### 3D Modeler (ksModeler)

**KSModeler** is a **Vulkan-powered 3D modeling environment** purpose-built for racing game assets. It offers **mesh primitives**, **UV mapping**, **skeletal rigging**, **PBR material system**, and **file format converters** for **KN5, FBX, GLB, OBJ**. Specialized editors include **CarEditor**, **TrackEditor**, and **CharacterEditor**.

### Audio Editor (ksAudioEditor)

The **Audio Editor** is a full-featured digital audio workstation built into ksEditor with multi-track mixing, effects, FMOD bank compatibility, and **ksAudioStudio** as the internal engine.

### Physics Editor (ksPhysicsEditor)

The **Physics Editor** delivers vehicle dynamics simulation and tuning: Pacejka tires, suspension kinematics, brakes, aero, powertrain, damage, and weather effects on grip.

### Display / Font / Paint / License Plates / Showroom / PP Filters

Specialized editors for dashboards, font atlases, liveries, plates, showroom presentation, and post-processing filter chains.

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
