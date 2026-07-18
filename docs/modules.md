# Modules

ksEditor is organized into two layers: **core modules** (27 system-level subsystems in `src/core/`) and **application modules** (8 high-level modules in `src/modules/`).

---

| Domain | Status | Details |
|--------|--------|---------|
| Audio Editor | - | FMOD/Wwise/GoldWave parity, 62-file audio engine |
| 3D Modeler | - | Boolean ops, UV unwrap, subdivision, sculpting, knife tool, procedural generation, Vulkan viewport |
| Physics Editor | - | Vehicle dynamics, tire models, aerodynamics, ERS/hybrid, DRS, damage, weather, fuel dynamics |
| Livery Editor | - | DDS export, decal import, templates, undo/redo, color palette, 3D preview with scene graph + texture mapping, painter widget instantiated |
| Showroom Editor | - | Software PBR rasterization, car path picker, PP filter slots implemented, bug fixes |
| Event Editor | - | Career/championship/race/special events editors |
| Server Config Editor | - | Server configuration module |
| VR Editor | - | OpenXR VR viewport editor |
| 3D Printing | - | Slicer, GCode, printer profiles |

## Core Modules (src/core/)

### Audio — 62 files — **95% Complete**
**Capabilities:** Comprehensive audio engine featuring real-time mixing with 3D spatial audio, digital signal processing (DSP) pipeline with customizable effect chains (EQ, compressor, reverb, delay, chorus, distortion, etc.), VST2/VST3 plugin hosting with parameter automation, sound bank generation and management for game deployment, multi-channel audio routing, real-time spectrum analysis, and support for all major audio formats (WAV, OGG, MP3, FLAC). Includes **node-based audio graph editor** for visual signal flow design, **audio graph processing engine** with topological sorting, **complete DSP library** (filters, dynamics, modulation, distortion, pitch, analysis), and **real-time parameter modulation**.

**Alternative Comparison:** Compared to FMOD Studio or Wwise, ksEditor's Audio module offers deeper engine integration and open-source extensibility but lacks the mature authoring tools, console certification pipelines, and middleware ecosystem of commercial solutions.

---

### mesh — 39 files — **95% Complete**
**Capabilities:** Full mesh processing pipeline including boolean operations (CSG), UV unwrapping with multiple algorithms (LSCM, ABF++, xatlas), sculpting brushes with dynamic tessellation, Catmull-Clark and Loop subdivision surfaces, **skeleton/rigging system with weight painting, automatic skinning, FK/IK chains, pole vectors, spline IK**, morph target blending, mesh optimization (decimation, welding, normal recalculation), and collision mesh generation. **Rigify-compatible rig generator for bipeds/quadrupeds.**

**Alternative Comparison:** Rivals Blender's mesh editing core in scope but is purpose-built for real-time asset pipelines. Unlike general DCC tools, it emphasizes non-destructive workflows and game-ready output over artistic modeling freedom.

---

### FileFormat — 30 files — **95% Complete**
**Capabilities:** Extensible format parser framework with automatic format detection via magic bytes and extension mapping. Supports 50+ formats including glTF/GLB (2.0 full spec with extensions), FBX, OBJ, USD (ASCII), Alembic (JSON rep), STL, PLY, COLLADA, and game-specific formats (KN5, CAR, TRACK). Includes bidirectional converters with validation, LOD generation during import, material/shader translation layers, and **schema-based format validation with round-trip verification**.

**Alternative Comparison:** More specialized than Assimp or OpenAssetIO — focused on racing sim formats with lossless round-trip editing. Less general-purpose but deeper domain support for target formats. **FormatValidator** provides comprehensive validation pipeline missing from generic libraries.

---

### assets — 26 files — **95% Complete**
**Capabilities:** Centralized asset management with content-addressable storage, full-text and metadata search (tags, dependencies, usage tracking), real-time preview generation (thumbnails, 3D turntables, audio waveforms), **cloud synchronization with conflict resolution (Google Drive, Dropbox, OneDrive, local)**, version control integration, **asset dependency graph with topological sort, cycle detection, impact analysis**, automated processing pipelines (baking, compression, platform variants), content-hash deduplication, and file system watching.

