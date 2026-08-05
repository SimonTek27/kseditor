# ksAssettoCorsaEVO.dll — Product Overview

## Current Status: INITIAL IMPLEMENTATION

`ksAssettoCorsaEVO.dll` has been **initially implemented** as of July 2026. The plugin skeleton, content discovery, kspkg parser, game launcher, and QML bridges are all in place. The CMake target builds alongside the existing AC1 plugin.

---

## 1. Background: The Existing ksAssettoCorsa Plugin

The codebase contains `ksAssettoCorsa.dll` — a simulator integration plugin for the **original Assetto Corsa (AC1)**. It is currently the only simulator plugin in ksEditor.

### Architecture

```
kseditor.exe
  └── PluginManager (loads .dll from bin/plugins/)
        └── ksAssettoCorsa.dll  (SHARED library, ~40+ source files)
              ├── Extern "C" API: 8 entry-point functions
              │     getPluginId, getName, getVersion, getDescription,
              │     initializePlugin, shutdownPlugin, isAvailable,
              │     getInstallPath/setInstallPath
              ├── Backed by KsPlugin (singleton in kseditor_lib)
              └── 18+ sub-modules:
                    ContentBrowser, KsContentPaths, KsAssettoCorsaRunner,
                    QmlBridges, SetupComparison, WorkshopModule,
                    AssetsLibraryModule, ACSharedMemory,
                    ACLivePreviewBridge, KN5Parser, KN5Decrypt,
                    ACDParser, CspConfigParser, CspConfigHandler,
                    FBXExporter, Kn5Previewer, LODExporter,
                    KsShaders, KSAnimFormat, KNHFormat
```

### Linked Libraries

| Library | Purpose |
|---------|---------|
| `kseditor_lib` | Core editor (KsPlugin, PluginBase, PluginManager) |
| `Qt6::Core` | Strings, files, JSON |
| `Qt6::Gui` | Images, painting |
| `Qt6::Widgets` | Dock widgets, dialogs |
| `Qt6::Network` | HTTP (Workshop downloads, cloud sync) |
| `Qt6::Qml` / `Qt6::Quick` | QML UI layer |
| `Qt6::Sql` | SQLite (Assets Library) |
| `Qt6::OpenGL` | OpenGL rendering (shaders, mesh preview) |
| `Qt6::Multimedia` | Audio playback |
| `opengl32`, `dwmapi`, `dwrite` | Windows platform |

### Key Features
- **Content Discovery**: Scan AC installation for cars/tracks/skins/drivers
- **Game Launcher**: Launch AC with command-line arguments
- **Steam Workshop**: Browse, search, download, check updates/dependencies
- **Assets Library**: SQLite-based asset DB with cloud sync, tags, thumbnails
- **KN5 Parsing/Decryption**: Read/write `.kn5` models, decrypt CSP-encrypted files
- **ACD Parsing**: Extract/repack `data.acd` archives
- **Shared Memory Telemetry**: Real-time physics/graphics data via memory-mapped files
- **Live Audio Preview**: Drive audio engine from live telemetry (RPM, throttle, etc.)
- **Setup Comparison**: Load/save/compare car setups
- **QML Bridges**: 5 bridge classes exposing C++ to QML (Content, CSP, Mesh, MeshOps, Livery)

---

## 2. Assetto Corsa EVO — The Game

**Assetto Corsa EVO** (ACE) is the next-generation racing simulator from Kunos Simulazioni, currently in Early Access on Steam (App ID: 3058630). The ACE SDK was released in **June 2026** with version 0.7 of the game.

### ACE SDK (Version 0.7+)

