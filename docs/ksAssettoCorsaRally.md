# ksAssettoCorsaRally.dll — Product Overview

## Current Status: DOES NOT EXIST

ksAssettoCorsaRally.dll does **not exist** in this codebase. No source files, no build target, no references — the name appears zero times across the entire repository. This document describes what such a plugin **would be** based on the existing ksAssettoCorsa.dll architecture, Assetto Corsa Rally's Unreal Engine 5 technology, and the vision of making ksEditor a complete UE5 replacement for ACR content editing.

---

## 1. Background: The Existing ksAssettoCorsa Plugin

The codebase contains ksAssettoCorsa.dll — a simulator integration plugin for the **original Assetto Corsa (AC1)**. It is currently the only simulator plugin in ksEditor.

### Architecture

`
kseditor.exe
  └── PluginManager (loads .dll from bin/plugins/)
        └── ksAssettoCorsa.dll  (SHARED library, ~40+ source files)
              ├── Extern ""C"" API: 8 entry-point functions
              └── 18+ sub-modules (ContentBrowser, KsContentPaths, KsRunner,
                    QmlBridges, SetupComparison, WorkshopModule,
                    AssetsLibraryModule, ACSharedMemory, ACLivePreviewBridge,
                    KN5Parser, KN5Decrypt, ACDParser, CspConfigParser,
                    CspConfigHandler, FBXExporter, etc.)
`

### Existing Rally Awareness in ksEditor

The codebase already has rally-specific references:

| Location | Feature |
|----------|---------|
| src/modules/PhysicsEditor/tire_PacejkaTireModel.cpp:310 | getRallyTireCoefficients() — Pacejka coefficients for loose-surface tires |
| src/modules/PhysicsEditor/TireCurveEditor.cpp:78 | ""Rally"" tire preset in the curve editor UI |
| src/core/ui/SoundWizard.cpp:84 | ""Rally"" category in the sound wizard |

---

## 2. Assetto Corsa Rally — The Game

**Assetto Corsa Rally (ACR)** is a standalone rally simulation developed by Supernova Games Studios in technical partnership with Kunos Simulazioni, published by 505 Games. It launched in Early Access on Steam on November 13, 2025 (App ID: 3917090), and is currently at EA 0.5 (June 2026).

### Key Facts

| Attribute | Value |
|-----------|-------|
| **Release** | November 13, 2025 (Early Access) |
| **Developer** | Supernova Games Studios (w/ Kunos Simulazioni) |
| **Publisher** | 505 Games |
| **Engine** | Customized Unreal Engine 5.4.3 |
| **Steam App ID** | 3917090 |
| **Physics** | Adapted AC1 physics engine (refined for rally) |
| **Audio** | FMOD middleware |
| **Rendering** | DirectX 12 (UE5) |
| **Multiplayer** | Up to 16 drivers (EA 0.3+) |
| **Current EA version** | 0.5 (June 2026) |
| **Price** | .99 / €29.99 |

### File Formats

| Format | Location | Description |
|--------|----------|-------------|
| .pak / .ucas / .utoc | cr/Content/Paks/ | UE5 cooked asset packages |
| .uasset / .uexp / .ubulk | Inside .pak | UE5 serialized assets |
| .bank | cr/Content/FMod/Desktop/ | FMOD audio banks (engine, dialogue, UI) |
| usmap | External | UE5 mapping file for asset navigation |

### Content at Launch (EA 0.1–0.5)

**Cars (14):**
- Alpine A110 Group 4 (1973)
- Alfa Romeo GTA 1300 Junior Gr.2 (1972)
- Citroën Xsara WRC (2001)
- Fiat 131 Abarth Gr.4 (1976)
- Fiat 124 Sport Abarth Rally 16V Gr.4 (1973)
- Hyundai i20N Rally2 (2021)
- Lancia Delta HF Integrale EVO Gr.A (1992)
- Lancia Rally 037 EVO 2 Gr.B (1984)
- Lancia Stratos Gr.4 (1976)
- Lancia Fulvia Coupé 1.6 HF Gr.4 (1970)
- Mini Cooper S Gr.2 (1964)
- Peugeot 208 Rally4 (2020)
- Peugeot 306 Maxi Kit Car (1997)
- Škoda Fabia RS Rally2 (2022)
- Subaru Impreza S3 Group A (1993)

