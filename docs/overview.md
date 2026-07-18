# ksEditor Overview

ksEditor is a comprehensive Assetto Corsa modding suite built with C++, Qt6, and Vulkan. It provides a full-featured integrated development environment for creating and editing audio, 3D models, textures, physics, and other game assets.

## Architecture

```
src/
├── core/           Core system modules (30+ subsystems)
│   ├── Audio/         Audio engine (65 files, audio graph, DSP library)
│   ├── Graphics/      Vulkan renderer & shaders (26 files + 12 shaders)
│   ├── FileFormat/    File format parsers & converters (36 files, +Alembic/USD/GLTF parsers)
│   ├── mesh/          Mesh operations & 3D tools (42 files)
│   ├── material/      Material library & shader manager (16 files, shader graph)
│   ├── sys/           System: plugin/module managers, settings, undo, tasks, transactions
│   ├── editor/        Base editor, console, scripting, timeline, ribbon tabs
│   ├── assets/        Asset manager, search, preview, cloud sync, dependency graph
│   ├── tools/         Tools framework (49 files, LOD/collision mesh generators)
│   ├── ui/            UI widgets (30 files, node graph editor, custom title bar)
│   ├── modmanager/    Mod/package manager & content repair
│   ├── Scripting/     Python & Lua scripting hosts, debugger, hot-reload, coroutines
│   ├── network/       Cloud sync, collaboration, WebSocket
│   ├── Config/        CSP config editor, schema-driven UI, config schema
│   ├── AIEditor/      AI behavior trees, telemetry trainer, multi-car AI
│   ├── animation/     Skeletal animation, IK, blend trees, shape keys, physics
│   ├── weather/       Weather editor, preview renderer
│   ├── eventEditor/   Career/championship/race/special events editors
│   ├── ServerConfigEditor/ Server configuration editor
│   ├── textEditor/    Code editor with LSP client, syntax highlighting, minimap
│   ├── FfbEditor/     Force feedback configuration tool
│   ├── ppfiltersEditor/ Post-processing filter editor
│   ├── 3dprint/       3D printing module (slicer, GCode, supports)
│   ├── VR/            OpenXR VR integration
│   ├── workshop/      Steam Workshop integration
│   ├── vcs/           Git version control integration
│   ├── archive/       7-Zip archive integration
│   ├── help/          Context help system with browser
│   ├── Scripting/     Coroutine manager, debugger frontend
│   └── formatTools/   Format conversion tools
├── modules/        High-level application modules (10 modules)
│   ├── modellingEditor/   3D modeling (49 files)
│   ├── soundEditor/       Audio editing (30 files, AC event bridge)
│   ├── PhysicsEditor/     Physics simulation (38 files, ERS/hybrid system)
│   ├── LiveryEditor/      Car livery painting (20 files, 3D viewport)
│   ├── ShowroomEditor/    3D showroom/preview (10 files)
│   ├── displayEditor/     Display/segment editor (6 files)
│   ├── LicensePlatesEditor/ License plate generation (6 files)
│   ├── fontEditor/        Font atlas generation (4 files)
│   └── VREditor/          VR viewport editor
├── plugins/        Plugin architecture
│   ├── base/PluginBase.h     Base interfaces
│   └── simulators/kunos/     Assetto Corsa integration (50+ files)
└── tests/          Unit tests (25+ files)
```

## Technology Stack

- **Language:** C++17
- **UI Framework:** Qt6 (QML + Widgets)
- **Graphics:** Vulkan + GLSL shaders
- **Scripting:** Python, Lua
- **Build:** CMake
- **External Libraries:** CGAL, Eigen, libigl, Bullet Physics, OpenVDB, STB, Mikktspace, 7-Zip
- **Target Platform:** Windows (Assetto Corsa modding)

## Feature Highlights

| Domain | Status | Details |
|--------|--------|---------|
| Audio Editor | 95% | FMOD/Wwise/GoldWave parity, audio graph editor, DSP library, VST hosting |
| 3D Modeler | 95% | Boolean ops, UV unwrap, subdivision, sculpting, knife tool, procedural generation, Vulkan viewport, rig generator |
| Physics Editor | 95% | Vehicle dynamics, tire models, aerodynamics, ERS/hybrid, DRS, damage, weather, fuel dynamics, Setup comparison |
| Livery Editor | 70% | DDS export, decal import, templates, undo/redo, color palette, 3D preview with scene graph + texture mapping, painter widget instantiated |
| Showroom Editor | 60% | Software PBR rasterization, car path picker, PP filter slots implemented, bug fixes |
| Help System | 90% | Full wiring: menu integration, F1 context help, tutorials, QuickStart with "Don't show again", 20+ help contexts registered, help browser |
| Event Editor | 30% | Career, championship, race config, special events editors implemented |
| Server Config Editor | 40% | Server configuration module, CSP shader compiler working |
| 3D Printing | 50% | Slicer engine, GCode generation, support generation, printer profiles |
| VR Editor | 35% | OpenXR integration, VR viewport renderer, input handling |
| CspShaderCompiler | 85% | Core compile pipeline working |
| Build System | 95% | CMake, all source files registered via GLOB_RECURSE |
| Testing | 95% | 39/39 tests pass (all configured) |
| File Formats | 95% | OBJ, GLB, FBX, STL, KN5, ACD, WAV, OGG, FLAC, Alembic, USD, more |