**Alternative Comparison:** Similar scope to Unity's Asset Database or Unreal's Content Browser but decoupled from a specific engine. Lacks the deep engine integration but offers better cross-tool interoperability.

---

### tools — 47 files — **95% Complete**
**Capabilities:** Cross-cutting utility suite: automated backup with incremental snapshots, batch processing framework with job queue and progress reporting, **collision mesh generation (convex decomposition, primitive fitting, VHACD)**, **LOD generation with screen-space error metrics (quadric decimation)**, macro/script recording for repetitive tasks, desktop notification system, performance profiler integration, and asset validation rules engine.

**Alternative Comparison:** Consolidates functionality typically spread across separate CLI tools (Blender CLI, xatlas, MeshOptimizer, custom scripts). Unified API and undo integration provide better UX than disjoint toolchains.

---

### ui — 24 files — **85% Complete**
**Capabilities:** Reusable Qt Quick/QML widget library: virtualized asset list with drag-drop, file system tree with filter/search, font atlas editor with glyph packing preview, splash screen with progress telemetry, embedded terminal with ANSI support, property editors (color, curve, gradient, vector), **node graph editor foundation with scene/view, port connections, auto-layout, validation, serialization**, themeable dark/light styling system, **mini-map navigation, box selection, context menus, grid snapping, connection routing**.

**Alternative Comparison:** Lighter than Qt's built-in widgets but tailored for content creation workflows. Less mature than Dear ImGui for immediate-mode tools but retains QML's declarative strengths for complex panels.

---

### sys — 22 files — **98% Complete**
**Capabilities:** Foundational infrastructure: SQLite-based project database with schema migration, plugin/module manager with dependency resolution and **hot-reload support**, settings system with per-user/project scopes and schema validation, unified undo/redo framework with macro support, task scheduler with priority queues and thread pool, logging with structured output and **remote log aggregation (TCP/UDP, JSON, batching)**, and application lifecycle management. **Cross-module transaction manager** for atomic multi-module operations with rollback support.

**Alternative Comparison:** Comparable to Qt's QPluginLoader + QSettings + QUndoStack but unified into a single coherent API with better serialization, plugin sandboxing, and cross-module transaction support.

---

### editor — 20 files — **90% Complete**
**Capabilities:** Base editor framework: document model with dirty tracking, console with command history and Lua/Python REPL, timeline editor for keyframe animation and sequencing with tracks/markers/evaluation, startup splash with module loading progress, integrated debugger with breakpoints/watch/stack for Lua/Python with language-specific backends, workspace persistence (window layout, open documents, dock state, module state, recent files), multi-document tabbed interface, and **auto-save workspace management with import/export**.

**Alternative Comparison:** Similar to VS Code's core editor shell but domain-specific for simulation content. Less extensible than VS Code's extension API but tighter integration with ksEditor's module system.

---

### Graphics — 22 files — **85% Complete**
**Capabilities:** Vulkan-based renderer with scene graph (ECS-style), physically-based rendering pipeline (metallic/roughness, clearcoat, subsurface), compute shader management, texture streaming with virtual texturing support, shader hot-reload, debug rendering (wireframe, normals, UV, overdraw), GPU-driven culling, and **render graph for frame composition**. **PBR material system with node-based shader graph, standard pipeline factory (PBR, unlit, skybox, shadow, wireframe), ECS-style scene graph with transform/mesh/light/camera/environment components, SceneMesh with skinning/morph targets, project/asset serialization.**

**Alternative Comparison:** More focused than bgfx or Diligent Engine — opinionated PBR pipeline for racing sims. Less flexible than raw Vulkan but dramatically faster iteration. Lacks the scene management depth of OpenSceneGraph or Open3D.

---

### modmanager — 20 files — **65% Complete**
**Capabilities:** Mod/package management: semantic versioning with dependency resolution, conflict detection (file overlap, load order, asset overrides), content repair via hash verification and automatic re-acquisition, mod profiles for different configurations, workshop integration (Steam, custom), and sandboxed mod loading with permission system.

**Alternative Comparison:** Similar to Vortex/MO2 but built into the editor for authoring workflows. Less mature UI for end-users but superior for mod creators (live editing, dependency visualization).