**Stages (8+):**
- Rally Alsace — Munster (gravel)
- Rally Alsace — Saverne (tarmac)
- Rally Wales — Hafren South (gravel)
- Rally Wales — Hafren North (gravel)
- Ghiacciodromo Livigno Circuit (snow/ice)
- La Bollène-Vésubie Col de Turini (tarmac)
- Sisteron (tarmac)
- Loutraki (gravel)
- Elatia (gravel)

### Modding Status

Unlike Assetto Corsa EVO (which received an official SDK in v0.7), ACR currently has **no official SDK**. Modding is community-driven via:
- **FModel** — UE4/5 asset explorer for extracting .pak contents
- **Unreal Engine 5.4** — required to cook modified assets back into .pak format
- **FSB BANK Extractor** — for FMOD audio bank extraction
- **Dumper-7** — for generating the .usmap mapping file needed by FModel
- Manual pak repacking with custom filenames (must end in _P)

This means ksEditor must work with UE5's cooked asset format directly — there is no official SDK for content creation, making the opportunity for a third-party tool like ksEditor even greater.

---

## 3. Vision: ksEditor as UE5 Replacement for ACR

### The Goal

Make ksEditor capable of covering **all Unreal Engine 5 features** required to create, edit, and manage ACR content — eliminating the need to install Unreal Engine or the UE5 Editor.

### The Asset Pipeline

`
Blender / 3ds Max / Maya ──FBX──→ ksEditor ──uasset──→ ACR (pak)
Photoshop / Substance ──PNG/TGA──→ ksEditor ──uexp────→ ACR (pak)
Audacity / Reaper ───WAV/OGG───→ ksEditor ──ubulk───→ ACR (pak)
                                    │
                            ┌───────┴───────┐
                            │   UE5 Asset   │
                            │   Pipeline    │
                            │   (cooked)    │
                            └───────────────┘
`

---

## 4. UE5 Systems to Replicate in ksEditor

### 4.1 UE5 Package System (uasset/uexp/ubulk)

UE5 uses the same fundamental package format as UE4 (.uasset + .uexp + .ubulk) with UE5-specific version numbers and new property types.

#### UE5-Specific Differences from UE4:

| Area | UE4 | UE5 |
|------|-----|-----|
| **Package Version** | FileVersionUE4 | FileVersionUE5 (newer constants) |
| **FName** | 16-bit case-preserving | 32-bit hashed (UE5.4+) |
| **Large World Coordinates** | No | Yes (double-precision) |
| **Virtual Texture** | Limited | Full VT support |
| **Nanite** | Not available | Mesh representation + cooking |
| **Chaos Physics** | Optional | Default physics engine |
| **Niagara** | Optional / legacy | Primary particle system |
| **String Table** | Limited | Full string table support |

#### Asset Type Map — UE5 → ksEditor

| UE5 Export Class | Contents | ksEditor Target |
|-----------------|----------|-----------------|
| UStaticMesh | LODs, sections, collision, UVs, material slots | Mesh editor (ksmodeler) |
| USkeletalMesh | Bones, skin weights, morph targets | Skeletal mesh + skinning |
| UMaterial | Expressions, inputs, shader graph | Material node graph editor |
| UMaterialInstanceConstant | Inherited parameter overrides | Material instance editor |
| UTexture2D | Compressed pixel data (BCn) | Texture viewer/editor |
| UAnimSequence | Compressed keyframes | Animation editor |
| UPhysicsAsset | Bodies and constraints (Chaos) | PhAT-like editor |
| UBlueprintGeneratedClass | Kismet bytecode | Blueprint decompiler |
| UWorld / ULevel (.umap) | Actors + transforms + references | Level editor 3D |
| USoundBase / USoundWave | Audio data (FMOD in ACR) | Audio viewer (ksAudioStudio) |
| UNiagaraSystem | Particle systems | Niagara editor (limited) |
| ULandscapeProxy | Terrain data | Landscape editor |

### 4.2 Content Browser / Asset Registry

ACR uses the same AssetRegistry.bin pattern as UE4 but with UE5-specific metadata.

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| AssetRegistry.bin | SQLite database with all ACR asset metadata | High |
| Content Browser (tree) | AssetsLibraryModule (existing) + ACR extension | High |
| PAK file browsing | Virtual filesystem overlay for .pak archives | High |
| Filter by type | Category filters in Content Browser | High |
| Search by name/path/tag | Full-text search in DB | High |
| Thumbnails | Thumbnailing system in AssetsLibraryModule | Medium |
| Asset dependencies | Dependency table in DB | Medium |

### 4.3 Material Editor

ACR uses cooked UE5 materials with **PBR (Physically Based Rendering) pipelines**. Unlike ACC which used Kunos-specific shaders, ACR materials are standard UE5 PBR with car-specific customizations.

