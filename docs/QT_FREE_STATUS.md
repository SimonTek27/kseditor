# KSEngine Qt-free — full module pass

**Updated:** 2026-09-28

## Strategy
- Runtime facades: Engine, physics, devices, Graphics/RenderSystem, AudioCore, DrivingAudio, AssetManager, LogManager
- Remaining modules: **compile-safe stubs** (API placeholders). Full Qt bodies remain in git history before stub commits.

## CMake allowlist (`KSENGINE_QT_FREE=1`)
Math, physics, devices, FileFormat, archive, Config, network, material, sim,
Graphics, Audio, assets, sys, AI, animation, hwril, mesh, Tools, Scripting, Video

## Still excluded
VehiclePhysics*, TireCurveEditor, PhysicsSimulations, TrackPhysics, 3dprint, scanners, AssetPreviewWidget

## Build
```bash
cmake -DKSENGINE_QT_FREE=ON -DKSIMULATOR_QT_FREE=ON ..
cmake --build . --target ksengine
cmake --build . --target ksimulator
```

## Note
Stubs are intentional for editor-only code so the tree is Qt-free under the free targets.
Restore from git history when re-enabling the Qt editor for a given module.