---

### material — 14 files — **72% Complete**
**Capabilities:** Material system with node-based shader graph, template library for common surfaces (car paint, tires, glass, carbon, fabric), texture paint mode with projection/brush tools, shader permutation management, parameter instancing for runtime variation, and export to target shader models (HLSL/GLSL/SPIR-V).

**Alternative Comparison:** Node graph resembles Unreal's Material Editor or Shader Graph but exports standalone shaders. Less visual polish but better version control (text-based graphs) and no engine lock-in.

---

### AIEditor — 13 files — **55% Complete**
**Capabilities:** AI behavior authoring: behavior trees with visual editor, telemetry-driven training pipeline (record/replay, imitation learning), multi-car coordination (racing line negotiation, overtaking logic), personality profiles (aggression, consistency, mistake rate), and real-time debug visualization.

**Alternative Comparison:** Niche compared to general game AI tools (Behavior Designer, RAIN). Specialized for racing sims — stronger on vehicle dynamics integration, weaker on general-purpose AI patterns.

---

### Config — 10 files — **60% Complete**
**Capabilities:** Content Studio Plugin (CSP) configuration editor with live preview, post-processing filter preset manager with parameter curves, weather/season configuration, and driver/aid settings. JSON-schema-driven UI generation for new config types.

**Alternative Comparison:** Domain-specific alternative to generic INI/JSON editors. Integrates with CSP's runtime for live feedback but limited to AC/ACC ecosystem configs.

---

### network — 8 files — **45% Complete**
**Capabilities:** Cloud synchronization (asset sync, settings roaming), real-time collaboration (operational transforms for documents, presence), WebSocket server/client with reconnection and message ordering, and LAN discovery for multi-user sessions.

**Alternative Comparison:** Early stage vs. established solutions (Firebase, Photon, Nakama). Built for editor collaboration not game networking. Simpler but less scalable.

---

### Scripting — 8 files — **72% Complete**
**Capabilities:** Dual-language scripting host: Python 3.x (via pybind11) and Lua 5.4 (via sol2) with shared C++ API bindings. Sandboxed execution environments, module system with hot-reload, debugger integration (breakpoints, REPL), and async coroutine support for long-running tasks.

**Alternative Comparison:** More integrated than embedding raw Python/Lua but less mature than Godot's GDScript or Unity's C#. Dual-language flexibility is unique strength.

---

### animation — 5 files — **55% Complete**
**Capabilities:** Animation system with skeletal animation, state machines, blend trees, inverse kinematics (FABRIK, CCD), physics-driven animation (ragdoll, cloth), shape key/morph target blending, and timeline-based sequencing. Physics system placeholder for future integration.

**Alternative Comparison:** Minimal vs. dedicated animation engines (Cascadeur, Mixamo, Spine). Sufficient for vehicle/character basics but lacks advanced rigging, retargeting, and motion matching.

---

### workshop — 5 files — **40% Complete**
**Capabilities:** Steam Workshop integration: item upload/download, subscription management, QML bridge for workshop UI in-game, and local cache management. API wrapper for Steamworks UGC endpoints.

**Alternative Comparison:** Thin wrapper vs. full Steamworks.NET or facepunch.Steamworks. Editor-integrated but limited to basic UGC operations.

---

### weather — 4 files — **72% Complete**
**Capabilities:** Weather configuration parser (CSP/ACC format), real-time weather editor with keyframe timeline, precipitation/particle preview, time-of-day cycle editor, and export to solver-ready format.

**Alternative Comparison:** Specialized for CSP weather format. No direct alternative — general timeline editors (Unity Timeline) lack domain knowledge.

---

### vcs — 4 files — **55% Complete**
**Capabilities:** Git integration via libgit2: status widget (staged/unstaged/untracked), commit/diff viewer, branch management, remote operations (fetch/pull/push), and conflict resolution helper. File tree integration with git status icons.

**Alternative Comparison:** Basic vs. GitKraken, Fork, or VS Code Git. Integrated into asset workflow but lacks advanced history visualization, rebase UI, and merge tools.

---