#### Material Expressions (~250 node types to support)

| Category | Examples | Count |
|----------|----------|-------|
| **Texture** | TextureSample, TextureCoordinate, TextureObjectParameter, VirtualTexture | ~15 |
| **Math** | Add, Multiply, Lerp, Clamp, Saturate, Power, Abs, Ceil, Floor, Frac, Sine, Cosine, DDX, DDY | ~35 |
| **Vector** | MakeFloat2/3/4, BreakFloat2/3/4, AppendVector, ComponentMask, Noise | ~20 |
| **Constant** | Constant, Constant2Vector, Constant3Vector, Constant4Vector, ScalarParameter, VectorParameter | ~15 |
| **PBR** | NormalFromHeightmap, Fresnel, ReflectionVector, PixelNormalWS, BentNormal | ~25 |
| **Utility** | Time, VertexNormalWS, CameraVector, Transform, Distance, ActorPositionWS | ~35 |
| **Custom** | Custom HLSL node | ~5 |
| **Landscape** | LandscapeLayerCoords, LandscapeLayerSample, GrassWeight | ~10 |
| **Car-specific** | CarPaint flakes, ClearCoat, Carbon fiber weave, Dirt/scratch blending, Brake disc heat, Rain droplets | ~15+ |

#### Material Instance System

`
UMaterial (Master)
├── BaseColor:            Param(""PaintColor"", float3)
├── Roughness:            Param(""Roughness"", float)
├── Metallic:             Param(""Metallic"", float)
├── Normal:               TextureSample(T_CarbonFiber_N)
├── ClearCoat:            Param(""ClearCoat"", float)       ← UE5 Clear Coat shading model
├── ClearCoatRoughness:   Param(""ClearCoatRoughness"", float)
│
└── UMaterialInstanceConstant (child)
    ├── Override: PaintColor = (0.85, 0.12, 0.05)
    ├── Override: ClearCoat = 1.0
    └── Unset: Roughness → inherits from master
`

ksEditor already has a custom node graph system in src/core/customNodeGraph/. This must be extended to support:
- UE5 material expression types (~250 node types)
- UE5 shading models (Default, ClearCoat, Subsurface, Hair, Eye, Cloth)
- Pin types (float, float2, float3, float4, TextureObject, VirtualTexture, etc.)
- Compilation: node graph → serialized uasset properties
- 3D preview on sphere/cube using Vulkan

### 4.4 Static Mesh Editor

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Static Mesh Editor** | Existing 3D mesh editor (ksmodeler) | High |
| **Nanite Support** | Mesh representation flags | Medium |
| **LOD Management** | LOD tools in ksmodeler + LODExporter | High |
| **Collision** (primitive, convex, complex) | Collision editor | High |
| **UV Editor** | UV editor in ksmodeler | High |
| **Vertex Color Editor** | Vertex painting (PaintSettings) | Medium |
| **Lightmap UV Generation** | UV2 generation | Medium |
| **Mesh Sectioning** (material slots) | Material assignment per section | High |

### 4.5 Texture System

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Texture Import** (PNG, TGA, DDS, EXR, TIFF) | Already supported in core Graphics | High |
| **Texture Cooking** (BC1-BC7, ASTC, ETC2) | GPU-backed compression via DirectXTex | High |
| **Virtual Texture** | VT support is UE5-specific | Medium |
| **Texture Manager** | Streaming, mip generation | Medium |

**Texture compression formats required for ACR:**

| Format | UE5 Name | Use in ACR |
|--------|----------|------------|
| BC1 (DXT1) | TF_DXT1 | RGB diffuse (opaque) |
| BC3 (DXT5) | TF_DXT5 | RGBA diffuse (alpha) |
| BC4 | TF_BC4 | Single-channel (roughness, AO) |
| BC5 | TF_BC5 | Normal maps |
| BC6H | TF_BC6H | HDR textures (lighting, environment) |
| BC7 | TF_BC7 | High-quality RGB(A) |
| G8 | TF_G8 | Grayscale (height, mask) |

### 4.6 Particle System (Niagara)

ACR uses UE5's **Niagara** particle system for: dust clouds, snow spray, mud splash, rocks, exhaust, rain, water spray.

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Niagara Emitter** | Basic particle emitter (spawn → update → render) | Medium |
| **Niagara Modules** | Position, velocity, color, size modules | Medium |
| **Niagara Renderer** | Sprite, ribbon, mesh rendering | Medium |
| **Niagara Effect Type** | LOD and quality settings | Low |

