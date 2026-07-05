# Changelog

All notable changes to ksEditor are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added
- **3D Viewport**: Real QOpenGLWidget-based 3D viewport replacing placeholder label — grid, axes, default cube mesh, orbit/pan/zoom camera controls, perspective/top/front/right views, solid/wireframe render modes, live FPS/triangle/vertex stats
- **Properties Panel**: Transform spinners (position/rotation/scale), name editor, visibility toggle, info tree with object ID/type/mesh status/children
- **Material Editor Panel**: Base color/emissive color pickers, blend mode dropdown, two-sided toggle, PBR sliders (roughness/metallic/opacity/emissive intensity) with live value labels
- **Transform Gizmo Widget**: Mode toggle buttons (Move/Rotate/Scale), position/rotation/scale spinner groups with X/Y/Z axes
- **Tool Palette Widget**: 9 tool buttons (Select, Move, Rotate, Scale, Add Cube/Sphere/Cylinder, Add Light, Add Camera) with exclusive checkable selection
- **Layer Panel Widget**: 3-column tree (name, visible checkmark, locked checkmark), click-to-toggle visibility/lock, layer selection tracking
- **DisplayEditor**: Complete implementation — loadFromFile (INI/Lua/JSON), saveToFile, loadFromLua with regex parsing, saveToLua, exportToIni, exportToJson, element CRUD (add/remove/update/get/clear), display settings (name/size/background), validateConfig with error reporting, type/data-source/color conversion helpers
- **ModuleManager**: Wired up 7 modules — ContentRepair (50), 3D Modeler (0), Physics Editor (0), Assets Library (10), Workshop (40), Mod Manager (35), License Plate Editor (35) — sorted by priority
- **CMakeLists**: All 60+ module .cpp files and 90+ common/ library files now registered — zero missing source files
- **File Format Parsersors**:
  - **CADOBJParser**: Wavefront OBJ parser — vertices, normals, UVs, faces (triangles/quads), negative index support, MTL material loading (Ka/Kd/Ks/Ns/d/illum, map_Kd/map_Ks/map_Bump), multi-object/group support
  - **GLBParser**: Binary glTF 2.0 parser — JSON chunk extraction (meshes/accessors/bufferViews/materials), BIN chunk binary data parsing, getVertices/getNormals/getTexCoords/getIndices helpers, uint16/uint32 index support
  - **FBXParser**: Autodesk FBX parser — auto-detect ASCII/Binary, ASCII geometry/material/connections parsing, Binary record parsing for Vertices/PolygonVertexIndex/Normals/UV, material properties (diffuse/specular/ambient/transparency/shading), mesh-to-material connections
- **Stub file cleanup**: Removed EditorWidgets_stub.cpp, Procedural_stub.cpp, TrackEditorWidget_stub.cpp; consolidated duplicate widget declarations between 3DModeling_widgets.h and 3DModeling_editor_widgets.h; added Generators_stub.cpp and 3DModeling_editor_widgets.cpp to build
- **Bug fix**: MeshModifier::rotateVertices used wrong index variable (`i` instead of `idx`)

### Changed
- Physics: improved vehicle model accuracy at high speeds
- ModuleManager: now loads 7 modules instead of 1, sorted by priority
- CMakeLists: all module and common/ source files registered (zero missing)
- 3DModeling panels: all placeholder labels replaced with functional widgets

### Fixed
- BackupSystem: pruning could skip newest backup under race condition
- AudioRecorder: Fixed Qt6 API usage and removed unused includes
- MeshModifier::rotateVertices: wrong index variable (`i` instead of `idx`)

---

## [2.0.0] – 2026-04-25

### Added
- **Full build system overhaul** — all 40+ `.cpp` modules now registered in CMakeLists.
- **Sound Editor** — complete FMOD Studio 1.08.12 integration: KN5 audio bank reader,
  waveform editor with FFT analysis, real-time audio processor, preset manager, metadata
  editor, GUID manager, car sound configurator.
- **PP Filters Editor** — full post-processing filter editor with split before/after
  preview, histogram, tone-curve controls, 6 parameter sections (Exposure, Color,
  Tone Curve, Bloom, Lens, DoF), and direct AC export.
- **License Plate Editor** — 10-country generator (IT, DE, UK, FR, ES, JP, US, AU, BR, CN)
  with live 7-segment preview, batch export (DDS/PNG/TGA), and preset system.
- **Font Creator** — bitmap glyph editor with 16×16 canvas, baseline/cap-height guides,
  atlas preview, and AC INI/BMFont/PNG export.
- **Display Editor** — 7/14/16-segment and LCD character display editor for AC
  dashboard instruments.
- **Assets Library** — full-featured asset browser with scan, search, tagging,
  thumbnail generation, and import/export.
- **PhysicsSimulator** — software Euler integrator with vehicle model, raycast,
  sphere overlap, joint management, and real-time playback.
- **TimelineEditor** — animation timeline with keyframe interpolation (linear, constant,
  cubic), multi-track, playback, baking, loop, and frame-rate control.
- **UndoRedo** — mergable command stack with limit, clean state, and full signal set.
- **AutoSave** — document manager, crash recovery via session persistence, and ring-buffer
  backup with configurable TTL and max-backup pruning.
- **ValidationSystem** — rule registry with built-in GEO/MAT/PHY rules and enable/disable
  per rule.