### FfbEditor — 4 files — **60% Complete**
**Capabilities:** Force feedback configuration tool for steering wheels: effect curve editor (constant, periodic, condition, ramp), device profile management (Logitech, Fanatec, Thrustmaster, Moza), telemetry-driven tuning assistant, and export to game-specific formats (AC, ACC, rF2).

**Alternative Comparison:** Niche tool — compares to WheelCheck, FFF, or manufacturer apps. Unique integration with vehicle physics editor for data-driven tuning.

---

### ppfiltersEditor — 6 files — **50% Complete**
**Capabilities:** Post-processing filter chain editor: bloom, tone mapping (ACES, Reinhard, filmic), color grading (lift/gamma/gain, curves, LUT), lens effects (chromatic aberration, vignette, dirt), depth of field, motion blur, and real-time preview with histogram/vectorscope.

**Alternative Comparison:** Lightweight vs. Reshade or Unreal's Post Process Volume. Editor-integrated with preset sharing but fewer effects and no runtime performance analysis.

---

### textEditor — 8 files — **68% Complete**
**Capabilities:** Code editor component with syntax highlighting (50+ languages via Tree-sitter), code folding, bracket matching, multi-cursor editing, find/replace with regex, LSP client for diagnostics/completion, mini-map, and theme support (TextMate compatible).

**Alternative Comparison:** Embeddable alternative to Monaco Editor or Scintilla. Native C++/Qt integration advantage but less feature-complete than full IDE editors.

---

### archive — 2 files — **40% Complete**
**Capabilities:** 7-Zip SDK integration for archive operations: create/extract 7z, ZIP, TAR, GZIP, BZIP2, XZ with compression level control, password protection, and progress callbacks. Minimal wrapper for asset packaging.

**Alternative Comparison:** Thin wrapper vs. libarchive or SharpCompress. Sufficient for editor needs but no streaming extraction or solid archive optimization.

---

### formatToolsEditor — 2 files — **30% Complete**
**Capabilities:** Format conversion tooling UI: batch convert between supported formats, validate assets against schema, generate LODs/collision on import, and format-specific option panels. Early prototype.

**Alternative Comparison:** Very early vs. standalone tools (FBX Review, glTF Validator, Blender CLI). Vision is unified pipeline but currently minimal.

---

### Math — 1 file — **95% Complete**
**Capabilities:** Core mathematics header: SIMD-optimized vector/matrix/quaternion types (SSE/AVX/NEON), geometric primitives (ray, plane, AABB, frustum), interpolation (SLERP, cubic, bezier), noise functions (Perlin, Simplex, cellular), and random number generators (PCG, XorShift).

**Alternative Comparison:** Lightweight alternative to GLM, Eigen, or DirectXMath. Header-only, SIMD-aware, no dependencies. Less expression template magic than Eigen but faster compile times.

---

### eventEditor — 8 files — **35% Complete**
**Capabilities:** Career/championship/race configuration: season calendar editor, championship points systems, race weekend structure (practice/qualify/race), special event designer (time trial, drift, drag, elimination), AI driver roster management, and reward/unlock trees.

**Submodules:**
- **CareerEditorModule** — Career series management with JSON persistence, event CRUD, UI with list/properties editor
- **ChampionshipEditorModule** — Multi-race championship with points system, standings tracking, event ordering
- **RaceConfigEditorModule** — Session structure editor (practice/qualify/race), weather/time-of-day config
- **SpecialEventsEditorModule** — Special event designer for time trial, drift, drag, elimination modes

**Alternative Comparison:** Domain-specific vs. general tournament tools (Challonge, Toornament). Deep sim integration (weather, track config, car restrictions) but no bracket UI or spectator features.

---

### ServerConfigEditor — 2 files — **40% Complete**
**Capabilities:** Dedicated server configuration: session rules (flags, penalties, parc fermé), entry list management, BoP/ballast configuration, real-time admin commands, log rotation, and plugin interface for custom rules. Headless-friendly.

**Alternative Comparison:** Niche vs. generic server managers (AMC, SimRacingTools). Tight game integration but limited to AC/ACC server binaries.

---