### 4.7 Level / World Editor

ACR levels (.umap) contain positioned actors for rally stages. Unlike circuit tracks, rally stages are point-to-point and include terrain, vegetation, and dynamic props.

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **Level Editor** | 3D viewport with actor placement | High |
| **Actor Placement** | Drag & drop actors into scene | High |
| **Transform Tools** (W/E/R) | Already present in ksmodeler | High |
| **World Outliner** | Hierarchical actor tree | High |
| **Detail Panel** | Property grid for selected actor | High |
| **Landscape Editor** | Terrain sculpting (UE5 heightmap) | Medium |
| **Foliage Editor** | Paint vegetation instances | Medium |
| **Lightmap Resolution** | Light baking configuration | Medium |

### 4.8 Blueprint System

ACR uses Blueprints (visual scripting) for: stage logic, UI, camera sequences, event triggers.

Same strategy as ACC: start with **read-only bytecode decompiler**.

### 4.9 Animation System

| UE5 System | ksEditor Equivalent | Priority |
|------------|---------------------|----------|
| **AnimSequence** | Keyframe editor for bones | High |
| **AnimBlueprint** | State machine for animation blending | Medium |
| **BlendSpace** | 1D/2D animation interpolation | Medium |
| **Montage** | Non-blocking animation sections | Low |

### 4.10 Physics Asset (Chaos)

ACR uses UE5's **Chaos Physics** for damage and deformable bodies (unlike UE4's legacy PhAT).

| Feature | Description | Priority |
|---------|-------------|----------|
| **Body Creation** | Physics bodies per bone | High |
| **Collision Shapes** | Box, sphere, capsule, convex, level set | High |
| **Constraint Setup** | Joint angle limits, stiffness, damping | High |
| **Deformable Bodies** | Chaos geometry collection for damage | High |
| **Skeletal Physics** | Ragdoll simulation preview | Medium |

### 4.11 Audio (FMOD)

ACR uses **FMOD** for audio, with banks organized by category:

| Bank Folder | Contents |
|-------------|----------|
| Dialogue/ | Co-driver pacenotes (6 languages: EN, FR, IT, ES, DE, ZH) |
| EngExh/ | Engine + exhaust |
| UI/ | Menu sounds |
| Environment/ | Ambient, weather, surface |
| Tyres/ | Tyre scrub, slip, lockup |
| Transmission/ | Gearbox, drivetrain |
| Brakes/ | Brake squeal |
| Body/ | Impacts, scrapes, damage |
| Crowd/ | Spectator reactions |

| System | Description | Priority |
|--------|-------------|----------|
| **FMOD Bank Reader** | Read/extract ACR FMOD banks | High |
| **Waveform Viewer** | Audio waveform visualization | Already in ksAudioStudio |
| **FMOD Project Importer** | Import FMOD projects | Medium |
| **Spatial Audio** | 3D positional audio | Medium |

### 4.12 Cooking / Packaging Pipeline

When ACR ""cooks"" assets, UE5:
1. Resolves dependencies
2. Compiles shaders for target platform (DX12)
3. Compresses textures (BCn)
4. Serializes in cooked format (UE5-specific)
5. Packages into .pak + .ucas + .utoc files

| Feature | Description | Priority |
|---------|-------------|----------|
| **Asset Dependency Resolution** | Find all dependencies of an asset | High |
| **Texture Compression** | BC1-BC7 via DirectXTex (GPU) | High |
| **Shader Compilation** | HLSL → DX12 bytecode via dxcompiler.dll | High |
| **Package Serialization** | Write uasset + uexp in cooked format (UE5) | High |
| **Pak Packaging** | Create .pak + .ucas + .utoc (UE5 I/O store) | Medium |
| **Asset Registry Builder** | Generate AssetRegistry.bin | Medium |

---

## 5. ksAssettoCorsaRally.dll — Proposed Architecture

### CMake Target

