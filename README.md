# ksEditor

A comprehensive, professional-grade modding toolkit for **Assetto Corsa** and other Kunos/Steam racing games. ksEditor provides a unified environment for editing game audio, 3D models, physics, telemetry, liveries, events, server configs, and more.

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)
[![Version](https://img.shields.io/badge/version-1.16.4-orange)]()
[![Qt](https://img.shields.io/badge/Qt-6.11-green)]()
[![C++](https://img.shields.io/badge/C++-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)]()

---

## Overview

ksEditor is a **Qt6-based desktop application** designed to provide modders with professional editing tools for racing game content. It combines multiple specialized editors into a single, cohesive environment with native performance and a modern UI.

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

### Text Editor

The **Text Editor** is a **code-aware IDE** for scripting and config files. It provides **LSP client** integration (clangd for C++, pylsp for Python, lua-language-server for Lua) with **hover docs, go-to-definition, find-references, rename, diagnostics**. **Syntax highlighting** for 20+ languages (C++, Python, Lua, JSON, XML, INI, GLSL, HLSL, CSP, ACD). **Minimap** with error/warning markers. **Multi-cursor editing**, **snippets**, **bracket matching**, **auto-indent**, **folding**. **Integrated terminal** for build/test commands.

### 3D Modeler (ksModeler)

**KSModeler** is a **Vulkan-powered 3D modeling environment** purpose-built for racing game assets. It offers **mesh primitives** (cube, sphere, cylinder, cone, torus, plane, grid), comprehensive **UV mapping** tools (planar, cylindrical, spherical, box projection, LSCM/ABF++ unwrap, auto-pack), and **skeletal rigging** with humanoid/quadruped skeleton creation, bone manipulation, weight painting, and IK solvers. The **PBR material system** includes a **node-based shader graph editor** with live preview. Animation support covers keyframe tracks, playback controls, and timeline editing. **File format converters** handle bidirectional workflows for **KN5, FBX, GLB, OBJ**. Specialized editors include **CarEditor** (vehicle hierarchy, livery painter, tire/engine rig tools), **TrackEditor** (geometry, terrain, waypoints, sectors), and **CharacterEditor** (driver models). Advanced geometry operations leverage **CGAL for boolean operations** (BSP), **OpenSubdiv for Catmull-Clark subdivision**, **libigl for mesh processing**, and **OpenVDB for volumetric workflows**. The UI provides a project explorer, properties inspector, layers, history, Python console, material preset library, curve editor, and render settings.

### Audio Editor (ksAudioEditor)

The **Audio Editor** is a full-featured digital audio workstation built directly into ksEditor. It provides **35+ specialized panels** covering the entire audio production pipeline: a multi-track **AudioMixerPanel** with channel strips, VU meters, and master bus routing; an **EffectsRackPanel** with parametric EQ, compressor, gate, reverb, delay, and limiter; real-time analysis via **AudioAnalyzerPanel** (spectrum, phase scope), **LoudnessMeterPanel** (EBU R128 LUFS/True Peak/LRA), and **OscilloscopePanel**. Advanced dynamics include **SidechainCompressorPanel**, **MultibandCompressorPanel** (4-band with crossover visualization), and **TransientDesignerPanel**. Creative tools feature **ConvolutionReverbPanel** (impulse responses), **TapeEmulatorPanel** (2"/1"/Cassette/VHS/DAT), **GuitarAmpSimulatorPanel**, **VocalProcessorPanel** (de-esser, pitch correction), **HarmonicGeneratorPanel**, and **StereoEnhancerPanel**. The editor supports **VST2/3 plugin hosting**, a **node-based audio graph** for complex routing, **surround mixing** up to 7.1.4, **automation lanes** for volume/pan/filter, and full **FMOD Studio 1.08.12 `.fspro` project and `.bank` runtime bank import/export** compatibility. Dialogs cover import/export wizards, tone generation, BPM detection, recording, preset management, and metadata editing.

### Audio Studio (ksAudioStudio)

**ksAudioStudio** is the **internal audio engine and workstation** powering all audio functionality in ksEditor, serving as a native **replacement for FMOD Studio**. It provides complete **FMOD Studio 1.08.12 project compatibility** with bidirectional **`.fspro` project file** and **`.bank` runtime bank** import/export. The **native "KSaudio" format** stores projects with full signal graph, automation, and plugin state. The **DSP pipeline** supports real-time synthesis (additive, subtractive, FM, granular, wavetable), **35+ built-in effects** (dynamics, EQ, modulation, delay, reverb, distortion, pitch), and **VST2/3 plugin hosting** with parameter automation. **Multi-channel surround** handles up to 7.1.4 with flexible bus routing. The **node-based audio graph editor** enables visual signal flow design with real-time parameter modulation. **Analysis tools** include spectrum analyzer (FFT, 1/3 octave), oscilloscope, phase correlation, loudness metering (EBU R128, ATSC A/85), and true peak detection. **Recording engine** captures multi-channel audio with punch-in/out, loop recording, and take management. **Batch processing** applies effect chains, format conversion, loudness normalization, and dithering across multiple files.

### Physics Editor (ksPhysicsEditor)

The **Physics Editor** delivers a complete **vehicle dynamics simulation and tuning environment**. It implements the **Pacejka "Magic Formula" tire model** with full parameter exposure for longitudinal/lateral/combined slip. Suspension geometry includes double-wishbone, MacPherson, multi-link configurations with kinematics visualization. **Brake tuning** covers bias, pressure curves, duct cooling, and temperature modeling. **Aerodynamics** supports configurable wings, splitter, diffuser, and DRS zones with ride-height-sensitive maps. **Powertrain modeling** encompasses engine torque curves, turbo/charger dynamics, hybrid/ERS deployment strategies (MGU-K, MGU-H), gearbox ratios, differential settings (preload, ramp angles, clutch packs), and driveline inertia. **Damage modeling** tracks mechanical wear, aero degradation, and tire punctures. **Weather simulation** integrates ambient temperature, track temperature, humidity, wind, and precipitation effects on grip. The editor provides **telemetry overlay**, **lap time estimation** with sector analysis, **setup comparison** (side-by-side parameter diff), and **86 validated unit tests** covering tire, suspension, aero, and powertrain subsystems.

### Display Editor

The **Display Editor** configures **on-screen dashboards and UI elements** for in-game telemetry. It edits **segment-based LCD/LED displays** (gear, RPM, speed, lap time, fuel, temperatures) with customizable fonts, colors, and layouts. Supports **custom display pages** with conditional visibility (pit limiter, DRS, ERS modes, flag signals). Preview mode simulates game rendering with accurate character spacing and kerning.

### Font Editor

The **Font Editor** generates **game-ready font atlases** from TTF/OTF sources. It packs glyphs into **power-of-two textures** with configurable padding, supports **Unicode ranges** (Basic Latin, Latin Extended, Cyrillic, CJK), and exports **distance field** or **standard bitmap** formats. Includes **kerning pair** extraction, **fallback chain** configuration, and **preview rendering** at multiple sizes. Output formats match game engine requirements (DDS/BC, PNG).

### Livery Editor

The **Livery Editor** is a **layer-based 3D painting system** for creating custom car liveries. It features **unlimited layers** with blending modes (normal, multiply, overlay, screen), **vector drawing tools** (pen, shapes, text, gradients), and a **decal/stencil system** for logos, numbers, and sponsorship graphics with precise placement. **Material masks** allow per-layer control over paint, metallic, roughness, and clearcoat properties. A **template system** provides base UV layouts for popular cars with automatic seam alignment. **Color palettes** support manufacturer swatches, custom gradients, and eyedropper sampling. The **3D preview viewport** uses studio lighting rigs (HDRI, key/fill/rim) with real-time PBR rendering. Export produces **DDS textures** with mipmaps and BC compression ready for game integration.

### License Plates

The **License Plate Editor** generates **custom registration plates** matching regional formats (EU, US, UK, JP, custom). It supports **font selection**, **character spacing**, **background templates** (reflective, flat, vintage), **embossed/flat styles**, and **official font recreation** (FE-Schrift, UK Mandatory, etc.). **Batch generation** creates sequential plates for AI traffic. Export produces **game-ready textures** with alpha masks.

### Showroom Editor

The **Showroom Editor** creates **photorealistic vehicle presentations** for showcases, thumbnails, and marketing. It provides **studio lighting rigs** (3-point, ring, softbox, HDRI environments) with physical light units (lumens, candela, kelvin). **Camera paths** support keyframed Dolly/Track/Crane movements with smooth interpolation for turntable videos. **Reflection probes** capture environment lighting for accurate PBR material response. **Material overrides** let you swap shaders/textures per-mesh for A/B comparisons. A **render queue** batches multiple camera angles, resolutions, and exposure settings. The **comparison slider** enables interactive before/after or variant comparisons in the viewport.

### PP Filters Editor

The **Post-Processing Filters Editor** manages the **visual effects pipeline** for tracks and showrooms. It organizes **filter chains** (bloom, tone mapping, color grading, vignette, chromatic aberration, film grain, lens flare, DOF, motion blur) with per-filter parameter exposure. **Preset system** saves/loads complete looks. **Real-time preview** in the showroom viewport with histogram and vectorscope. Exports filter configurations for game integration.



---

## Technical Stack

| Category | Technology |
|----------|------------|
| **Language** | C++17 |
| **UI Framework** | Qt6 (Widgets + QML + Quick3D) |
| **3D Rendering** | Vulkan (via QVulkanWindow) + GLSL shaders |
| **Audio Engine** | ksAudioStudio (internal) + Qt6::Multimedia |
| **Physics** | Bullet Physics 3.25+ (optional) |
| **Geometry** | CGAL, Eigen, libigl, OpenVDB, OpenSubdiv, mikktspace |
| **Scripting** | Python 3, Lua 5.4 |
| **Build System** | CMake 3.16+ |
| **Package Manager** | vcpkg (embedded) |
| **Archive** | 7-Zip 24+ |
| **Target Platform** | Windows 10/11 (x64) |

---

## Project Structure

```
kseditor/
├── CMakeLists.txt              # Build configuration (850+ lines)
├── CMakePresets.json           # CMake presets for IDEs
├── compile.bat / compile.sh    # Quick build scripts
├── build.bat                   # Full Windows build + deployment
├── LICENSE.txt                 # MIT License
├── CONTRIBUTING.md             # Contribution guidelines
├── CHANGELOG.md                # Version history
├── docs/                       # Documentation
│   ├── overview.md             # Architecture overview
│   ├── modules.md              # Module documentation
│   ├── graphics.md             # Graphics pipeline
│   ├── audio.md                # Audio system
│   ├── file_formats.md         # Supported formats
│   ├── plugins.md              # Plugin development
│   ├── video.md                # Video encoding
│   ├── ksAssettoCorsa.MD       # AC plugin docs
│   ├── ksAssettoCorsaEVO.md    # AC EVO support
│   ├── ksAssettoCorsaCompetizione.md # ACC support
│   └── ksAssettoCorsaRally.md  # ACR support
├── src/
│   ├── main.cpp                # Application entry point
│   ├── SDKBackend.cpp/h        # Game SDK integration
│   ├── app_icon.rc             # Windows icon resource
│   ├── core/                   # Core subsystems (32 modules)
│   │   ├── Audio/              # Audio engine (65 files, DSP, graph)
│   │   ├── Graphics/           # Vulkan renderer (26 files + 12 shaders)
│   │   ├── FileFormat/         # 50+ format parsers/converters
│   │   ├── mesh/               # Mesh operations (42 files)
│   │   ├── material/           # PBR materials, shader graph (16 files)
│   │   ├── sys/                # Plugin/module managers, undo, tasks
│   │   ├── editor/             # Base editor, console, timeline, ribbon
│   │   ├── assets/             # Asset manager, search, preview, cloud
│   │   ├── tools/              # Tools framework (49 files, LOD/collision)
│   │   ├── ui/                 # Widgets (30 files, node graph, title bar)
│   │   ├── modmanager/         # Mod/package manager
│   │   ├── Scripting/          # Python/Lua hosts, debugger, hot-reload
│   │   ├── network/            # Cloud sync, collaboration, WebSocket
│   │   ├── Config/             # CSP config editor, schema-driven UI
│   │   ├── AIEditor/           # Behavior trees, telemetry trainer
│   │   ├── animation/          # Skeletal animation, IK, blend trees
│   │   ├── weather/            # Weather editor, preview
│   │   ├── eventEditor/        # Career/championship/race editors
│   │   ├── ServerConfigEditor/ # Server configuration editor
│   │   ├── textEditor/         # Code editor with LSP
│   │   ├── FfbEditor/          # Force feedback configuration
│   │   ├── ppfiltersEditor/    # Post-processing filters
│   │   ├── 3dprint/            # 3D printing (slicer, GCode)
│   │   ├── VR/                 # OpenXR integration
│   │   ├── workshop/           # Steam Workshop integration
│   │   ├── vcs/                # Git version control
│   │   ├── archive/            # 7-Zip integration
│   │   ├── help/               # Context help system
│   │   ├── splitter/           # Window tiling management
│   │   └── formatTools/        # Format conversion utilities
│   ├── modules/                # High-level application modules (9)
│   │   ├── modellingEditor/    # 3D modeling (49 files)
│   │   ├── soundEditor/        # Audio editing (30 files)
│   │   ├── PhysicsEditor/      # Physics simulation (38 files)
│   │   ├── LiveryEditor/       # Car livery painting (20 files)
│   │   ├── ShowroomEditor/     # 3D showroom (10 files)
│   │   ├── displayEditor/      # Display editor (6 files)
│   │   ├── LicensePlatesEditor/ (6 files)
│   │   ├── fontEditor/         # Font atlas (4 files)
│   │   └── VREditor/           # VR viewport
│   ├── plugins/                # Plugin architecture
│   │   ├── base/               # PluginBase.h interfaces
│   │   └── simulators/kunos/   # Assetto Corsa integration (50+ files)
│   └── tests/                  # Unit tests (25+ files, 40/40 suites pass)
├── resources/
│   ├── ui/                     # Qt UI resources, Ribbon UI
│   └── qml/                    # QML components
├── i18n/                       # Localization (en, de, ja, it, es, fr, pt-BR, ru, zh-CN, zh-TW)
├── external/                   # Embedded dependencies
│   ├── eigen/                  # Eigen 3.4+ (linear algebra)
│   ├── bullet/                 # Bullet Physics 3.25+
│   ├── libigl/                 # libigl 2.5+ (geometry processing)
│   ├── 7zip/                   # 7-Zip 24+
│   ├── mikktspace/             # Mikktspace (tangent space)
│   ├── openxr/                 # OpenXR headers
│   ├── stb/                    # STB image libraries
│   └── vcpkg/                  # vcpkg (CGAL, OpenVDB, libuv, OpenSubdiv, Lua, Python)
└── bin/                        # Build output (executables, plugins, DLLs)
```

---

## Building

### Requirements

- **Qt 6.11+** (MSVC 2022 or MinGW-w64)
- **CMake 3.16+**
- **C++17 compatible compiler** (MSVC 2022 17.8+, GCC 11+, Clang 14+)
- **Vulkan SDK** (optional, for 3D features)
- **Windows 10/11** (primary target)

### Quick Build (Windows)

```powershell
# Using the build script (handles dependencies, deployment)
.\build.bat

# Or manually:
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Build Scripts

| Script | Purpose |
|--------|---------|
| `build.bat` | Full Windows build with vcpkg, deployment, DLL copying |
| `compile.bat` | Basic compilation only |
| `compile.sh` | Basic compilation (Linux/WSL) |

### CMake Options

```cmake
# Key variables (auto-detected by default)
-DQT6_PATH="C:/Qt/6.11.1/msvc2022_64"     # Qt installation path
-DCMAKE_BUILD_TYPE=Release                 # Build type
-DBUILD_TESTS=ON                           # Enable unit tests
```

### Output

After building, the executable and plugins are in:
```
bin/
├── ksEditor.exe              # Main application
├── plugins/
│   └── ksAssettoCorsa.dll    # AC plugin
└── lib/                      # Runtime DLLs (Qt, Vulkan, etc.)
```

---

## SDK Integration

ksEditor includes a comprehensive **SDKBackend** for Assetto Corsa content manipulation:

```cpp
#include "SDKBackend.h"

// Initialize
ks::SDKBackend* sdk = ks::SDKBackend::instance();
sdk->initialize();

// List installed content
QStringList cars = ks::SDKBackend::getCarList();
QStringList tracks = ks::SDKBackend::getTrackList();

// Load car/track specifications
ks::ACCarSpec spec;
ks::SDKBackend::loadCarSpec("ks_nissan_gtr", spec);

// Physics calculations
float downforce = ks::SDKBackend::calculateDownforce(speed, aoa, cl);
float cornerG = ks::SDKBackend::calculateCornerG(speedMs, radius);
```

**SDK Version:** 1.4

---

## Supported File Formats

### 3D Models
`KN5` `FBX` `GLB` `OBJ` `STL` `Alembic` `USD` `Collada` `3DS` `PLY` `DXF` `VRML` `3MF` `STEP` `IGES`

### Audio
`WAV` `OGG` `FLAC` `MP3` `AIFF` `FMOD .fspro` `FMOD .bank` `KSaudio` (native)

### Textures
`DDS` `PNG` `JPG` `TGA` `BMP` `HDR` `EXR` `KTX` `KTX2`

### Data
`ACD` (physics) `INI` `JSON` `XML` `Lua` `Python` `CSP` configs

### Fonts
`TTF` `OTF` `WOFF` `WOFF2`

---

## Localization

ksEditor supports **10 languages**:
- English (default)
- German (de)
- Japanese (ja)
- Italian (it)
- Spanish (es)
- French (fr)
- Portuguese-BR (pt-BR)
- Russian (ru)
- Chinese Simplified (zh-CN)
- Chinese Traditional (zh-TW)

Translation files: `i18n/kseditor_<locale>.ts`

---

## Testing

```powershell
# Run all tests (40 test suites)
cd build
ctest --output-on-failure -C Release

# Or run specific test
./bin/Release/ksEditor_tests --gtest_filter=AudioEditor*
```

**Test Coverage:** 40/40 test suites passing (audio, physics, graphics, file formats, scripting, UI, tools, etc.)

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style (C++ Core Guidelines)
- Commit message format
- Pull request process
- Testing requirements
- Documentation standards

---

## License

**MIT License** - see [LICENSE.txt](LICENSE.txt) for details.

---

## Links

- **Repository:** https://github.com/kseditor/kseditor
- **Issues:** https://github.com/kseditor/kseditor/issues
- **Discussions:** https://github.com/kseditor/kseditor/discussions
- **Wiki:** https://github.com/kseditor/kseditor/wiki

---

## Acknowledgments

- **Kunos Simulazioni** for Assetto Corsa
- **Qt Project** for Qt6 framework
- **Khronos Group** for Vulkan/OpenXR
- **FMOD** for audio reference implementation
- **All contributors** and the Assetto Corsa modding community

---

*ksEditor v1.16.4 — Professional modding toolkit for racing simulators*