# ksEditor Overview

ksEditor is a comprehensive Assetto Corsa modding suite built with C++, Qt6, and Vulkan. It provides a full-featured integrated development environment for creating and editing audio, 3D models, textures, physics, and other game assets.

## Architecture

```
src/
├── core/           Core system modules (27 subsystems)
│   ├── Audio/         Audio engine (62 files)
│   ├── Graphics/      Vulkan renderer & shaders (22 files + 12 shaders)
│   ├── FileFormat/    File format parsers & converters (30 files)
│   ├── mesh/          Mesh operations & 3D tools (39 files)
│   ├── material/      Material library & shader manager (14 files)
│   ├── sys/           System: plugin/module managers, settings, undo, tasks
│   ├── editor/        Base editor, console, scripting, timeline
│   ├── assets/        Asset manager, search, preview, cloud sync
│   ├── tools/         Tools framework (47 files)
│   ├── ui/            UI widgets (24 files)
│   ├── modmanager/    Mod/package manager & content repair
│   ├── Scripting/     Python & Lua scripting hosts
│   ├── network/       Cloud sync, collaboration, WebSocket
│   └── ...            19 more subsystems
├── modules/        High-level application modules (8 modules)
│   ├── modellingEditor/   3D modeling (49 files)
│   ├── soundEditor/       Audio editing (28 files)
│   ├── PhysicsEditor/     Physics simulation (36 files)
│   ├── LiveryEditor/      Car livery painting (16 files)
│   ├── ShowroomEditor/    3D showroom/preview (10 files)
│   ├── displayEditor/     Display/segment editor (6 files)
│   ├── LicensePlatesEditor/ License plate generation (6 files)
│   └── fontEditor/        Font atlas generation (4 files)
├── plugins/        Plugin architecture
│   ├── base/PluginBase.h     Base interfaces
│   └── simulators/kunos/     Assetto Corsa integration (50+ files)
└── tests/          Unit tests (22 files)
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
| Audio Editor | 100% | FMOD/Wwise/GoldWave parity, 62-file audio engine |
| 3D Modeler | 98% | Boolean ops, UV unwrap, subdivision, sculpting |
| Physics Editor | 88% | Vehicle dynamics, tire models, aerodynamics |
| Build System | 100% | CMake, 176 .cpp / 280 .h files registered |
| File Formats | Full | OBJ, GLB, FBX, STL, KN5, ACD, WAV, OGG, FLAC, more |