`cmake
add_library(ksAssettoCorsaRally SHARED
    ksAssettoCorsaRally.cpp
    # UE5 Core
    ue5/FPackageFileSummary.h
    ue5/FNameTable.h
    ue5/FObjectImportExport.h
    ue5/UAssetSerializer.h/.cpp
    ue5/UAssetDeserializer.h/.cpp
    ue5/UPropertyReader.h/.cpp
    ue5/UPropertyWriter.h/.cpp
    ue5/UE5Version.h
    # UE5 Asset Types
    ue5/types/UStaticMesh.h/.cpp
    ue5/types/USkeletalMesh.h/.cpp
    ue5/types/UMaterial.h/.cpp
    ue5/types/UMaterialInstance.h/.cpp
    ue5/types/UTexture2D.h/.cpp
    ue5/types/UAnimSequence.h/.cpp
    ue5/types/UPhysicsAsset.h/.cpp
    ue5/types/UWorld.h/.cpp
    ue5/types/UBlueprint.h/.cpp
    ue5/types/USoundWave.h/.cpp
    ue5/types/UNiagaraSystem.h/.cpp
    ue5/types/ULandscapeProxy.h/.cpp
    # Factories
    ue5/factories/MeshFactory.h/.cpp
    ue5/factories/SkeletalMeshFactory.h/.cpp
    ue5/factories/TextureFactory.h/.cpp
    ue5/factories/MaterialFactory.h/.cpp
    ue5/factories/AnimationFactory.h/.cpp
    # Cooking
    ue5/cooking/TextureCompressor.h/.cpp
    ue5/cooking/ShaderCompiler.h/.cpp
    ue5/cooking/AssetCooker.h/.cpp
    ue5/cooking/PakWriter.h/.cpp
    ue5/cooking/AssetRegistryBuilder.h/.cpp
    # Editors
    editors/MaterialEditor/*.h/.cpp
    editors/BlueprintViewer/*.h/.cpp
    editors/LevelEditor/*.h/.cpp
    # ACR Integration
    ACRPaths.h/.cpp
    ACRSharedMemory.h/.cpp
    ACRStageImporter.h/.cpp
    ACRCarImporter.h/.cpp
    ACRQmlBridge.h/.cpp
    ACRLiveries.h/.cpp
)
target_link_libraries(ksAssettoCorsaRally PRIVATE
    kseditor_lib
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Qml Qt6::Quick
    Qt6::OpenGL Qt6::Multimedia Qt6::Sql
    ws2_32
    dxcompiler          # HLSL → DX12 bytecode (DXC)
    DirectXTex          # BCn texture compression
    FBX_SDK             # FBX import/export
    zlib                # Pak compression
    minizip             # Pak archive handling
)
set_target_properties(ksAssettoCorsaRally PROPERTIES
    OUTPUT_NAME ""ksAssettoCorsaRally""
    RUNTIME_OUTPUT_DIRECTORY ""/bin/plugins""
)
add_dependencies(kseditor ksAssettoCorsaRally)
`

### Plugin Entry-Point

`cpp
extern ""C"" {
    KS_ASSETTOCORSARALLY_API const char* getPluginId() {
        return ""ksAssettoCorsaRally"";
    }
    KS_ASSETTOCORSARALLY_API const char* getPluginName() {
        return ""Assetto Corsa Rally Plugin"";
    }
    KS_ASSETTOCORSARALLY_API const char* getPluginVersion() {
        return ""0.1.0"";
    }
    KS_ASSETTOCORSARALLY_API const char* getPluginDescription() {
        return ""Complete UE5 content pipeline for Assetto Corsa Rally — ""
               ""edit meshes, materials, textures, stages, and audio ""
               ""without Unreal Engine"";
    }
    KS_ASSETTOCORSARALLY_API bool initializePlugin() { ... }
    KS_ASSETTOCORSARALLY_API void shutdownPlugin() { ... }
    KS_ASSETTOCORSARALLY_API bool isPluginAvailable() { ... }
    KS_ASSETTOCORSARALLY_API const char* getInstallPath() { ... }
    KS_ASSETTOCORSARALLY_API void setInstallPath(const char* path) { ... }
}
`

### File Structure

