# Graphics

ksEditor uses a Vulkan-based rendering pipeline with GLSL shaders, supporting PBR materials, 3D modeling, and real-time viewport rendering.

## Renderer

**`src/core/Graphics/`** — 22 files + 12 GLSL shaders:

- **Vulkan Renderer** — Core rendering pipeline
- **Scene Graph** — Hierarchical scene management
- **Scene Data / Mesh / Object** — Scene entity system
- **Shader Material System** — PBR material pipeline
- **Texture Tools** — Texture loading, processing, and management
- **Post-Process Shaders** — Post-processing effects
- **Shadow / Sky Shaders** — Shadow mapping and sky rendering

**Shader types:**
- Per-pixel lighting, Per-pixel AT (alpha test)
- Multi-layer blending
- Post-process
- Shadow mapping
- Sky rendering

## 3D Modeling

**`src/core/mesh/`** — 39 files for mesh operations:

- Mesh creation and editing
- Boolean operations (Union, Difference, Intersection, XOR)
- UV unwrapping (LSCM, Harmonic, custom seam detection)
- Subdivision surfaces
- Sculpting tools
- Skeleton / skinning system
- Weight painting
- Physics mesh generation
- Normals map baking
- Shape keys / morph targets
- Modifier stack

**`src/core/material/`** — 14 files:
- Material library and shader manager
- Texture atlas and texture paint system
- Texture paint QML bridge
- PBR material definitions

## 3D Modeling Module

**`src/modules/modellingEditor/`** — 49 files:
- Primitives, boolean ops, UV unwrap
- Geometry nodes system
- Car / Track / Character builders
- Physics mesh generation
- 3D viewport
- Rig generator
- Scene mesh management

## Showroom

**`src/modules/ShowroomEditor/`** — 10 files for 3D preview and showcase rendering.

## Feature Comparison

| Feature | ksEditor | Blender |
|---------|----------|---------|
| Vulkan Renderer | Yes | No (Cycles/Eevee) |
| PBR Materials | Yes | Yes |
| Boolean Ops | Yes | Yes |
| UV Unwrapping | Yes | Yes |
| Subdivision | Yes | Yes |
| Sculpting | Yes | Yes |
| Skeleton/IK | Yes | Yes |
| Parity | 98% | — |
