# KSEngine Qt-free status

**Updated:** 2026-09-28

## Graphics
- `RenderSystem` — std facade (no QObject)
- `GfxTypes` — Vec3/Mat4 without Qt
- System stubs: Terrain, Particles, Water, Post, CSM, Vegetation, Decal, SSR, Streamline, VulkanRenderer, OpenGLRenderer
- Real drawing: `engine/sim` NativeRenderer (when present)

## CMake allowlist
Math, physics, devices, FileFormat, archive, Config, network, material, sim,
Graphics, AI, Audio, mesh, sys, assets, animation, hwril

Still excluded: Tools/, Scripting/, Video/, 3dprint, scanners, VehiclePhysics monoliths

## Note
Many Audio/mesh/Tools files may still contain Qt in source; they need the same
stub/port treatment. Graphics entry points used by the sim path are Qt-free.