`
src/plugins/simulators/kunos/assettocorsarally/
├── ksAssettoCorsaRally.cpp/.h          # Plugin C API
├── ksAssettoCorsaRally_export.h        # DLL export macros
│
├── ue5/                                # Core UE5 reader/writer
│   ├── FPackageFileSummary.h           # uasset header parser
│   ├── FNameTable.h                    # Name table reader
│   ├── FObjectImportExport.h           # Import/Export table
│   ├── UAssetSerializer.h/.cpp         # Generic uasset+uexp serializer
│   ├── UAssetDeserializer.h/.cpp       # Generic deserializer
│   ├── UPropertyReader.h/.cpp          # Property deserialization
│   ├── UPropertyWriter.h/.cpp          # Property serialization
│   ├── UE5Version.h                    # Supported UE5 versions
│   ├── FIoStoreReader.h/.cpp           # UE5 .ucas/.utoc I/O store reader
│   └── FPakFileReader.h/.cpp           # .pak archive reader
│
├── ue5/types/                          # Asset type serialization
│   ├── UStaticMesh.h/.cpp
│   ├── USkeletalMesh.h/.cpp
│   ├── UMaterial.h/.cpp
│   ├── UMaterialInstance.h/.cpp
│   ├── UTexture2D.h/.cpp
│   ├── UAnimSequence.h/.cpp
│   ├── UPhysicsAsset.h/.cpp
│   ├── UWorld.h/.cpp
│   ├── UBlueprint.h/.cpp
│   ├── USoundWave.h/.cpp
│   ├── UNiagaraSystem.h/.cpp
│   └── ULandscapeProxy.h/.cpp
│
├── ue5/factories/                      # Import/Export factories
│   ├── MeshFactory.h/.cpp              # FBX ↔ UStaticMesh
│   ├── SkeletalMeshFactory.h/.cpp      # FBX ↔ USkeletalMesh
│   ├── TextureFactory.h/.cpp           # PNG/DDS ↔ UTexture2D
│   ├── MaterialFactory.h/.cpp          # Graph ↔ UMaterial
│   └── AnimationFactory.h/.cpp         # FBX anim ↔ UAnimSequence
│
├── ue5/cooking/                        # Cooking pipeline
│   ├── TextureCompressor.h/.cpp        # BC1-BC7 via DirectXTex
│   ├── ShaderCompiler.h/.cpp           # HLSL → DX12 bytecode (DXC)
│   ├── AssetCooker.h/.cpp              # Cooking orchestrator
│   ├── PakWriter.h/.cpp                # .pak archive creation
│   ├── IoStoreWriter.h/.cpp            # UE5 .ucas/.utoc I/O store
│   └── AssetRegistryBuilder.h/.cpp     # Generate AssetRegistry.bin
│
├── editors/                            # Visual editors
│   ├── MaterialEditor/
│   │   ├── MaterialGraphNode.h
│   │   ├── MaterialGraphPin.h
│   │   ├── MaterialExpressionTypes.h   # ~250 expression types
│   │   └── MaterialPreview3D.h         # Vulkan material preview
│   ├── BlueprintViewer/
│   │   ├── KismetBytecodeReader.h
│   │   └── KismetDecompiler.h
│   └── LevelEditor/
│       ├── ActorPlacementTool.h
│       ├── WorldOutliner.h
│       └── DetailPanel.h
│
├── ACRPaths.h/.cpp                     # ACR installation detection
├── ACRSharedMemory.h/.cpp              # Live telemetry
├── ACRStageImporter.h/.cpp             # Stage data extraction
├── ACRCarImporter.h/.cpp               # Car data extraction
├── ACRLiveries.h/.cpp                  # Livery export/import
└── ACRQmlBridge.h/.cpp                 # QML bridges
`

### Dependencies

| Dependency | Purpose | Source |
|------------|---------|--------|
| kseditor_lib | Core editor | Built in-tree |
| Qt6::Core, Gui, Widgets, Network, Qml, Quick, OpenGL, Multimedia, Sql | Qt framework | Qt 6.11.1 |
| DXC (DirectXShaderCompiler) | HLSL → DX12 bytecode | GitHub (Microsoft) |
| DirectXTex | BC1-BC7 texture compression | GitHub (Microsoft) |
| FBX SDK (2020.2) | FBX mesh/animation import/export | Autodesk SDK |
| zlib | Pak compression | vcpkg (already present) |
| minizip | Pak archive handling | vcpkg |
| libpng, libjpeg-turbo | Texture decode | vcpkg (already present) |
| xxhash | UE5 FName hashing | vcpkg |
| OpenSSL (AES) | Pak encryption (if needed) | vcpkg |
| FMOD Studio API | Read ACR audio banks | FMOD SDK |

---

## 6. Implementation Roadmap

### Phase 1: Asset Reading (3 months)

`
Goal: ksEditor can read and display all ACR assets

[ ] Pak file reader (.pak + .ucas/.utoc I/O store)
[ ] FPackageFileSummary parser (uasset header)
[ ] NameTable reader/writer (UE5 32-bit hashed FName)
[ ] Import/Export table reader/writer
[ ] UProperty deserializer (core types + UE5-specific)
[ ] UStaticMesh deserializer (mesh data → internal ksEditor format)
[ ] UTexture2D deserializer (BCn → PNG preview)
[ ] UMaterial deserializer (expressions → node list)
[ ] 3D viewport mesh display
[ ] Texture viewer (with BCn decompression)
[ ] Material network tree viewer (read-only)
[ ] Blueprint bytecode reader + basic decompilation
[ ] Niagara system reader (emitter tree)
[ ] ACR installation detection
`