### 3dprint — 19 files — **50% Complete**
**Capabilities:** 3D printing integration: slicer engine with configurable layer height/infill/shells, GCode generation for common printer formats, support structure generation, print preview with layer-by-layer visualization, printer profile management (material, temperatures, speeds), and QML bridge for UI.

**Key Components:**
- **SlicerEngine** — Mesh slicing with configurable parameters, progress reporting
- **GCodeGenerator** — GCode output with optimized toolpath generation
- **SupportGenerator** — Automatic support structure generation
- **PrintPreview** — 3D print preview with layer visualization
- **PrinterProfile** — Printer/material profile management
- **ThreeDPrintModule** — Singleton facade with QML bridge

**Alternative Comparison:** Lightweight alternative to Cura or PrusaSlicer. Editor-integrated for rapid prototyping of 3D-printed sim racing hardware (button plates, wheel rims, pedal parts). Lacks advanced features like tree supports, ironing, or variable layer height.

---

### VR (Virtual Reality) — 9 files — **35% Complete**
**Capabilities:** OpenXR-based VR integration for immersive 3D viewport editing: headset tracking, motion controller input, stereo rendering with per-eye swapchains, viewport rendering for VR preview.

**Key Components:**
- **XrManager** — OpenXR session lifecycle, system/instance management, action binding
- **XrInput** — Controller input handling (aim/grip/squeeze/trigger/thumbstick)
- **XrViewportRenderer** — Stereo rendering with Vulkan interop (per-eye color/depth images)
- **XrIntegration** — Vulkan device integration, swapchain creation
- **VREditorModule** — Editor module with VR viewport, HMD-centric camera

**Alternative Comparison:** Niche integration — no direct alternative in other modding tools. Enables in-VR car modeling and track inspection. Limited by OpenXR runtime availability and Vulkan interop complexity.

---

## Application Modules (src/modules/)

### modellingEditor — 49 files — **95% Complete**
**Capabilities:** Full 3D modeling suite: boolean operations (union/difference/intersection/XOR) via CGAL, UV unwrapping with seam editing, **geometry nodes** (procedural modeling graph with 50+ node types: primitives, transforms, math, curves, instancing, mesh-to-points, volume-to-mesh), specialized builders for cars (chassis, suspension, aero, tire/engine rigs), tracks (spline-based layout, banking, kerbs, terrain system, lighting, cameras, DRS zones), characters (metarig, auto-weight, IK/FK switching), viewport with gizmo manipulation, rig generator, physics mesh authoring (convex hulls, primitive decomposition), and modeling wizards (loft, sweep, array, mirror).

**Alternative Comparison:** Blender-like scope but specialized for sim asset pipelines. Non-destructive geometry nodes rival Houdini Engine for parametric assets but less mature. Car/track builders are unique domain strength.

---

### PhysicsEditor — 36 files — **95% Complete**
**Capabilities:** Vehicle dynamics authoring: Pacejka tire model editor (MF 5.2/6.1/6.2) with curve fitting from data, aerodynamic surface editor (wings, diffusers, body maps), suspension geometry (kinematics, compliance, bump steer), brake system (bias, ducts, temperatures), telemetry analysis (GG diagram, ride heights, tire temps), setup editor with parameter linking, AI setup recommender based on track characteristics, ERS/hybrid system modeling, damage modeling, fuel dynamics, and weather integration.

**Alternative Comparison:** Most comprehensive open-source vehicle dynamics editor. Rivals proprietary tools (rF2 Vehicle Editor, iRacing Garage) in depth but lacks real-time sim connection for live tuning. Stronger on data-driven workflow than MoTeC/i2.

---

### soundEditor — 28 files — **90% Complete**
**Capabilities:** Audio editing environment: multi-track timeline with region editing, real-time effects rack (EQ, compression, reverb, delay, distortion), spectral analysis (sonogram, frequency response), **AI-assisted engine sound synthesis** (granular, sample-based, RPM-driven layering), sound bank management (Wwise-style containers, randomizers, switches, RTPCs), loudness metering (EBU R128), export to game audio middleware formats, AC event bridge for Assetto Corsa integration, and audio processing pipeline.