The ACE SDK allows creation and import of **custom vehicles for offline modes**, using development processes similar to Kunos's internal pipeline. Key facts:
- Uses **protobuf** (Protocol Buffers) for the majority of game content/files
- Game content is packaged in **`.kspkg`** archives (KSPackage format)
- Modding is possible by unpacking `content.kspkg` to the game root
- **No track/decal modding support yet** — only custom cars for offline
- **No multiplayer custom cars** in the current SDK version
- SDK documentation on Kunos forum: [ACE SDK Documentation](https://www.assettocorsa.net/forum/index.php?threads/ace-sdk-documentation.83772/)

### File Format Differences

| Feature | Assetto Corsa (AC1) | Assetto Corsa EVO (ACE) |
|---------|---------------------|------------------------|
| Model format | `.kn5` (proprietary binary) | `.kspkg` (protobuf-based archives) |
| Archive format | `.acd` (data archives) | `.kspkg` (unified content packages) |
| Config format | `.ini` (per-file) | Protobuf + JSON |
| Shaders | Custom GLSL (ksPerPixel, etc.) | Unknown (likely PBR via protobuf) |
| Telemetry | Shared memory (memory-mapped files) | Unknown |
| Steam Workshop | Yes | Likely |

### Community Tools for ACE
- **ACEvo.Package** (C#): KSPackage archive tool by Nenkai — [github.com/Nenkai/ACEvo.Package](https://github.com/Nenkai/ACEvo.Package)
- **kspkg-viewer** (C++): KSPackage viewer/extractor by sa413x — [github.com/sa413x/kspkg-viewer](https://github.com/sa413x/kspkg-viewer)
- **ace-kspkg** (Python): Python kspkg tooling by ntpopgetdope

---

## 3. ksAssettoCorsaEVO.dll — Proposed Architecture

A hypothetical `ksAssettoCorsaEVO.dll` would follow the same plugin pattern as `ksAssettoCorsa.dll` but target ACE instead of AC1.

### CMake Target (Model)

```cmake
add_library(ksAssettoCorsaEVO SHARED
    ksAssettoCorsaEVO.cpp
    ACEPackageParser.cpp/h
    ACEPackageParser.h
    ACEPackageManager.cpp/h
    ACRunner.cpp/h
    ACEQmlBridge.cpp/h
    ...
)
target_link_libraries(ksAssettoCorsaEVO PRIVATE
    kseditor_lib
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick
    Qt6::Sql Qt6::OpenGL Qt6::Multimedia
)
set_target_properties(ksAssettoCorsaEVO PROPERTIES
    OUTPUT_NAME "ksAssettoCorsaEVO"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
)
add_dependencies(kseditor ksAssettoCorsaEVO)
```

### Plugin Entry-Point (model)

```cpp
extern "C" {
    KS_ASSETTOCORSAEVO_API const char* getPluginId() { return "ksAssettoCorsaEVO"; }
    KS_ASSETTOCORSAEVO_API const char* getPluginName() { return "Assetto Corsa EVO Plugin"; }
    KS_ASSETTOCORSAEVO_API const char* getPluginVersion() { return "0.1.0"; }
    KS_ASSETTOCORSAEVO_API const char* getPluginDescription() {
        return "Assetto Corsa EVO content management and editing plugin for ksEditor";
    }
    KS_ASSETTOCORSAEVO_API bool initializePlugin() { ... }
    KS_ASSETTOCORSAEVO_API void shutdownPlugin() { ... }
    KS_ASSETTOCORSAEVO_API bool isPluginAvailable() { ... }
}
```

### Required Sub-Modules

| Module | Purpose | Based On |
|--------|---------|----------|
| `ACEPluginEntry.cpp` | Plugin lifecycle (C API) | `ksAssettoCorsa.cpp` |
| `ACEPackageParser` | Parse `.kspkg` protobuf archives | Community tools (ACEvo.Package) |
| `ACEContentManager` | Discover installed cars/tracks | `KsContentPaths` |
| `ACERunner` | Launch ACE with parameters | `KsAssettoCorsaRunner` |
| `ACEQmlBridge` | QML-C++ interop for UI | `QmlBridges` |
| `ACESharedMemory` | Read ACE telemetry | `ACSharedMemory` |
| `ACELivePreview` | Audio preview from live data | `ACLivePreviewBridge` |
| `ACESetupComparison` | Setup file compare | `SetupComparison` |
| `ACEWorkshop` | Steam Workshop integration | `WorkshopModule` |
| `ACEAssetsLibrary` | Asset DB with ACE metadata | `AssetsLibraryModule` |

### Implementation Challenges

1. **Protobuf dependency**: ACE uses protobuf schemas. The plugin would need `protobuf` library or `libprotobuf` to parse `.kspkg` content. Tools like `protodump` can recover schemas from the game executable.

2. **No public file format spec**: Unlike AC1's well-documented KN5 format, ACE's formats are newer and reverse-engineered only by community tools.

3. **Evolving SDK**: ACE is in Early Access (currently v0.8). The SDK and file formats are likely to change.

4. **Limited modding scope**: Current SDK only supports custom cars for offline. Full content modding (tracks, UI) is not yet available.

5. **Separate installation path**: ACE installs to its own Steam directory, not alongside AC1. Path detection must be distinct.

6. **No CSP**: ACE has native modern rendering. CSP-specific config parsing is not needed.

---

## 4. Build and Deployment

### Build System Integration

The plugin would follow the same pattern as `ksAssettoCorsa`:

```
cmake/
  ├── target: ksAssettoCorsaEVO (SHARED)
  ├── output: bin/plugins/ksAssettoCorsaEVO.dll
  └── dependency: kseditor_lib (static)
```

### Directory Structure (Proposed)

```
src/plugins/simulators/kunos/assettocorsaevo/
├── ksAssettoCorsaEVO.cpp       # C API entry point
├── ksAssettoCorsaEVO_export.h  # DLL export macros
├── ACEPackageParser.h/.cpp     # .kspkg parsing
├── ACEPaths.h/.cpp             # ACE installation detection
├── ACERunner.h/.cpp            # Game launcher
├── ACEQmlBridge.h/.cpp         # QML bridge classes
├── ACEWorkshop.h/.cpp          # Steam Workshop
├── ACEAssetsLibrary.h/.cpp     # Asset DB
├── ACESharedMemory.h/.cpp      # Telemetry
├── ACELivePreview.h/.cpp       # Audio preview
└── ACESetupComparison.h/.cpp   # Setup tools
```

### Dependencies

| Dependency | Required? | Source |
|-----------|-----------|--------|
| `kseditor_lib` | Yes | Built in-tree |
| `Qt6::Core` | Yes | Qt 6.11.1 |
| `Qt6::Gui` | Yes | Qt 6.11.1 |
| `Qt6::Widgets` | Yes | Qt 6.11.1 |
| `Qt6::Network` | Yes | Qt 6.11.1 |
| `Qt6::Qml` | Yes | Qt 6.11.1 |
| `Qt6::Quick` | Yes | Qt 6.11.1 |
| `Qt6::Sql` | Yes | Qt 6.11.1 |
| `Qt6::OpenGL` | Yes | Qt 6.11.1 |
| `Qt6::Multimedia` | Yes | Qt 6.11.1 |
| `protobuf` | Likely | vcpkg or system |
| `mikktspace` | Optional | In-tree |
| `Vulkan` | Optional | Vulkan SDK |

---

## 5. Comparison: ksAssettoCorsa vs ksAssettoCorsaEVO

| Aspect | ksAssettoCorsa (AC1) | ksAssettoCorsaEVO (ACE) |
|--------|---------------------|------------------------|
| **Status** | ✅ Exists, functional | ❌ Does not exist |
| **Target game** | Assetto Corsa (2014) | Assetto Corsa EVO (2025) |
| **Source files** | ~40 files (20k+ lines) | 0 files |
| **CMake target** | `ksAssettoCorsa` | Not defined |
| **Model format** | `.kn5` (proprietary binary) | `.kspkg` (protobuf archives) |
| **Telemetry** | Shared memory (Windows) | TBD |
| **CSP support** | Extensive (parser, editor) | Not applicable |
| **SDK maturity** | Mature (10+ years) | Early Access (v0.7 SDK) |
| **Modding scope** | Cars, tracks, skins, apps | Cars only (offline) |
| **Community tools** | Mature ecosystem | Nascent (3 tools) |

---

## 6. Conclusion

`ksAssettoCorsaEVO.dll` is a **future product that has not been implemented**. The existing `ksAssettoCorsa.dll` provides a proven architecture for simulator plugins in ksEditor. Creating an ACE EVO plugin would require:

1. Implementing protobuf-based `.kspkg` parsing
2. Adapting content discovery for ACE's file layout
3. Building ACE-specific telemetry reading (if supported)
4. Following the established plugin pattern (C API + QML bridges)
5. Keeping pace with ACE Early Access SDK changes

The ACE SDK was released in June 2026 (v0.7), so the timing is right for development — but the formats and SDK are still evolving.