### Phase 2: Asset Writing (4 months)

`
Goal: ksEditor can modify and rewrite ACR assets

[ ] UProperty serializer (all core types + UE5)
[ ] UStaticMesh serializer (mesh edits → valid .uasset)
[ ] UTexture2D serializer (re-encode BCn → .uasset)
[ ] Material editor: base nodes (TextureSample, Constant, Multiply, Lerp, etc.)
[ ] Material editor: pin connections
[ ] Material editor: 3D preview on sphere
[ ] UMaterialInstance serializer (parameter overrides)
[ ] UMaterial serializer (full graph → valid .uasset)
[ ] Dependency management (import table updates)
[ ] Pak writer (.pak creation)
[ ] I/O store writer (.ucas/.utoc for UE5)
[ ] Validation: edited asset loads in ACR
`

### Phase 3: Full Pipeline (3 months)

`
Goal: ksEditor has complete import → edit → export pipeline

[ ] FBX → UStaticMesh factory (import from Blender/Maya)
[ ] UStaticMesh → FBX factory (export for DCC tools)
[ ] FBX → USkeletalMesh (with skeleton and skinning)
[ ] PNG/DDS → UTexture2D (with BCn compression)
[ ] UTexture2D → PNG (decompression)
[ ] Texture compressor: BC1, BC3, BC5, BC6H, BC7 via DirectXTex
[ ] Cooker: resolve dependencies → compile → serialize
[ ] AssetRegistry.bin builder
[ ] Material editor: ~50 most common expression types
[ ] Material instance editor (parameter overrides)
`

### Phase 4: Advanced Editors (4 months)

`
Goal: ksEditor can fully replace UE5 Editor for ACR

[ ] Level Editor: open/save .umap
[ ] Level Editor: place, move, rotate, scale actors
[ ] Level Editor: World Outliner with hierarchy
[ ] Level Editor: Detail Panel for actor properties
[ ] Level Editor: landscape terrain support
[ ] Chaos Physics Asset editor (deformable bodies)
[ ] LOD management (creation, preview, export)
[ ] UV editor + UV2 generation (lightmap)
[ ] Animation editor (keyframes, curves)
[ ] Blueprint decompiler (readable pseudo-code)
[ ] Collision editor (primitive, convex, complex, level set)
[ ] Audio: read ACR FMOD banks
[ ] Audio: preview sounds
[ ] Livery system: extract/import liveries from ACR
[ ] Stage importer: extract stage geometry and props
`

### Phase 5: ACR Integration (2 months)

`
Goal: Complete ksAssettoCorsaRally plugin

[ ] Content Browser: browse cars, stages, audio, UI assets
[ ] Content Browser: thumbnail previews
[ ] Live telemetry from ACR shared memory (if available)
[ ] Livery export/import (replace UE5 cooking for liveries)
[ ] Car importer: extract and reimport car meshes
[ ] Stage importer: full stage reimport pipeline
[ ] Workshop integration (if ACR supports it)
[ ] Deploy: write modified assets to ACR pak folder
`

### Total Estimate: ~16 months (team of 3 developers)

---

## 7. Gap Analysis vs. Full UE5 Editor

| UE5 Feature | Effort | Needed for ACR? | Decision |
|-------------|--------|-----------------|----------|
| Blueprint full editor | Very High | Low | Read-only decompiler only |
| Blueprint compiler | Extremely High | Very Low | Do not implement |
| Niagara full editor | High | Medium | Basic emitter/module editor |
| Sequencer | High | Medium | Basic timeline editor |
| Landscape full editor | High | Medium | Basic sculpting |
| Foliage editor | Medium | Low | Do not implement |
| Nanite support | Medium | Low | Mesh flags only |
| Chaos Physics | High | High | Implement for damage |
| Lumen / GI | N/A | Not needed at asset level | Read-only |
| Virtual Texture | High | Low | Read-only |

---

## 8. Risks and Mitigations

### Technical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **UE5 cooked format varies** between ACR updates | High | Version detection; test with every ACR update |
| **UE5 I/O store format (.ucas/.utoc)** is complex | High | Reference FModel/UE5 source |
| **Chaos Physics asset format** undocumented | Medium | Reverse-engineer with UE5 Editor |
| **Blueprint bytecode complexity** | Medium | Start read-only; compiler is very hard |
| **Texture cooking performance** | Medium | DirectXTex with GPU acceleration |
| **FMOD bank structure** | Medium | ACR uses standard FMOD; readable via FMOD API |
| **Pak AES encryption** | Low | ACR currently does not encrypt paks; monitor |
| **No official SDK** | Medium | Community tools (FModel) provide reference |