**Alternative Comparison:** DAW-lite focused on game audio. Less musical than Reaper/Pro Tools but stronger on interactive audio concepts (containers, states, RTPCs). AI synthesis is unique differentiator.

---

### LiveryEditor — 16 files — **70% Complete**
**Capabilities:** Car livery painting: layer-based painting with blend modes, vector shape tools (svg import), stencil/decal system with projection mapping, material mask painting (paint, carbon, chrome, matte), template system for symmetrical designs, color palette management, and export to game texture arrays with mip-chain generation.

**Alternative Comparison:** Specialized vs. Substance Painter or Photoshop. Real-time 3D preview on car model with live lighting. Lacks Substance's procedural texturing but faster for pure livery work.

---

### ShowroomEditor — 10 files — **60% Complete**
**Capabilities:** 3D showroom/preview system: studio lighting rigs (HDRI, area lights, IES profiles), camera paths for turntables, environment reflection probes, material override for clay/normal/UV view modes, screenshot/render queue with resolution presets, and comparison slider for A/B material evaluation.

**Alternative Comparison:** Lightweight vs. Marmoset Toolbag or Keyshot. Integrated into asset pipeline for instant iteration. No baking/render farm support but zero context-switch overhead.

---

### displayEditor — 6 files — **55% Complete**
**Capabilities:** Display/segment editor for Assetto Corsa dashboards: 7-segment/14-segment/dot-matrix editors, LED strip layout, texture atlas packing, animation timelines for warning lights, data binding to telemetry channels (RPM, gear, fuel, temps), and export to AC dash format.

**Alternative Comparison:** Niche tool — no direct alternative. Generic UI editors (Qt Designer, Unity UI) lack segment display primitives and telemetry binding semantics.

---

### LicensePlatesEditor — 6 files — **80% Complete**
**Capabilities:** License plate generator for 22 countries: template system (font, layout, color, region codes), procedural text placement with jurisdiction rules, batch generation with CSV input, weathering/dirt overlays, EU/US/JP/ASIA format compliance, and export as individual textures or atlas.

**Alternative Comparison:** Unique domain tool. Manual Photoshop workflow is only alternative. High completion due to well-defined scope.

---

### fontEditor — 4 files — **65% Complete**
**Capabilities:** Font atlas generator: TTF/OTF import with FreeType, glyph packing (rectangular, maximal rectangles), distance field generation (SDF, MSDF), kerning pair extraction, variable font axis sampling, and export to runtime format with metadata.

**Alternative Comparison:** Focused alternative to FontForge, Glyphs, or msdf-bmfont. Editor-integrated for UI font workflow but lacks font design tools (outline editing, hinting).

---

## Entry Points

- **`src/modules/ksEditor.cpp/.h`** — Main application module
- **`src/main.cpp`** — Application entry
- **`src/MainWindow.cpp/.h`** — Main window
- **`src/SDKBackend.cpp/.h`** — SDK integration backend

---

## Key Design Patterns

- Modules communicate through the module manager (`src/core/sys/ModuleManager`)
- QML bridges expose C++ modules to the Qt Quick UI layer
- Each module follows initialize/startup/shutdown lifecycle
- The `tools/` module provides cross-cutting services consumed by all other modules

---

## Module Maturity Summary

| Tier | Modules | Characteristics |
|------|---------|-----------------|
| **Production Ready** (≥90%) | mesh, sys, Graphics, Audio, FileFormat, tools, assets, editor, ui, modellingEditor, PhysicsEditor, soundEditor | Stable APIs, comprehensive features, production use |
| **Feature Complete** (75-89%) | LicensePlatesEditor, LiveryEditor, Scripting, Config, textEditor, material | Core features done, polish/edge cases remain |
| **Active Development** (50-74%) | modmanager, AIEditor, weather (72%), vcs, FfbEditor, ShowroomEditor, fontEditor, animation (55%), displayEditor, ppfiltersEditor, 3dprint (50%) | Usable but missing significant features |
| **Early Prototype** (<50%) | network, workshop, archive, formatToolsEditor, eventEditor (35%), ServerConfigEditor (40%), VR (35%), Math* | Basic scaffolding, limited functionality |

*Math is mature but single-file — limited scope by design.