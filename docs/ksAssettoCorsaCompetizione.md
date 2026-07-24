# ksAssettoCorsaCompetizione.dll — Product Overview

## Current Status: DOES NOT EXIST

`ksAssettoCorsaCompetizione.dll` does **not exist** in this codebase. No source files, no build target, no references — the name appears zero times across the entire repository. This document describes what such a plugin **would be** based on the existing `ksAssettoCorsa.dll` architecture, Assetto Corsa Competizione's underlying Unreal Engine 4 technology, and the vision of making ksEditor a complete UE4 replacement for ACC content editing.

---

## 1. Background: The Existing ksAssettoCorsa Plugin

The codebase contains `ksAssettoCorsa.dll` — a simulator integration plugin for the **original Assetto Corsa (AC1)**. It is currently the only simulator plugin in ksEditor.

### Architecture

```
kseditor.exe
  └── PluginManager (loads .dll from bin/plugins/)
        └── ksAssettoCorsa.dll  (SHARED library, ~40+ source files)
              ├── Extern "C" API: 8 entry-point functions
              └── 18+ sub-modules (ContentBrowser, KsContentPaths, KsAssettoCorsaRunner,
                    QmlBridges, SetupComparison, WorkshopModule,
                    AssetsLibraryModule, ACSharedMemory, ACLivePreviewBridge,
                    KN5Parser, KN5Decrypt, ACDParser, CspConfigParser,
                    CspConfigHandler, FBXExporter, etc.)
```

### Plugin Entry Point (ksAssettoCorsa.cpp)
- `getPluginId()` → `"ksAssettoCorsa"`
- `getPluginName()` → `"Assetto Corsa Plugin"`
- `getPluginVersion()` → `"0.9.0"`
- `getPluginDescription()` → `"Assetto Corsa content management and editing plugin for ksEditor"`

---

## 2. Assetto Corsa Competizione — The Game

**Assetto Corsa Competizione (ACC)** is Kunos Simulazioni's GT racing simulator, released in 2019. Unlike AC1, it uses **Unreal Engine 4** and is exclusively focused on GT World Challenge Europe (GT3, GT4, GT2 classes).

### Key Facts

| Attribute | Value |
|-----------|-------|
| **Release** | May 2019 (PC), 2020/2022 (consoles) |
| **Engine** | Unreal Engine 4.27 (custom fork) |
| **Steam App ID** | 805550 |
| **File format** | UE4 cooked packages (`.uasset` + `.uexp` + `.ubulk`) |
| **Modding policy** | No official content creation SDK |
| **Plugin/App support** | Python apps (shared memory), Broadcasting API |
| **Telemetry** | Shared memory (memory-mapped files) — similar to AC1 |
| **Broadcasting** | UDP-based API for race control, overlays, HUDs |
| **Audio** | FMOD middleware |
| **Rendering** | DirectX 11 (deferred renderer) |
| **Physics** | Kunos proprietary (not UE4 Chaos/PhysX for vehicle dynamics) |

### Official SDK Components

ACC ships with the following in its `sdk/` folder:

1. **Broadcasting SDK** (C#)
   - UDP protocol for real-time race data
   - Car positions, lap times, sector times, penalties
   - Event-based (session start/end, yellow flags, etc.)
   - Used by broadcast overlays, race control tools

2. **Shared Memory** (C++ structs)
   - Memory-mapped file: `Local\acpmf_static`, `Local\acpmf_physics`, `Local\acpmf_graphics`
   - Same concept as AC1 but with ACC-specific structures
   - Static info: car/track names, physics version
   - Graphics: camera position, weather, time of day
   - Physics: speed, RPM, throttle, brake, gear, tyre temps/pressure/wear, ABS, TC

### Kunos's Statement on Modding

> *"Assetto Corsa's structure was designed from the beginning to be moddable. Being developed with UE4, AC Competizione processes data and assets through a completely different structure and file format. Therefore, the game will not be compatible with mod contents created for Assetto Corsa."* — Kunos Simulazioni

This means ksEditor must work with UE4's cooked asset format directly — there is no official SDK for content creation.

---

## 3. Vision: ksEditor as UE4 Replacement for ACC

### The Goal

Make ksEditor capable of covering **all Unreal Engine 4 features** required to create, edit, and manage ACC content — eliminating the need to install Unreal Engine or the UE4 Editor.

### How UE4 Works — the Systems to Replicate

```
┌──────────────────────────────────────────────────────────┐
│                    UNREAL ENGINE 4                        │
├──────────────────────────────────────────────────────────┤
│  Content Browser   │  Material Editor   │  Blueprint     │
│  (Asset Registry)  │  (Material Graph)  │  (Visual SC)   │
├────────────────────┴────────────────────┴────────────────┤
│  Static Mesh Editor  │  Skeletal Mesh Editor             │
│  (LOD, Collision)    │  (Skinning, AnimSet)              │
├────────────────────┬────────────────────┬────────────────┤
│  World / Level     │  Landscape Editor  │  Cascade FX    │
│  Editor            │  (Terrain)         │  (Particles)   │
├────────────────────┴────────────────────┴────────────────┤
│  Cooking / Packaging  │  Asset Manager   │  Physics      │
│  (Pak, uasset, uexp)  │  (Redirector)    │  (Chaos)      │
└──────────────────────────────────────────────────────────┘
```

### The Asset Pipeline

```
Blender / 3ds Max / Maya ──FBX──→ ksEditor ──uasset──→ ACC
Photoshop / Substance ──PNG/TGA──→ ksEditor ──uexp────→ ACC
Audacity / Reaper ───WAV/OGG───→ ksEditor ──ubulk───→ ACC
                                    │
                            ┌───────┴───────┐
                            │   UE4 Asset   │
                            │   Pipeline    │
                            │   (cooked)    │
                            └───────────────┘
```

---

## 4. UE4 Systems to Replicate in ksEditor

### 4.1 UE4 Package System (uasset/uexp/ubulk)

UE4 saves assets in `.uasset` + `.uexp` (+ `.ubulk` for bulk data). This is the **core format** that ksEditor must read and write.

```
Cooked UE4 asset structure:

.uasset (header)
├── FPackageFileSummary (magic: 0x9E2A83C1)
│   ├── FileVersionUE4 / FileVersionUE5
│   ├── EngineVersion (major, minor, patch, changelist, branch)
│   ├── NameTable (FNameEntry[])
│   ├── ImportTable (FObjectImport[]) — dependencies on other packages
│   ├── ExportTable (FObjectExport[]) — objects in this package
│   └── DependsMap
│
.uexp (export data)
├── Serialized properties for each export
│   ├── UStaticMesh: LODs, sections, collision, UVs, material slots
│   ├── UMaterial: shader graph, expressions, parameters
│   ├── UTexture: pixel data (or reference to .ubulk)
│   ├── USkeletalMesh: bones, skin weights, morph targets
│   └── UAnimSequence: keyframes, bone tracks, compression
│
.ubulk (optional bulk data)
└── Raw data (texture mips, vertex buffers, index buffers)
```

**Existing libraries for uasset parsing (reference for C++ port):**

| Library | Language | Description |
|---------|----------|-------------|
| [UAssetAPI](https://github.com/atenfyr/UAssetAPI) | C#/.NET | Read/write uasset from UE4.13 to 5.7. 100+ property types, 12 export types |
| [UAssetReader](https://github.com/codeaid/UAssetReader) | C#/.NET | .NET 5 UE4 binary asset reader |
| [uasset_read](https://github.com/soatori/uasset_read) | Rust | Minimal reader |
| [FModel](https://github.com/4sval/FModel) | C# | UE4/5 asset explorer |
| [UE4SS](https://github.com/UE4SS-RE/UE4SS) | C++ | UE4 debugger with serialization support |

#### Asset Type Map — UE4 → ksEditor

| UE4 Export Class | Contents | ksEditor Target |
|-----------------|----------|-----------------|
| `UStaticMesh` | LODs, sections, collision, UVs, material slots | Mesh editor (`ksmodeler`) |
| `USkeletalMesh` | Bones, skin weights, morph targets | Skeletal mesh + skinning |
| `UMaterial` | Expressions, inputs, shader graph | Material node graph editor |
| `UMaterialInstanceConstant` | Inherited parameter overrides | Material instance editor |
| `UTexture2D` | Compressed pixel data (BCn) | Texture viewer/editor |
| `UAnimSequence` | Compressed keyframes | Animation editor |
| `UPhysicsAsset` | Bodies and constraints | PhAT-like editor |
| `UBlueprintGeneratedClass` | Kismet bytecode | Blueprint decompiler (read-only) |
| `UWorld` / `ULevel` (`.umap`) | Actors + transforms + references | Level editor 3D |
| `USoundBase` / `USoundWave` | Audio data (FMOD in ACC) | Audio viewer (`ksAudioStudio`) |

### 4.2 Content Browser / Asset Registry

UE4 tracks all assets via `AssetRegistry.bin`. ksEditor needs equivalent functionality.

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| `AssetRegistry.bin` | SQLite database with all ACC asset metadata | High |
| Content Browser (tree) | `AssetsLibraryModule` (existing) + ACC extension | High |
| Filter by type | Category filters in Content Browser | High |
| Search by name/path/tag | Full-text search in DB | High |
| Thumbnails | Thumbnailing system in `AssetsLibraryModule` | Medium |
| Asset dependencies | Dependency table in DB | Medium |
| Redirectors (moved assets) | Path tracking in DB | Low |

### 4.3 Material Editor

ACC uses cooked UE4 materials with **custom Kunos shaders**. ksEditor must replicate the **Material Editor** with a node graph.

#### Material Expressions (~200 node types to support)

Key categories:

| Category | Examples | Count |
|----------|----------|-------|
| **Texture** | TextureSample, TextureCoordinate, TextureObjectParameter | ~10 |
| **Math** | Add, Multiply, Lerp, Clamp, Saturate, Power, Abs, Ceil, Floor, Frac, Sine, Cosine | ~30 |
| **Vector** | MakeFloat2/3/4, BreakFloat2/3/4, AppendVector, ComponentMask | ~15 |
| **Constant** | Constant, Constant2Vector, Constant3Vector, Constant4Vector, ScalarParameter, VectorParameter | ~15 |
| **PBR** | NormalFromHeightmap, Fresnel, ReflectionVector, PixelNormalWS | ~20 |
| **Utility** | Time, VertexNormalWS, CameraVector, Transform, Distance | ~30 |
| **Custom** | Custom HLSL node | ~5 |
| **Kunos-specific** | CarPaint flakes, Carbon fiber weave, Rain droplets, Brake glow | ~10+ |

#### Kunos-specific shaders for ACC:

- **Car paint**: Metallic flakes, clear coat, orange peel
- **Carbon fiber**: Woven pattern, UV direction mapping
- **Tyres**: Rubber anisotropy, wear gradient
- **Brake discs**: Heat glow gradient, groove pattern
- **Headlights/taillights**: Emissive with lens flare, DRL animation
- **Rain**: Water droplets, rain streaks on windshield
- **Window glass**: Reflection, opacity gradient, dirt

#### Material Instance System

```
UMaterial (Master)
├── BaseColor:            Param("PaintColor", float3)
├── Roughness:            Param("Roughness", float)
├── Metallic:             Param("Metallic", float)
├── Normal:               TextureSample(T_CarbonFiber_N)
│
└── UMaterialInstanceConstant (child)
    ├── Override: PaintColor = (0.85, 0.12, 0.05)  ← Ferrari Red
    ├── Override: Roughness = 0.15
    └── Unset: Metallic → inherits from master
```

#### Implementation:

```
┌──────────────────────────────────────────────────┐
│           ksEditor — Material Editor              │
├──────────────────────────────────────────────────┤
│  ┌──────────────────────────────────────────┐    │
│  │  Canvas: Material Node Graph              │    │
│  │  (QML Canvas + custom NodeEditor)         │    │
│  │                                            │    │
│  │  [BaseColor] ○──○ [Multiply] ○──○ [Tex]  │    │
│  │  [Roughness] ○──○ [Lerp]     ○──○ [Const] │    │
│  │  [Normal]    ○──○ [NormalFromHeightmap]   │    │
│  └──────────────────────────────────────────┘    │
├──────────────────────────────────────────────────┤
│  3D Preview (Vulkan) │ Properties │ Parameters   │
└──────────────────────────────────────────────────┘
```

ksEditor already has a custom node graph system in `src/core/customNodeGraph/`. This must be extended to support:

- UE4 material expression types (~200 node types)
- Pin types (float, float2, float3, float4, TextureObject, etc.)
- Compilation: node graph → serialized uasset properties
- 3D preview on sphere/cube using Vulkan

### 4.4 Static Mesh Editor

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Static Mesh Editor** | Existing 3D mesh editor (`ksmodeler`) | High |
| **LOD Management** | LOD tools in `ksmodeler` + `LODExporter` | High |
| **Collision** (primitive, convex, complex) | Collision editor | High |
| **UV Editor** | UV editor in `ksmodeler` | High |
| **Vertex Color Editor** | Vertex painting (PaintSettings) | Medium |
| **Lightmap UV Generation** | UV2 generation | Medium |
| **Mesh Sectioning** (material slots) | Material assignment per section | High |

### 4.5 Skeletal Mesh Editor

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Skeletal Mesh Editor** | Bone/skin tools in `ksmodeler` | High |
| **Skinning** (weight painting) | Weight painting in `CharacterEditor` | High |
| **Physics Asset** (PhAT) | Physics bodies per bone | Medium |
| **Morph Targets** | Blend shape editing | Medium |
| **Socket Editor** | Attachment points for objects | Low |

### 4.6 Texture System

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Texture Import** (PNG, TGA, DDS, EXR, TIFF) | Already supported in core Graphics | High |
| **Texture Cooking** (BC1-BC7, ASTC, ETC2) | GPU-backed compression via DirectXTex | High |
| **Texture Manager** | Streaming, mip generation | Medium |
| **Virtual Texture** | Not needed for ACC | N/A |

**Texture compression formats required for ACC:**

| Format | UE4 Name | Use in ACC |
|--------|----------|------------|
| BC1 (DXT1) | `TF_DXT1` | RGB diffuse (opaque) |
| BC3 (DXT5) | `TF_DXT5` | RGBA diffuse (alpha) |
| BC4 | `TF_BC4` | Single-channel (roughness, AO) |
| BC5 | `TF_BC5` | Normal maps |
| BC7 | `TF_BC7` | High-quality RGB(A) |
| G8 | `TF_G8` | Grayscale (height, mask) |

### 4.7 Particle System

ACC uses particles for: rain, water spray, tyre smoke, gravel/debris, brake sparks.

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Cascade** (UE4 legacy) | Custom particle system (emitter → spawn → update → render) | Medium |
| **Niagara** (UE5, not in ACC) | Not needed | N/A |
| **Particle LOD** | Distance-based LOD | Low |

### 4.8 Level / World Editor

ACC levels (`.umap`) contain positioned actors. `.umap` files are almost identical to `.uasset` but with `UWorld`/`ULevel` exports containing actor arrays.

**Level structure (cooked .umap):**

```
FPackageFileSummary
├── Magic: 0x9E2A83C1
├── NameTable
├── ImportTable (dependencies on meshes, materials, etc.)
└── ExportTable:
    └── [0]: UWorld
        └── [1]: ULevel (inside UWorld)
            ├── Actors: TArray<AActor*>
            │   ├── [0]: AStaticMeshActor
            │   │   ├── StaticMeshComponent
            │   │   │   ├── StaticMesh: FObjectImport
            │   │   │   ├── RelativeLocation: FVector
            │   │   │   ├── RelativeRotation: FRotator
            │   │   │   └── RelativeScale3D: FVector
            │   │   └── ...
            │   ├── [1]: ASkeletalMeshActor
            │   ├── [2]: ALight (point, directional, spot)
            │   └── [3]: APlayerStart
            ├── Model (BSP, if present)
            └── WorldSettings
```

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Level Editor** | 3D viewport with actor placement | High |
| **Actor Placement** | Drag & drop actors into scene | High |
| **Transform Tools** (W/E/R) | Already present in `ksmodeler` | High |
| **World Outliner** | Hierarchical actor tree | High |
| **Detail Panel** | Property grid for selected actor | High |
| **Landscape Editor** | Terrain sculpting | Medium |
| **Foliage Editor** | Paint vegetation instances | Low |
| **Lightmap Resolution** | Light baking configuration | Medium |

### 4.9 Blueprint System

ACC uses Blueprints (visual scripting) for: car logic, AI, UI, event handling.

**Challenge:** The Blueprint graph is extremely complex. Full replication is **prohibitive**, but partial support is feasible.

| Approach | Description | Effort |
|----------|-------------|--------|
| **Read-only Blueprint viewer** | Decompiled bytecode tree | Low |
| **Blueprint decompiler** | Kismet bytecode → pseudo-code | Medium |
| **Blueprint editor** | Visual node editor | Very High |
| **Blueprint compiler** | Pseudo-code → Kismet bytecode | Extremely High |

**Strategy:** Start with a read-only bytecode decompiler. UAssetAPI already supports reading Kismet bytecode.

Kismet bytecode format:
```
FKismetCompiledStatement:
  - uint8 OpCode (EX_CallFunction, EX_Let, EX_Return, ...)
  - uint8[] Operands (variable length per OpCode)
  - UObject* ExpressionPointer (original node reference)
```

### 4.10 Animation System

| UE4 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **AnimSequence** | Keyframe editor for bones | High |
| **AnimBlueprint** | State machine for animation blending | Medium |
| **BlendSpace** | 1D/2D animation interpolation | Medium |
| **Montage** | Non-blocking animation sections | Low |
| **Retargeting** | Animation adaptation between skeletons | Low |

### 4.11 Physics Asset (PhAT)

| Feature | Description | Priority |
|---------|-------------|----------|
| **Body Creation** | Physics bodies (primitive, convex) per bone | High |
| **Collision Shapes** | Box, sphere, capsule, convex hull | High |
| **Constraint Setup** | Joint angle limits, stiffness, damping | High |
| **Skeletal Physics** | Ragdoll simulation preview | Medium |

### 4.12 Audio (FMOD)

ACC uses **FMOD** for audio, not UE4's native audio engine.

| System | Description | Priority |
|--------|-------------|----------|
| **FMOD Bank Reader** | Read/extract ACC FMOD banks | High |
| **Waveform Viewer** | Audio waveform visualization | Already in `ksAudioStudio` |
| **FMOD Project Importer** | Import FMOD projects | Medium |
| **Spatial Audio** | 3D positional audio | Medium |

### 4.13 Cooking / Packaging Pipeline

When ACC "cooks" assets, UE4:
1. Resolves dependencies
2. Compiles shaders for target platform (DX11)
3. Compresses textures (BCn)
4. Serializes in cooked format (no editor-only data)
5. Packages into `.pak` files

| Feature | Description | Priority |
|---------|-------------|----------|
| **Asset Dependency Resolution** | Find all dependencies of an asset | High |
| **Texture Compression** | BC1-BC7 via DirectXTex (GPU) | High |
| **Shader Compilation** | HLSL → DX11 bytecode via `d3dcompiler_47.dll` | High |
| **Package Serialization** | Write uasset + uexp in cooked format | High |
| **Pak Packaging** | Create `.pak` archives (zlib/AES) | Medium |
| **Asset Registry Builder** | Generate `AssetRegistry.bin` | Medium |

---

## 5. ksAssettoCorsaCompetizione.dll — Proposed Architecture

The plugin is the vehicle that delivers all the UE4-replacement systems into ksEditor.

### CMake Target

```cmake
add_library(ksAssettoCorsaCompetizione SHARED
    ksAssettoCorsaCompetizione.cpp
    # UE4 Core
    ue4/FPackageFileSummary.h
    ue4/FNameTable.h
    ue4/FObjectImportExport.h
    ue4/UAssetSerializer.h/.cpp
    ue4/UAssetDeserializer.h/.cpp
    ue4/UPropertyReader.h/.cpp
    ue4/UPropertyWriter.h/.cpp
    # UE4 Asset Types
    ue4/types/UStaticMesh.h/.cpp
    ue4/types/USkeletalMesh.h/.cpp
    ue4/types/UMaterial.h/.cpp
    ue4/types/UMaterialInstance.h/.cpp
    ue4/types/UTexture2D.h/.cpp
    ue4/types/UAnimSequence.h/.cpp
    ue4/types/UPhysicsAsset.h/.cpp
    ue4/types/UWorld.h/.cpp
    ue4/types/UBlueprint.h/.cpp
    ue4/types/USoundWave.h/.cpp
    # Factories
    ue4/factories/MeshFactory.h/.cpp
    ue4/factories/SkeletalMeshFactory.h/.cpp
    ue4/factories/TextureFactory.h/.cpp
    ue4/factories/MaterialFactory.h/.cpp
    ue4/factories/AnimationFactory.h/.cpp
    # Cooking
    ue4/cooking/TextureCompressor.h/.cpp
    ue4/cooking/ShaderCompiler.h/.cpp
    ue4/cooking/AssetCooker.h/.cpp
    ue4/cooking/PakWriter.h/.cpp
    ue4/cooking/AssetRegistryBuilder.h/.cpp
    # Editors
    editors/MaterialEditor/*.h/.cpp
    editors/BlueprintViewer/*.h/.cpp
    editors/LevelEditor/*.h/.cpp
    # ACC Integration
    ACCSharedMemory.h/.cpp
    ACCBroadcastingClient.h/.cpp
    ACCPaths.h/.cpp
    ACCWeatherImport.h/.cpp
    ACCSetupManager.h/.cpp
    ACCServerConfig.h/.cpp
    ACCQmlBridge.h/.cpp
    ACCAssetsLibrary.h/.cpp
    ACCLivePreviewBridge.h/.cpp
)
target_link_libraries(ksAssettoCorsaCompetizione PRIVATE
    kseditor_lib
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick
    Qt6::OpenGL Qt6::Multimedia Qt6::Sql
    ws2_32              # Winsock for Broadcasting UDP
    d3dcompiler         # HLSL → DX11 bytecode
    DirectXTex          # BCn texture compression
    FBX_SDK             # FBX import/export
    zlib                # Pak compression
)
set_target_properties(ksAssettoCorsaCompetizione PROPERTIES
    OUTPUT_NAME "ksAssettoCorsaCompetizione"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin/plugins"
)
add_dependencies(kseditor ksAssettoCorsaCompetizione)
```

### Plugin Entry-Point

```cpp
extern "C" {
    KS_ASSETTOCORSACOMPETIZIONE_API const char* getPluginId() {
        return "ksAssettoCorsaCompetizione";
    }
    KS_ASSETTOCORSACOMPETIZIONE_API const char* getPluginName() {
        return "Assetto Corsa Competizione Plugin";
    }
    KS_ASSETTOCORSACOMPETIZIONE_API const char* getPluginVersion() {
        return "1.16.0";
    }
    KS_ASSETTOCORSACOMPETIZIONE_API const char* getPluginDescription() {
        return "Complete UE4 content pipeline for Assetto Corsa Competizione — "
               "edit meshes, materials, textures, levels, and blueprints "
               "without Unreal Engine";
    }
    KS_ASSETTOCORSACOMPETIZIONE_API bool initializePlugin() { ... }
    KS_ASSETTOCORSACOMPETIZIONE_API void shutdownPlugin() { ... }
    KS_ASSETTOCORSACOMPETIZIONE_API bool isPluginAvailable() { ... }
    KS_ASSETTOCORSACOMPETIZIONE_API const char* getInstallPath() { ... }
    KS_ASSETTOCORSACOMPETIZIONE_API void setInstallPath(const char* path) { ... }
}
```

### File Structure

```
src/plugins/simulators/kunos/competizione/
├── ksAssettoCorsaCompetizione.cpp/.h       # Plugin C API
├── ksAssettoCorsaCompetizione_export.h     # DLL macros
│
├── ue4/                                    # Core UE4 reader/writer
│   ├── FPackageFileSummary.h               # uasset header parser
│   ├── FNameTable.h                        # Name table reader
│   ├── FObjectImportExport.h               # Import/Export table
│   ├── UAssetSerializer.h/.cpp             # Generic uasset+uexp serializer
│   ├── UAssetDeserializer.h/.cpp           # Generic deserializer
│   ├── UPropertyReader.h/.cpp              # Property deserialization
│   ├── UPropertyWriter.h/.cpp              # Property serialization
│   └── UE4Version.h                        # Supported UE4 versions
│
├── ue4/types/                              # Asset type serialization
│   ├── UStaticMesh.h/.cpp
│   ├── USkeletalMesh.h/.cpp
│   ├── UMaterial.h/.cpp
│   ├── UMaterialInstance.h/.cpp
│   ├── UTexture2D.h/.cpp
│   ├── UAnimSequence.h/.cpp
│   ├── UPhysicsAsset.h/.cpp
│   ├── UWorld.h/.cpp
│   ├── UBlueprint.h/.cpp
│   └── USoundWave.h/.cpp
│
├── ue4/factories/                          # Import/Export factories
│   ├── MeshFactory.h/.cpp                  # FBX ↔ UStaticMesh
│   ├── SkeletalMeshFactory.h/.cpp          # FBX ↔ USkeletalMesh
│   ├── TextureFactory.h/.cpp               # PNG/DDS ↔ UTexture2D
│   ├── MaterialFactory.h/.cpp              # Graph ↔ UMaterial
│   └── AnimationFactory.h/.cpp             # FBX anim ↔ UAnimSequence
│
├── ue4/cooking/                            # Cooking pipeline
│   ├── TextureCompressor.h/.cpp            # BC1-BC7 via DirectXTex
│   ├── ShaderCompiler.h/.cpp               # HLSL → DX11 bytecode
│   ├── AssetCooker.h/.cpp                  # Cooking orchestrator
│   ├── PakWriter.h/.cpp                    # .pak archive creation
│   └── AssetRegistryBuilder.h/.cpp         # Generate AssetRegistry.bin
│
├── editors/                                # Visual editors
│   ├── MaterialEditor/
│   │   ├── MaterialGraphNode.h
│   │   ├── MaterialGraphPin.h
│   │   ├── MaterialExpressionTypes.h       # ~200 expression types
│   │   └── MaterialPreview3D.h             # Vulkan material preview
│   ├── BlueprintViewer/
│   │   ├── KismetBytecodeReader.h
│   │   └── KismetDecompiler.h
│   └── LevelEditor/
│       ├── ActorPlacementTool.h
│       ├── WorldOutliner.h
│       └── DetailPanel.h
│
├── ACCSharedMemory.h/.cpp                  # Live telemetry
├── ACCBroadcastingClient.h/.cpp            # UDP broadcasting protocol
├── ACCPaths.h/.cpp                         # ACC installation detection
├── ACCWeatherImport.h/.cpp                 # Weather config
├── ACCSetupManager.h/.cpp                  # Setup files
├── ACCServerConfig.h/.cpp                  # Dedicated server
├── ACCQmlBridge.h/.cpp                     # QML bridges
├── ACCLivePreviewBridge.h/.cpp             # Audio from telemetry
└── ACCAssetsLibrary.h/.cpp                 # Asset DB
```

### Dependencies

| Dependency | Purpose | Source |
|------------|---------|--------|
| `kseditor_lib` | Core editor | Built in-tree |
| `Qt6::Core`, `Gui`, `Widgets`, `Network`, `Qml`, `Quick`, `OpenGL`, `Multimedia`, `Sql` | Qt framework | Qt 6.11.1 |
| `FBX SDK` (2020.2) | FBX mesh/animation import/export | Autodesk SDK |
| `DirectXTex` | BC1-BC7 texture compression | GitHub (Microsoft) |
| `d3dcompiler` | HLSL → DX11 bytecode | Windows SDK |
| `zlib` | Pak compression | vcpkg (already present) |
| `ws2_32` | Winsock for Broadcasting UDP | Windows SDK |
| `libpng`, `libjpeg-turbo` | Texture decode | vcpkg (already present) |
| `minizip` | Pak archive handling | vcpkg |
| `xxhash` | UE4 FName hashing | vcpkg |
| `OpenSSL` (AES) | Pak encryption (if needed) | vcpkg |
| `FMOD Studio API` | Read ACC audio banks | FMOD SDK |

### Existing ACC Integration in ksEditor

The codebase already has some ACC awareness:

| Location | Feature |
|----------|---------|
| `src/core/weather/WeatherEditor.cpp:789` | `exportToACC()` — weather config in ACC INI format |
| `src/core/weather/WeatherEditorModule.h:235` | `exportToACC()` declaration |
| `docs/modules.md:141` | Weather module mentions "CSP/ACC format" |
| `src/core/Config/ConfigSchema.cpp:831` | Schema includes "ACC dedicated server settings" |
| `src/core/modmanager/ModManager.h:629` | `scanACContentMods()` — scans ACC content |

---

## 6. Implementation Roadmap

### Phase 1: Asset Reading (3 months)

```
Goal: ksEditor can read and display all ACC assets

[ ] FPackageFileSummary parser (uasset header)
[ ] NameTable reader/writer
[ ] Import/Export table reader/writer
[ ] UProperty deserializer (core types: int, float, bool, FString, FName, FVector, etc.)
[ ] UStaticMesh deserializer (mesh data → internal ksEditor format)
[ ] UTexture2D deserializer (BCn → PNG preview)
[ ] UMaterial deserializer (expressions → node list)
[ ] 3D viewport mesh display
[ ] Texture viewer (with BCn decompression)
[ ] Material network tree viewer (read-only)
[ ] Blueprint bytecode reader + basic decompilation
```

### Phase 2: Asset Writing (4 months)

```
Goal: ksEditor can modify and rewrite ACC assets

[ ] UProperty serializer (all core types)
[ ] UStaticMesh serializer (mesh edits → valid .uasset)
[ ] UTexture2D serializer (re-encode BCn → .uasset)
[ ] Material editor: base nodes (TextureSample, Constant, Multiply, Lerp, etc.)
[ ] Material editor: pin connections
[ ] Material editor: 3D preview on sphere
[ ] UMaterialInstance serializer (parameter overrides)
[ ] UMaterial serializer (full graph → valid .uasset)
[ ] Dependency management (import table updates)
[ ] Validation: edited asset loads in ACC
```

### Phase 3: Full Pipeline (3 months)

```
Goal: ksEditor has complete import → edit → export pipeline

[ ] FBX → UStaticMesh factory (import from Blender/Maya)
[ ] UStaticMesh → FBX factory (export for DCC tools)
[ ] FBX → USkeletalMesh (with skeleton and skinning)
[ ] PNG/DDS → UTexture2D (with BCn compression)
[ ] UTexture2D → PNG (decompression)
[ ] Texture compressor: BC1, BC3, BC5, BC7 via DirectXTex
[ ] Cooker: resolve dependencies → compile → serialize
[ ] Pak writer: .pak archive with zlib
[ ] AssetRegistry.bin builder
[ ] Material editor: ~50 most common expression types
[ ] Material instance editor (parameter overrides)
```

### Phase 4: Advanced Editors (4 months)

```
Goal: ksEditor can fully replace UE4 Editor for ACC

[ ] Level Editor: open/save .umap
[ ] Level Editor: place, move, rotate, scale actors
[ ] Level Editor: World Outliner with hierarchy
[ ] Level Editor: Detail Panel for actor properties
[ ] Level Editor: copy/paste actors, grouping
[ ] PhAT-like Physics Asset editor
[ ] Weight painting for skeletal meshes
[ ] LOD management (creation, preview, export)
[ ] UV editor + UV2 generation (lightmap)
[ ] Animation editor (keyframes, curves)
[ ] Blueprint decompiler (readable pseudo-code)
[ ] Collision editor (primitive, convex, complex)
[ ] Audio: read ACC FMOD banks
[ ] Audio: preview sounds in materials
```

### Phase 5: ACC Integration (2 months)

```
Goal: Complete ksAssettoCorsaCompetizione plugin

[ ] ACC installation detection
[ ] Content Browser: browse cars, tracks, UI assets
[ ] Content Browser: thumbnail previews
[ ] Live telemetry from ACC shared memory
[ ] Broadcasting API client
[ ] Weather import/export
[ ] Setup comparison
[ ] ACC Live Preview: audio driven by telemetry
[ ] Pak extract (from .pak to filesystem)
[ ] Pak repack (with modifications)
[ ] Deploy: write modified assets to ACC/<car>/<track>/
```

### Total Estimate: ~16 months (team of 3 developers)

---

## 7. Gap Analysis vs. Full UE4 Editor

| UE4 Feature | Effort | Needed for ACC? | Decision |
|-------------|--------|-----------------|----------|
| Blueprint full editor | Very High | Low | Read-only decompiler only |
| Blueprint compiler | Extremely High | Very Low | Do not implement |
| Niagara particles | High | Medium | Custom particle editor |
| Sequencer / Matinee | High | Medium | Basic timeline editor |
| Landscape full editor | High | Medium | Basic sculpting |
| Foliage editor | Medium | Low | Do not implement |
| Media Player / Media Texture | Medium | Low | Do not implement |
| Geometry Cache | Medium | Low | Do not implement |
| Python scripting | Medium | Medium | Script editor already exists |
| Virtual Texture | High | None | Do not implement |
| Chaos Physics | High | None (Kunos physics) | Do not implement |
| Lumen / Nanite (UE5) | N/A | Not in ACC | Not needed |

---

## 8. Risks and Mitigations

### Technical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Cooked format varies** between ACC versions | High | Version detection; test with every ACC update |
| **.pak AES encryption** unknown | High | ACC currently does not encrypt .pak; monitor |
| **Kunos proprietary shader bytecode** | High | Reverse-engineer with RenderDoc / DX11 API |
| **Blueprint bytecode complexity** | Medium | Start read-only; compiler is very hard |
| **Texture cooking performance** | Medium | DirectXTex with GPU acceleration |
| **UAssetAPI license (MIT)** | Low | C++ port is derived work; compatible |
| **FMOD bank encryption** | Medium | ACC uses standard FMOD; readable via FMOD API |
| **Custom UE4 properties** in ACC | High | Implement incrementally; test with real assets |

### Legal Risks

- **ACC EULA**: Verify if asset modification is permitted
- **Reverse engineering**: Parsing cooked format may be considered reverse engineering. Seek legal advice
- **Distribution**: ksEditor is MIT, but distributing extracted ACC assets is forbidden

---

## 9. Community Ecosystem

### Existing Libraries for Reference

| Project | Language | Purpose |
|---------|----------|---------|
| [UAssetAPI](https://github.com/atenfyr/UAssetAPI) | C# | Full uasset read/write (UE4.13–5.7) |
| [UAssetGUI](https://github.com/atenfyr/UAssetGUI) | C# | Visual uasset editor |
| [FModel](https://github.com/4sval/FModel) | C# | UE4/5 asset explorer |
| [UE4SS](https://github.com/UE4SS-RE/UE4SS) | C++ | UE4 debugger + serialization |
| [DirectXTex](https://github.com/Microsoft/DirectXTex) | C++ | BCn texture compression |
| [UModel (UE Viewer)](https://www.gildor.org/) | C++ | UE4/5 model viewer |
| [accbroadcastingsdk](https://github.com/toonknapen/accbroadcastingsdk) | Go | Broadcasting protocol |
| [ACCTelemetry](https://github.com/gotzl/acctelemetry) | C# | ACC telemetry display |
| [PyAccSharedMemory](https://github.com/rrennoir/PyAccSharedMemory) | Python | ACC shared memory reader |
| [accservermanager](https://github.com/gotzl/accservermanager) | Go | Dedicated server manager |

### Community Tools Using ACC Data

| Tool | Data Source | Purpose |
|------|-------------|---------|
| SimHub | Shared memory | Dashboards, haptic feedback, motion rigs |
| Crew Chief | Shared memory | Race engineer, spotter |
| RaceElements | Shared memory | Live telemetry analysis |
| Z1 Dashboard | Shared memory | Racing dashboard |
| ACC Server Manager | Broadcasting | Dedicated server management |

---

## 10. Comparison: ksAssettoCorsa vs ksAssettoCorsaCompetizione

| Aspect | ksAssettoCorsa (AC1) | ksAssettoCorsaCompetizione (ACC) |
|--------|---------------------|----------------------------------|
| **Status** | ✅ Exists, functional | ❌ Does not exist |
| **Target game** | Assetto Corsa (2014) | Assetto Corsa Competizione (2019) |
| **Engine** | Proprietary (in-house) | Unreal Engine 4.27 |
| **Source files** | ~40 files (20k+ lines) | ~70+ files (est. 50k+ lines) |
| **CMake target** | `ksAssettoCorsa` | Not defined |
| **Model format** | `.kn5` (proprietary binary) | `.uasset` + `.uexp` + `.ubulk` (UE4 cooked) |
| **Content modding** | Full (cars, tracks, skins, apps) | Via UE4 replacement pipeline |
| **Telemetry** | Shared memory | Shared memory + Broadcasting (UDP) |
| **CSP support** | Extensive | Not applicable (native UE4 shaders) |
| **Workshop** | Full integration | Liveries/setups only |
| **Modding policy** | Open | Closed (no official SDK) |
| **Primary use** | Content editing, model parsing | Full UE4 content pipeline + telemetry |

---

## 11. Conclusion

`ksAssettoCorsaCompetizione.dll` is a **future product that has not been implemented**. The vision is ambitious: make ksEditor a **complete replacement for Unreal Engine 4** for the purpose of editing Assetto Corsa Competizione content, eliminating the need to install UE4.

The key technical challenge is implementing read/write support for UE4's cooked asset format (`.uasset`/`.uexp`/`.ubulk`) — a proprietary binary format that varies between engine versions. Existing libraries like UAssetAPI provide a reference implementation in C# that would need to be ported to C++.

Once the format layer is in place, ksEditor's existing tools can be extended:
- **`ksmodeler`** for static/skeletal mesh editing
- **`ksAudioStudio`** for FMOD audio
- **`CustomNodeGraph`** for the material editor
- **`CharacterEditor`** for skinning/weight painting
- **`AssetsLibraryModule`** for the content browser
- **Vulkan renderer** for 3D previews

The existing `ACSharedMemory.h/.cpp` provides a template for ACC telemetry, and the Broadcasting API (UDP-based event system) is entirely new infrastructure with no equivalent in the current codebase.