- **NotificationSystem** — singleton notification center with auto-dismiss TTL.
- **CommandPalette** — searchable command palette with relevance ranking.
- **BackupSystem** — project backup with auto-backup timer, restore, JSON index, and
  disk size tracking.
- **ThemeSystem** — dark/light built-in themes with full QPalette + QSS stylesheet
  generation; user theme JSON support.
- **SettingsSystem** — INI-backed settings with defaults, temporary overrides, group
  navigation, export/import JSON, and reset.
- **CacheManager** — memory + disk cache with TTL eviction and MD5 key hashing.
- **UpdateChecker** — GitHub Releases API integration with semantic version comparison
  and auto-check timer.
- **VersionControl** — Git wrapper with commit, branch, log, status, revert, and
  merge operations.
- **CloudSync** — OAuth2 upload/download with auto-sync queue and retry.
- **Collaboration** — WebSocket real-time collaboration with exponential reconnect,
  cursor/selection sharing, and chat.
- **PluginSystem** — Qt `.dll` and Python `.py` plugin loader with settings persistence,
  importer/exporter registration, and save/restore loaded list.
- **ScriptConsole** — QJSEngine-based JS console with auto-complete, history, and
  global object injection.
- **MacroSystem** — action-sequence recorder with per-step delay and repeat count.
- **TaskSystem** — QThreadPool-based async task runner with pause/resume, priority,
  and progress reporting.
- **StateMachine** — event-driven FSM with final states and condition-based transitions.
- **HotkeyActionSystem** — application-wide action registry with QShortcut binding
  and file-based profile persistence.
- **ShortcutProfile** — full shortcut profile system with per-module presets and
  conflict detection.
- **PreviewGenerator** — async thumbnail generation with disk cache and image/placeholder
  support.
- **SearchFilter** — full-text file index with multi-condition filter and relevance sort.
- **AssetManager** — MD5-keyed asset registry with MIME type detection, tagging, and
  JSON persistence.
- **ProjectTemplates** — 10 built-in AC car/track templates with folder scaffolding,
  `ui_car.json` / `ui_track.json` stubs.
- **ExportPresets** — 10 built-in export presets (KN5, FBX, OBJ, GLB, DDS BC7/BC3,
  WAV, FMOD BNK, Physics INI).
- **ImportExportFilters** — import and export filter registries with Qt file dialog
  string generation.
- **WizardSystem** — multi-page wizard with forward/back navigation and data collection.
- **MetadataSystem** — schema-based metadata catalog with field validation.
- **PerformanceOptimizer** — scene benchmarking, FPS estimation, and warning generation.
- **ConsolePanel** — ring-buffer message panel with per-type filter.
- **DebugTools** — module-level debug logger with breakpoint manager.
- **LoggingSystem** — Qt message handler integration with 7-day log rotation and
  5000-entry in-memory ring buffer.
- **`resources.qrc`** — Qt resource file with 89 SVG icons, QML pages, and SPIR-V
  shader stubs.
- **`assets/splash.png`** — 800×500 splash screen with module bar and loading indicator.
- **CI/CD** — GitHub Actions workflow for Windows 11 (MinGW + MSVC), Vulkan SDK,
  windeployqt, NSIS installer, static analysis with cppcheck, and GitHub Releases.
- **Unit tests** — 13 Qt Test test suites covering UndoRedo, ValidationSystem,
  StateMachine, PhysicsSimulator, CommandPalette, CacheManager, TimelineEditor,
  SettingsSystem, BackupSystem, RecentFilesManager, NotificationSystem, AssetManager,
  AutoSave.
- **i18n** — Qt Linguist `.ts` files for EN, IT, DE, JA with 80+ translated strings.
- **CPack/NSIS** — Windows 11 installer with desktop shortcut, version metadata,
  and uninstall support.

### Changed
- CMakeLists: `CMAKE_AUTORCC ON`; `Qt6::Sql` added; `ksppfilterseditor` and
  `src/core/scene` added to include dirs.
- WIN32 block rewritten: auto-generates `.rc` with version info + icon, compatible
  with both MinGW and MSVC.
- Vulkan block: auto-detects SDK, compiles shaders at build time via `glslc`.
- Compiler flags: `WINVER=0x0A00` / `_WIN32_WINNT=0x0A00` for Windows 11 API surface.
- windeployqt runs as post-build step automatically when found.

---

## [1.0.0] – 2026-01-15

### Added
- Initial project structure: Qt 6 + QML architecture, modular source layout.
- Core modules: 3D Modeler (ksmodeler), Physics Editor (ksphysicseditor),
  Sound Editor stub (kssoundeditor).
- Vulkan renderer foundation: `VulkanRenderer`, `VulkanShaderLoader`,
  `ShaderParamRegistry`, GLSL shader stubs.
- Scene graph: `SceneGraph`, `SceneMesh`, `SceneObject`.
- File formats: KN5 parser, ACD reader, INI reader/writer.
- QML UI: main window, editor home page, module pages.
- Python scripting bridge (10 utility scripts).
- Ribbon toolbar component.
- Database manager (`DatabaseManager`) and logging (`LogManager`).

---

[Unreleased]: https://github.com/kseditor/kseditor/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/kseditor/kseditor/compare/v1.0.0...v2.0.0
[1.0.0]: https://github.com/kseditor/kseditor/releases/tag/v1.0.0
