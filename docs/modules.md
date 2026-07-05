# Modules

ksEditor is organized into two layers: **core modules** (27 system-level subsystems in `src/core/`) and **application modules** (8 high-level modules in `src/modules/`).

## Core Modules (src/core/)

| Module | Files | Description |
|--------|-------|-------------|
| Audio | 62 | Audio engine, mixing, DSP, VST hosting, bank generation |
| mesh | 39 | Mesh operations, UV unwrap, sculpting, subdivision, skeleton |
| FileFormat | 30 | Format parsers, converters, format detector |
| assets | 26 | Asset manager, search engine, preview, cloud sync |
| tools | 47 | Backup, batch processing, collision mesh, LOD, macros, notifications |
| ui | 24 | UI widgets: asset list, file tree, font editor, splash, terminal |
| sys | 22 | Database, plugin/module managers, settings, undo, tasks |
| editor | 20 | Base editor, console, scripting, timeline, startup, debugger |
| Graphics | 22 | Vulkan renderer, scene graph, shaders, texture tools |
| modmanager | 20 | Mod/package manager, conflict resolution, content repair |
| material | 14 | Material library, shader manager, texture paint |
| AIEditor | 13 | AI behavior, telemetry trainer, multi-car AI |
| Config | 10 | CSP config editor, PP filter presets |
| network | 8 | Cloud sync, collaboration, WebSocket |
| Scripting | 8 | Python and Lua scripting hosts |
| animation | 5 | Animation system, physics system, shape keys |
| workshop | 5 | Workshop manager, QML bridge |
| weather | 4 | Weather config parser and editor |
| vcs | 4 | Git integration, status widget |
| FfbEditor | 4 | Force feedback config tool |
| ppfiltersEditor | 6 | Post-processing filters editor |
| textEditor | 8 | Code editor with syntax highlighting |
| archive | 2 | 7-Zip integration |
| formatToolsEditor | 2 | Format tools module |
| Math | 1 | Math core header |
| eventEditor | — | Career, championship, race config, special events |
| ServerConfigEditor | — | Server configuration |

## Application Modules (src/modules/)

| Module | Files | Description |
|--------|-------|-------------|
| modellingEditor | 49 | 3D modeling: boolean ops, UV unwrap, geometry nodes, car/track/character builders, viewport, rig generator, physics mesh, wizards |
| PhysicsEditor | 36 | Vehicle dynamics, tire models (Pacejka), aerodynamics, suspension, brakes, telemetry, setup editor/recommender |
| soundEditor | 28 | Audio editing: effects, mixing, analysis, AI engine, bank management |
| LiveryEditor | 16 | Car livery painting and editing |
| ShowroomEditor | 10 | 3D showroom/preview system |
| displayEditor | 6 | Display/segment editor for AC dashes |
| LicensePlatesEditor | 6 | License plate generation (22 countries) |
| fontEditor | 4 | Font atlas generation |

## Entry Points

- **`src/modules/ksEditor.cpp/.h`** — Main application module
- **`src/main.cpp`** — Application entry
- **`src/MainWindow.cpp/.h`** — Main window
- **`src/SDKBackend.cpp/.h`** — SDK integration backend

## Key Design Patterns

- Modules communicate through the module manager (`src/core/sys/ModuleManager`)
- QML bridges expose C++ modules to the Qt Quick UI layer
- Each module follows initialize/startup/shutdown lifecycle
- The `tools/` module provides cross-cutting services consumed by all other modules