### Legal Risks

- **ACR EULA**: Verify if asset modification is permitted
- **Reverse engineering**: Parsing cooked format may be considered reverse engineering. Seek legal advice
- **Distribution**: ksEditor is MIT, but distributing extracted ACR assets is forbidden

---

## 9. Community Ecosystem

### Existing Libraries for Reference

| Project | Language | Purpose |
|---------|----------|---------|
| [FModel](https://github.com/4sval/FModel) | C# | UE4/5 asset explorer |
| [UAssetAPI](https://github.com/atenfyr/UAssetAPI) | C# | Full uasset read/write (UE4.13–5.7) |
| [UAssetGUI](https://github.com/atenfyr/UAssetGUI) | C# | Visual uasset editor |
| [UE4SS](https://github.com/UE4SS-RE/UE4SS) | C++ | UE4/5 debugger + serialization |
| [DirectXTex](https://github.com/Microsoft/DirectXTex) | C++ | BCn texture compression |
| [UModel (UE Viewer)](https://www.gildor.org/) | C++ | UE4/5 model viewer |
| [Dumper-7](https://github.com/Encryqed/Dumper-7) | C++ | UE4/5 SDK/mapping generator |
| [FSB BANK Extractor](https://github.com/IZH318/FSB_BANK_Extractor) | C# | FMOD bank extraction |

### Community Modding Tools for ACR

| Tool | Purpose |
|------|---------|
| **FModel** | Extract .pak → .uasset + preview assets |
| **Unreal Engine 5.4** | Cook modified assets → .pak |
| **Dumper-7** | Generate .usmap mapping file |
| **FSB BANK Extractor** | Extract FMOD audio banks |
| **Fmod-Bank-Tools** | Repack FMOD banks (binary replacement) |

---

## 10. Comparison: ksAssettoCorsa vs ksAssettoCorsaRally

| Aspect | ksAssettoCorsa (AC1) | ksAssettoCorsaRally (ACR) |
|--------|---------------------|---------------------------|
| **Status** | ✅ Exists, functional | ❌ Does not exist |
| **Target game** | Assetto Corsa (2014) | Assetto Corsa Rally (2025) |
| **Engine** | Proprietary (in-house) | Unreal Engine 5.4.3 |
| **Source files** | ~40 files (20k+ lines) | ~75+ files (est. 55k+ lines) |
| **CMake target** | ksAssettoCorsa | Not defined |
| **Model format** | .kn5 (proprietary binary) | .uasset + .uexp + .ubulk (UE5 cooked) |
| **Content modding** | Full (cars, tracks, skins, apps) | Via UE5 replacement pipeline |
| **Telemetry** | Shared memory | TBD (likely shared memory) |
| **CSP support** | Extensive | Not applicable (native UE5 shaders) |
| **Workshop** | Full integration | TBD |
| **Modding policy** | Open | No official SDK (community-driven) |
| **Official SDK** | KN5 exporter | None (community tools only) |
| **Primary use** | Content editing, model parsing | Full UE5 content pipeline + telemetry |

---

## 11. Conclusion

ksAssettoCorsaRally.dll is a **future product that has not been implemented**. The vision is ambitious: make ksEditor a **complete replacement for Unreal Engine 5** for the purpose of editing Assetto Corsa Rally content, eliminating the need to install UE5.

The key technical challenge is implementing read/write support for UE5's cooked asset format (.uasset/.uexp/.ubulk) — a proprietary binary format that varies between engine versions. Unlike ACC (UE4), ACR uses UE5 with additional complexities: I/O store format (.ucas/.utoc), 32-bit hashed FNames, Chaos Physics, and Niagara particles.

Once the format layer is in place, ksEditor's existing tools can be extended:
- **ksmodeler** for static/skeletal mesh editing
- **ksAudioStudio** for FMOD audio
- **CustomNodeGraph** for the material editor
- **AssetsLibraryModule** for the content browser
- **Vulkan renderer** for 3D previews

The existing codebase provides foundational elements — rally tire coefficients in PacejkaTireModel, the SoundWizard rally category, and the plugin architecture from ksAssettoCorsa. However, the UE5 format layer is entirely new infrastructure with no equivalent in the current codebase.
